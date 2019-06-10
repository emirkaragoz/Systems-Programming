FLAGS= -Wall -ansi -pedantic -errors -std=gnu99

all: server client

server: server.o
	gcc -o Banka server.o
server.o: server.c 
	gcc -c ${FLAGS} server.c

client: client.o
	gcc -o Client client.o
client.o: client.c 
	gcc -c ${FLAGS} client.c

clean:
	rm -rf *.o server client Banka Client
