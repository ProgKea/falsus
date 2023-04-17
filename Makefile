CC?=clang
CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb
LIBS=
SRC=src/main.c src/falsus.c

falsus: $(SRC)
	$(CC) $(CFLAGS) -o falsus $(SRC) $(LIBS)
