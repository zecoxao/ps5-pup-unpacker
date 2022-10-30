# pup_unpacker
A utility to unpack PS5 update blobs that have been previously decrypted using [pup_decrypt](https://github.com/zecoxao/ps5-pup-decrypt/). this is based on [idc](https://github.com/idc)/[ps4-pup_unpack](https://github.com/idc/ps4-pup_unpack) rewritten with C++ and runs on Linux/macOS/Win32


## Note
This utility will not unpack the contents of nested filesystems. The filesystem images in updates are FAT32, exFAT, etc images and can be mounted or unpacked with other tools (for example 7zip with Formats exFAT).


## To Build
```
cmake .
make 
```
