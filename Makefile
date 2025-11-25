CC = gcc
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`

SRC = src/main.c src/core.c src/gui.c src/io.c
OBJ = $(SRC:.c=.o)

all: medmate

medmate: $(OBJ)
	$(CC) -o medmate $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) medmate
