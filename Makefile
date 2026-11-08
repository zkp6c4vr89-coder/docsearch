CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
SRCS = $(wildcard *.c)
TARGET = search

all: $(TARGET)

$(TARGET): $(SRCS) common.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
ifeq ($(OS), Windows_NT)
	del /Q $(TARGET).exe 2>nul
else
	rm -f $(TARGET)
endif

.PHONY: all clean 
