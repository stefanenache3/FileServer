.PHONE:clean
CC = gcc
CFLAGS = -Wall -Wextra -g 
all: server client

server: Server/FileServer.c Prot.c
	$(CC) $(CFLAGS) -pthread -o $@ $^
client: Client/FTClient.c  Prot.c
	$(CC) $(CFLAGS) -o $@ $^
	
	
clean: 
	rm server client
