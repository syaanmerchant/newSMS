CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC = src/main.c src/core.c src/io.c src/gui.c
OBJ = $(SRC:.c=.o)

all: medmate

medmate: $(OBJ)
	$(CC) -o medmate $(OBJ)

clean:
	rm -f $(OBJ) medmate
