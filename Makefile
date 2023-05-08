CC=gcc

server: server.c
	$(CC) -o server server.c -Wall -lm

subscriber: subscriber.c
	$(CC) -o subscriber subscriber.c -Wall -lm

clean:
	rm -f *.o server subscriber