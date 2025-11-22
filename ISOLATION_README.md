# Ultra-Isolated Toolchain Setup

Everything ESP8266-related stays in ONE folder: `.esp-tools/`

## What Gets Installed Where

```
esp-homekit-sdk/
├── .esp-tools/                    ← EVERYTHING goes here
│   ├── ESP8266_RTOS_SDK/          ← The SDK
│   ├── python-venv/               ← Isolated Python virtualenv
│   └── .espressif/                ← Toolchain (xtensa-gcc, etc)
├── activate_toolchain.sh          ← Run this to use the tools
└── setup_isolated_toolchain.sh    ← Run once to install
```

## Installation (One Command)

```bash
cd /Users/byronjohnson/Development/esp-homekit-sdk
./setup_isolated_toolchain.sh
```

This will:
1. Create `.esp-tools/` directory
2. Set up isolated Python virtualenv inside it
3. Clone ESP8266-RTOS-SDK inside it
4. Install toolchain inside it
5. Build your firmware
6. Takes ~30 minutes

**ZERO system pollution:**
- ✓ No global Python packages
- ✓ No PATH modifications outside this folder
- ✓ No system-wide installs (except Homebrew deps: cmake, ninja)
- ✓ Everything self-contained

## Using the Toolchain (After Install)

Every time you want to build firmware in a new terminal:

```bash
cd /Users/byronjohnson/Development/esp-homekit-sdk
source ./activate_toolchain.sh

# Now you can use idf.py
cd examples/keepconnect
idf.py build
```

## Deactivating

```bash
deactivate
```

Your terminal returns to normal. No ESP8266 stuff in PATH anymore.

## Completely Removing Everything

When you're done with the project:

```bash
rm -rf .esp-tools/
rm activate_toolchain.sh
```

Done. Your Mac is back to exactly how it was (except Homebrew packages cmake/ninja, which are harmless).

## Disk Space

Expect ~1.5-2 GB in `.esp-tools/`:
- SDK: ~500 MB
- Toolchain: ~800 MB
- Python venv: ~200 MB

## Why This is Better Than PlatformIO

PlatformIO already installed:
- `~/.platformio/` (hundreds of MB)
- System-wide or user Python packages
- Multiple framework versions

This approach:
- Single directory
- Easy to remove
- Full IDF functionality (not subset)
- No version conflicts

## How It Works

1. **Python virtualenv** - Creates isolated Python in `.esp-tools/python-venv/`
2. **IDF_PATH** - Points to `.esp-tools/ESP8266_RTOS_SDK/`
3. **IDF_TOOLS_PATH** - Forces toolchain into `.esp-tools/.espressif/`
4. **activate_toolchain.sh** - Sets these vars only when you run it

When you close terminal or run `deactivate`, all environment changes are gone.

## Troubleshooting

**Q: Can I move the project folder?**
A: Yes, but re-run `setup_isolated_toolchain.sh` (it updates paths in activation script)

**Q: Can I have multiple projects?**
A: Each project can have its own `.esp-tools/`, or symlink to share one

**Q: What if I already have IDF installed?**
A: This won't interfere. It's completely separate.

**Q: Does this work for ESP32 too?**
A: No, this is ESP8266-specific. But same approach works for ESP32 (use ESP-IDF instead)
