# Project_OS
Commands to run OS:

docker build buildenv -t bill-buildenv

docker run --rm -it -v "%cd%":/root/env bill-buildenv

make build-x86_64

make clean

qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso