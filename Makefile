CC      := gcc
CFLAGS  := -std=c99 \
           -D_POSIX_C_SOURCE=200809L \
           -D_XOPEN_SOURCE=700 \
           -Wall -Wextra -Werror \
           -Wno-unused-parameter \
           -fno-asm -I include

SRC     := src/main.c src/prompt.c src/input.c src/parser.c
OUT     := shell.out

# Default target: builds the executable.
all: $(OUT)

# Link the object files to create the final executable
$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

# Clean up build files
clean:
	rm -f $(OUT)

# Declare targets that are not files
.PHONY: all clean