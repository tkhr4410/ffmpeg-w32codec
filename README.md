# ffmpeg-w32codec

Copyright (c) 2024-2025 Takahiro Ishida

This project is licensed under the MIT License.

## Build environment setup

1. Install MSYS2 (https://www.msys2.org/)
2. Run "mingw32.bat"
3. Run the following commands:
``` sh
$ pacman -Syuu
$ pacman -S mingw-w64-i686-toolchain
$ pacman -S mingw-w64-i686-diffutils
$ pacman -S mingw-w64-i686-clang
$ pacman -S mingw-w64-i686-llvm-tools
$ pacman -S make
$ pacman -S cmake
$ pacman -S ninja
```

## Build

1. Run "mingw32.bat"
2. Run the following commands:
``` sh
$ cd external
$ sh ffmpeg_config.sh
$ sh ffmpeg_build.sh
$ cmake -S . -B ../build -G Ninja -DCMAKE_TOOLCHAIN_FILE=mingw32-clang.cmake
$ cd ../build
$ ninja
```
