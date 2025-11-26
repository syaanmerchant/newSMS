CC = gcc

# Normal compile flags
CFLAGS = -Wall -Wextra -Iinclude `pkg-config --cflags gtk+-3.0`

# Normal linker flags
LDFLAGS = `pkg-config --libs gtk+-3.0`

SRC = src/main.c src/core.c src/io.c src/gui.c
OBJ = $(SRC:.c=.o)

# Coverage flags (added ON TOP of CFLAGS/LDFLAGS)
COVERAGE_FLAGS = -O0 -g --coverage

all: medmate

medmate: $(OBJ)
	$(CC) -o medmate $(OBJ) $(LDFLAGS)

# Generic compile rule
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)  *.gcov src/*.gcno src/*.gcda data/meds_export.csv data/meds.txt

coverage: clean
	$(MAKE) CFLAGS="$(CFLAGS) $(COVERAGE_FLAGS)" \
	        LDFLAGS="$(LDFLAGS) $(COVERAGE_FLAGS)" \
	        
	./medmate --help || true

	gcov -o src src/main.c src/core.c src/io.c src/gui.c
