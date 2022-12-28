CC = gcc
CC=gcc
CFLAGS=-std=c99 -O2
TARGET=battle player

all: $(TARGET)
	
battle: battle.c
	$(CC) -o $@ $^ $(CFLAGS)

player: player.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(TARGET)
	rm *.fifo
	rm log*