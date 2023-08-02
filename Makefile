CFLAGS ::= -Wall -Wextra -Wshadow -Wpedantic

le: le.c

clean:
	find . -maxdepth 1 ! -name 'Makefile' ! -name '*.md' ! -name 'le.c' -type f -exec rm -v {} +


.PHONY: clean
