CC = gcc
CFLAGS = -Wall -O3
LIB = -pthread -lm

all: tserver.o pserver.o lib.o mylib.o
	$(CC) $(CFLAGS) tserver.o lib.o mylib.o -o tserver $(LIB)
	$(CC) $(CFLAGS) pserver.o lib.o mylib.o -o pserver $(LIB)

thread: tserver.o lib.o mylib.o
	$(CC) $(CFLAGS) tserver.o lib.o mylib.o -o tserver $(LIB)

process: pserver.o lib.o mylib.o
	$(CC) $(CFLAGS) pserver.o lib.o mylib.o -o pserver $(LIB)

tserver.o: server.c
	$(CC) $(LIB) -c -D THREAD server.c -o tserver.o

pserver.o: server.c 
	$(CC) $(LIB) -c -D PROCESS server.c -o pserver.o

lib.o: lib.c
	$(CC) -c lib.c

mylib.o: mylib.c
	$(CC) -c mylib.c

clean:
	rm -rf *.out *.o *server
