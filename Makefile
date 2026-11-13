CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
SRCS = $(wildcard *.c)
TARGET = search

all: $(TARGET)

$(TARGET): $(SRCS) common.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

split_cranfield: tools/split_cranfield.c loader.c common.h
	$(CC) $(CFLAGS) -o split_cranfield tools/split_cranfield.c loader.c


clean:
ifeq ($(OS),Windows_NT)
	del /Q $(TARGET).exe split_cranfield.exe 2>nul
else
	rm -f $(TARGET) split_cranfield
endif

.PHONY: all clean 
