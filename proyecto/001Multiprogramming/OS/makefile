CROSS = arm-none-eabi
CC = $(CROSS)-gcc
AS = $(CROSS)-gcc
LD = $(CROSS)-gcc
OBJCOPY = $(CROSS)-objcopy
OBJDUMP = $(CROSS)-objdump

CFLAGS = -mcpu=cortex-a8 -marm -ffreestanding -nostdlib -nostartfiles -O0 -g -Wall -Wextra
LDFLAGS = -T linker.ld -nostdlib -nostartfiles -ffreestanding

TARGET = os

all: $(TARGET).bin $(TARGET).lst

$(TARGET).elf: root.o os.o linker.ld
	$(LD) $(CFLAGS) root.o os.o -o $@ $(LDFLAGS)

root.o: root.s
	$(AS) $(CFLAGS) -c $< -o $@

os.o: os.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -D $< > $@

clean:
	rm -f *.o *.elf *.bin *.lst
