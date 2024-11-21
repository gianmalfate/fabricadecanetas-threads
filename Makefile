CC = gcc
CFLAGS = -Wno-everything -pthread -Wno-unused-result

all:
	$(CC) main.c $(CFLAGS) -o main
	
run: all
	./main

clean:
	rm -f main
