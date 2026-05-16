CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2

TARGET  = vm_translate
SRC     = vm_translate.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET) 8 example_addresses.txt 12345

clean:
	rm -f $(TARGET)
