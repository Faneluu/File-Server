.PHONY: all clean

all: final

final: server.o operations.o client.c
	@gcc server.o operations.o -lpthread -o server
	@gcc client.c -o client

operations.o: operations.c
	@gcc -c operations.c -o operations.o

server.o: server.c
	@gcc -c server.c -o server.o

clean: 
	@rm -f server client *.o
