# Building on Ubuntu VM (Apple Silicon Mac Workaround)

Since ESP8266-RTOS-SDK doesn't support Apple Silicon (ARM64), use your Parallels Ubuntu VM.

## Setup (One-Time in Ubuntu VM)

```bash
# Map this folder to your Ubuntu VM
# In Parallels: Shared Folders → Add /Users/byronjohnson/Development/esp-homekit-sdk

# In Ubuntu VM:
cd /media/psf/Home/Development/esp-homekit-sdk  # or wherever it's mounted

# Install dependencies
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone ESP8266-RTOS-SDK
cd ~
git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
cd ESP8266_RTOS_SDK
./install.sh

# Set up environment
export IDF_PATH=~/ESP8266_RTOS_SDK
source $IDF_PATH/export.sh

# Build the firmware
cd /media/psf/Home/Development/esp-homekit-sdk/examples/keepconnect
idf.py build
```

## Future Builds

```bash
# In Ubuntu VM:
export IDF_PATH=~/ESP8266_RTOS_SDK
source $IDF_PATH/export.sh
cd /media/psf/Home/Development/esp-homekit-sdk/examples/keepconnect
idf.py build
```

Binaries will be in `build/` - accessible from both macOS and Ubuntu since it's a shared folder.

## Then Build Migrator (Back on macOS)

```bash
# On macOS:
cd /Users/byronjohnson/Development/esp-homekit-sdk/migrator
./build_migrator.sh
```

This uses PlatformIO (which DOES support Apple Silicon) to build the Arduino-based migrator.
