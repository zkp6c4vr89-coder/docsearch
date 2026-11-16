CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I. -Isrc
SRCS = $(wildcard *.c) $(wildcard src/*.c)
HDRS = $(wildcard *.h) $(wildcard src/*.h)
TARGET = search

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

split_cranfield: tools/split_cranfield.c loader.c common.h
	$(CC) $(CFLAGS) -o split_cranfield tools/split_cranfield.c loader.c

# same program built with AddressSanitizer + UBSan, to catch memory bugs
# builds ./search_asan, so ./search is always the normal build
sanitize: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET)_asan $(SRCS)

clean:
ifeq ($(OS),Windows_NT)
	del /Q $(TARGET).exe $(TARGET)_asan.exe split_cranfield.exe 2>nul
else
	rm -f $(TARGET) $(TARGET)_asan split_cranfield
endif

.PHONY: all clean sanitize 
