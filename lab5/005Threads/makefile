CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11
TARGET = log_analyzer
SRC = main_linux.c log_processor.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET) access.log 4

clean:
	rm -f $(TARGET)
