CC = gcc
CFLAGS = -Wall

all: server client tunnel

server: server.c rsa.c aes.c
	$(CC) $(CFLAGS) server.c rsa.c aes.c -o server

client: client.c rsa.c aes.c
	$(CC) $(CFLAGS) client.c rsa.c aes.c -o client

tunnel: tunnel.c
	$(CC) $(CFLAGS) tunnel.c -o tunnel

clean:
	rm -f server client tunnel alinan_*