# Project_OS

![Capture](https://github.com/Josue-Castellanos/Project_OS/assets/98190733/a1d5a576-0f8a-4cac-a669-597dd16e81d1)

Commands to run OS:

docker build buildenv -t bill-buildenv

docker run --rm -it -v "%cd%":/root/env bill-buildenv

make build-x86_64

make clean

qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso
