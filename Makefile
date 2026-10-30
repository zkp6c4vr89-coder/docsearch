# DocSearch - build file
#   make            build ./docsearch (docsearch.exe on Windows)
#   make test       build and run the unit tests
#   make asan       debug build with AddressSanitizer + UBSan (macOS/Linux)
#   make clean      remove build outputs
# On Windows use MinGW-w64:  mingw32-make     (or run build.bat)

CC      ?= gcc
CSTD    := -std=c11
WARN    := -Wall -Wextra -pedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes
CFLAGS  ?= -O2
CFLAGS  += $(CSTD) $(WARN)
LDLIBS  := -lm

ifeq ($(OS),Windows_NT)
    EXE     := .exe
    LDLIBS  += -lws2_32
    RM      := del /Q
else
    EXE     :=
    RM      := rm -f
endif

SRC      := $(wildcard src/*.c)
LIB_SRC  := $(filter-out src/main.c,$(SRC))
TEST_SRC := $(wildcard tests/*.c)
BIN      := docsearch$(EXE)
TEST_BIN := run_tests$(EXE)

.PHONY: all test asan clean

all: $(BIN)

$(BIN): $(SRC) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

$(TEST_BIN): $(LIB_SRC) $(TEST_SRC) $(wildcard src/*.h) $(wildcard tests/*.h)
	$(CC) $(CFLAGS) -Isrc -o $@ $(LIB_SRC) $(TEST_SRC) $(LDLIBS)

test: $(TEST_BIN)
	./$(TEST_BIN)

asan: CFLAGS := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
asan:
	$(CC) $(CFLAGS) $(CSTD) $(WARN) -o docsearch_asan $(SRC) $(LDLIBS)
	$(CC) $(CFLAGS) $(CSTD) $(WARN) -Isrc -o run_tests_asan $(LIB_SRC) $(TEST_SRC) $(LDLIBS)

clean:
	$(RM) $(BIN) $(TEST_BIN) docsearch_asan run_tests_asan
