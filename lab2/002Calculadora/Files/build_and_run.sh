#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Remove previous compiled objects and binaries
echo "Cleaning up previous build files..."
rm -f ./program/root.o ./program/main.o calculadora.elf calculadora.bin

echo "Assembling startup.s..."
arm-none-eabi-as -o ./program/root.o ./program/root.s

echo "Compiling os.c..."
arm-none-eabi-gcc -c -o ./program/os.o ./os/os.c

echo "Compiling stdio.c..."
arm-none-eabi-gcc -c -o ./lib/stdio.o ./lib/stdio.c

echo "Compiling main.c..."
arm-none-eabi-gcc -c -o ./program/main.o ./program/main.c

echo "Linking object files..."
#arm-none-eabi-ld -T linker.ld -o calculadora.elf ./program/root.o ./program/main.o ./program/os.o ./lib/stdio.o
arm-none-eabi-gcc -T linker.ld -nostdlib -Wl,--gc-sections \
  -o calculadora.elf \
  ./program/root.o ./program/main.o ./program/os.o ./lib/stdio.o \
  -lgcc

echo "Converting ELF to binary..."
arm-none-eabi-objcopy -O binary calculadora.elf calculadora.bin

echo "Running QEMU..."
qemu-system-arm -M versatilepb -nographic -kernel calculadora.elf --audio none
