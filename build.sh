#!/bin/bash

# Define the names of the files
KERNEL_ASM="kernel.asm"
KERNEL_C="kernel.c"
KEYBOARD_C="keyboard.c"
KEYBOARD_MAP_C="keyboard_map.c"
LINKER_SCRIPT="link.ld"
OUTPUT="kernel.bin"
ISO_DIR="iso"
ISO="coreos.iso"

# Assemble kernel.asm
nasm -f elf32 -o kasm.o $KERNEL_ASM

# Compile C files with stack protection disabled
gcc -m32 -ffreestanding -fno-stack-protector -c -o kc.o $KERNEL_C
gcc -m32 -ffreestanding -fno-stack-protector -c -o keyboard.o $KEYBOARD_C
gcc -m32 -ffreestanding -fno-stack-protector -c -o keyboard_map.o $KEYBOARD_MAP_C

# Link the object files
ld -m elf_i386 -T $LINKER_SCRIPT -o $OUTPUT kasm.o kc.o keyboard.o keyboard_map.o

# Create ISO directory structure
mkdir -p $ISO_DIR/boot/grub
cp $OUTPUT $ISO_DIR/boot/

# Create GRUB configuration file
echo 'menuentry "CoreOS" {' > $ISO_DIR/boot/grub/grub.cfg
echo '  multiboot /boot/kernel.bin' >> $ISO_DIR/boot/grub/grub.cfg
echo '  boot' >> $ISO_DIR/boot/grub/grub.cfg
echo '}' >> $ISO_DIR/boot/grub/grub.cfg

# Create bootable ISO
grub-mkrescue -o $ISO $ISO_DIR

# Clean up object files
rm *.o

# Run the OS using QEMU
qemu-system-i386 -cdrom $ISO