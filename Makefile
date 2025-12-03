CC = gcc

# normal compile flags
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags gtk+-3.0`

# normal linker flags
LDFLAGS = `pkg-config --libs gtk+-3.0`

SRC = src/main.c src/core.c src/io.c src/gui.c
OBJ = $(SRC:.c=.o)
TXT = data/meds_export.csv data/meds_test.csv data/meds_test.txt data/meds.txt

# coverage flags (added on top of cflags/ldflags)
COVERAGE_FLAGS = -O0 -g --coverage

PYTHON = python3

all: medmate

medmate: $(OBJ)
	$(CC) -o medmate $(OBJ) $(LDFLAGS)

# generic compile rule
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TXT) medmate *.gcov src/*.gcno src/*.gcda

coverage: clean
	$(MAKE) CFLAGS="$(CFLAGS) $(COVERAGE_FLAGS)" \
	        LDFLAGS="$(LDFLAGS) $(COVERAGE_FLAGS)" \
	        medmate

	# run cli paths and self-tests to exercise core/io
	./medmate --help || true
	./medmate --selftest || true

	# generate coverage reports for all source files
	gcov -o src $(SRC)
