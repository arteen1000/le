CFLAGS ::= -Wall -Wextra -Wshadow -Wpedantic

le: le.c

clean:
	-rm le	

.PHONY: clean
