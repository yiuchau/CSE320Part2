CFLAGS = -Wall -Werror
BINS: server client

all: $(BINS)

server: server.c csapp.o
	gcc $(CFLAGS) -pthread -o server server.c csapp.o

client:	client.o csapp.o
	gcc $(CFLAGS) -pthread -o client client.c csapp.o

csapp.o: csapp.c
	gcc -c csapp.c

clean:
	rm -f *.o server client