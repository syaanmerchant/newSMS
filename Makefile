CC = gcc

# Coverage flags (for gcov)
COVERAGE_FLAGS = -O0 -g --coverage

# Compile flags:
#  -Wall -Wextra : show useful warnings
#  -Iinclude     : look for headers in include/
#  pkg-config    : add GTK include flags
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags gtk+-3.0`

# Linker flags: use pkg-config to link against GTK libs
LDFLAGS = `pkg-config --libs gtk+-3.0`

SRC = src/main.c src/core.c src/io.c src/gui.c
OBJ = $(SRC:.c=.o)

# Allow C code to call python script
PYTHON = python3

all: medmate

medmate: $(OBJ)
	$(CC) -o medmate $(OBJ) $(LDFLAGS)

# Generic compile rule
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) medmate

coverage: clean
	$(MAKE) CFLAGS="$(CFLAGS) $(COVERAGE_FLAGS)" \
	        LDFLAGS="$(LDFLAGS) $(COVERAGE_FLAGS)" \
	        medmate

	./medmate --help || true

	gcov -o src src/main.c src/core.c src/io.c src/gui.c
