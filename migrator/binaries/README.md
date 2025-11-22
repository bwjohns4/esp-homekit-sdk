# Binary Files Directory

This directory is for storing the IDF firmware binaries before conversion.

## What goes here

After building the HomeKit firmware in `examples/keepconnect/`, copy the binaries here for convenience:

```bash
cp ../examples/keepconnect/build/bootloader/bootloader.bin ./
cp ../examples/keepconnect/build/partition_table/partition-table.bin ./
cp ../examples/keepconnect/build/smart_outlet.bin ./
```

Then run the conversion:
```bash
cd ..
./convert_bins.sh binaries/bootloader.bin binaries/partition-table.bin binaries/smart_outlet.bin
```

## Note

These binary files are gitignored. The conversion script can also accept absolute paths, so this directory is optional - just for organization.
