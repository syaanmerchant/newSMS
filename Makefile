CC = gcc
COVFLAGS = --coverage

# Normal compile flags
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags gtk+-3.0`

# Normal linker flags
LDFLAGS = `pkg-config --libs gtk+-3.0`

SRC = src/main.c src/core.c src/io.c src/gui.c
OBJ = $(SRC:.c=.o)

# For coverage builds
COVERAGE_FLAGS = -O0 -g --coverage   # or: -fprofile-arcs -ftest-coverage

CFLAGS = $(CFLAGS_BASE)
LDFLAGS = $(LDFLAGS_BASE)

all: medmate

medmate: $(OBJ)
	$(CC) -o medmate $(OBJ) $(LDFLAGS)

# Generic compile rule
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Simple test run we can use for coverage
test: medmate
	./medmate --help

coverage: clean
	$(MAKE) CFLAGS="$(CFLAGS_BASE) --coverage" LDFLAGS="$(LDFLAGS_BASE) --coverage" medmate
	./medmate --help
	gcov -o src src/main.c src/core.c src/io.c src/gui.c

clean:
	rm -f $(OBJ) medmate *.gcov src/*.gcda src/*.gcno

# ===== Coverage target =====
coverage: clean
	# Rebuild everything with coverage instrumentation
	$(MAKE) CFLAGS="$(CFLAGS) $(COVERAGE_FLAGS)" \
	        LDFLAGS="$(LDFLAGS) $(COVERAGE_FLAGS)" medmate

	# Run the program at least once to generate .gcda
	./medmate --help

	# Run gcov on all source files
	gcov -o src src/main.c src/core.c src/io.c src/gui.c
