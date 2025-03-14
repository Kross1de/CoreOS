#!/bin/bash

# cmanru script

# Define the names of the files
KERNEL_ASM="kernel.asm"
KERNEL_C="kernel.c"
LINKER_SCRIPT="link.ld"
OUTPUT="kernel.bin"
ISO_DIR="iso"
ISO="coreos.iso"

# Assemble kernel.asm
nasm -f elf32 -o kasm.o $KERNEL_ASM

# Compile kernel.c with stack protection disabled
gcc -m32 -ffreestanding -fno-stack-protector -c -o kc.o $KERNEL_C

# Link the object files
ld -m elf_i386 -T $LINKER_SCRIPT -o $OUTPUT kasm.o kc.o

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

# Run the OS using QEMU
qemu-system-i386 -cdrom $ISO
