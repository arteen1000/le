CFLAGS := -std=c99 -Wall -Wextra -Wshadow -Wpedantic

le: le.c
	$(CC) $(CFLAGS) le.c -o le

clean:
	find . -maxdepth 1 ! -name 'Makefile' ! -name '*.md' ! -name 'le.c' -type f -exec rm -v {} +

.PHONY: clean
