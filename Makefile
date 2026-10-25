CC     = gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

BIN = transit
SRC = src/transit.c

.PHONY: all test clean

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

# Needs a POSIX shell (Linux, macOS, WSL or Git Bash).
test: $(BIN)
	sh tests/smoke.sh ./$(BIN)

clean:
	rm -f $(BIN) $(BIN).exe
