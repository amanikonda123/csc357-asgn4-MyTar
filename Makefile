CC = gcc
CFLAGS = -Wall -Werror -g -pedantic -std=c99 -std=gnu99

all: prog

prog: mytar

mytar: mytar.o create.o list.o extract.o utility.o
	$(CC) -o mytar $(CFLAGS) mytar.o create.o list.o extract.o utility.o

mytar.o: mytar.c mytar.h
	$(CC) $(CFLAGS) -c mytar.c

create.o: create.c create.h
	$(CC) $(CFLAGS) -c create.c

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c

extract.o: extract.c extract.h
	$(CC) $(CFLAGS) -c extract.c

utility.o: utility.c utility.h
	$(CC) $(CFLAGS) -c utility.c

clean: prog
	rm -f *.o *~

test: prog
	~pn-cs357/demos/tryAsgn4