#ifndef RAW_FLASH_WRITER_H
#define RAW_FLASH_WRITER_H

#include <Arduino.h>

#define FLASH_SECTOR_SIZE 4096
#define RAW_FLASH_BUFFER_SIZE 512

class RawFlashWriter {
public:
    RawFlashWriter() : _buffer(nullptr), _bufferLen(0), _startAddress(0),
                       _currentAddress(0), _size(0), _error(0) {}

    ~RawFlashWriter() {
        if (_buffer) {
            delete[] _buffer;
            _buffer = nullptr;
        }
    }

    // Initialize writer
    bool begin(size_t size, uint32_t address) {
        if (_buffer) {
            delete[] _buffer;
        }

        _buffer = new uint8_t[RAW_FLASH_BUFFER_SIZE];
        if (!_buffer) {
            _error = 1;  // Allocation error
            return false;
        }

        _bufferLen = 0;
        _startAddress = address;
        _currentAddress = address;
        _size = size;
        _error = 0;

        return true;
    }

    // Write data (buffers and writes when buffer is full or at sector boundary)
    size_t write(const uint8_t *data, size_t len) {
        if (!_buffer || _error) {
            return 0;
        }

        size_t written = 0;

        while (written < len) {
            // Check if we need to flush before adding more data
            // Flush if: buffer is full OR next write position is at sector boundary
            if (_bufferLen >= RAW_FLASH_BUFFER_SIZE ||
                (_bufferLen > 0 && (_currentAddress + _bufferLen) % FLASH_SECTOR_SIZE == 0)) {
                if (!_writeBuffer()) {
                    return written;  // Return what we wrote before error
                }
            }

            // Calculate how much we can write to buffer
            size_t available = RAW_FLASH_BUFFER_SIZE - _bufferLen;

            // Don't cross sector boundary within a single buffer
            size_t untilSectorEnd = FLASH_SECTOR_SIZE - ((_currentAddress + _bufferLen) % FLASH_SECTOR_SIZE);
            available = min(available, untilSectorEnd);

            size_t toWrite = min(available, len - written);

            // Copy to buffer
            memcpy(_buffer + _bufferLen, data + written, toWrite);
            _bufferLen += toWrite;
            written += toWrite;
        }

        return written;
    }

    // Finalize - write any remaining buffered data
    bool end() {
        if (_error) {
            return false;
        }

        // Write any remaining data in buffer
        if (_bufferLen > 0) {
            // Pad to 4-byte alignment
            while (_bufferLen % 4 != 0) {
                _buffer[_bufferLen++] = 0xFF;
            }

            if (!_writeBuffer()) {
                return false;
            }
        }

        return true;
    }

    int getError() const {
        return _error;
    }

private:
    bool _writeBuffer() {
        if (_bufferLen == 0) {
            return true;
        }

        bool eraseResult = true;
        bool writeResult = true;

        // Erase sector if at sector boundary (BEFORE writing!)
        if (_currentAddress % FLASH_SECTOR_SIZE == 0) {
            uint32_t sector = _currentAddress / FLASH_SECTOR_SIZE;

            Serial.printf("  Erasing sector %u at 0x%06X...\n", sector, _currentAddress);
            Serial.flush();  // Ensure serial output completes

            // CRITICAL: Give system time to service watchdog
            optimistic_yield(10000);  // Same as Updater uses
            ESP.wdtFeed();

            eraseResult = ESP.flashEraseSector(sector);

            // CRITICAL: Feed watchdog immediately after erase
            optimistic_yield(10000);
            ESP.wdtFeed();

            if (!eraseResult) {
                _error = 2;  // Erase error
                Serial.printf("ERROR: Failed to erase sector %u at 0x%06X\n", sector, _currentAddress);
                return false;
            }

            Serial.printf("  ✓ Sector %u erased\n", sector);
        }

        // Write buffer to flash
        if (eraseResult) {
            yield();

            // Ensure buffer is 4-byte aligned
            size_t alignedLen = (_bufferLen + 3) & ~3;
            if (alignedLen > _bufferLen) {
                // Pad with 0xFF
                for (size_t i = _bufferLen; i < alignedLen; i++) {
                    _buffer[i] = 0xFF;
                }
            }

            writeResult = ESP.flashWrite(_currentAddress, reinterpret_cast<uint32_t*>(_buffer), alignedLen);
            yield();
            ESP.wdtFeed();

            if (!writeResult) {
                _error = 3;  // Write error
                Serial.printf("ERROR: Failed to write at 0x%06X\n", _currentAddress);
                return false;
            }
        }

        // Update position (use original bufferLen, not aligned)
        _currentAddress += _bufferLen;
        _bufferLen = 0;

        return true;
    }

    uint8_t *_buffer;
    size_t _bufferLen;
    uint32_t _startAddress;
    uint32_t _currentAddress;
    size_t _size;
    int _error;
};

#endif // RAW_FLASH_WRITER_H
