all: client clientUDP server serverUDP

client:
	g++ -o client client.c
clientUDP:
	gcc -o clientUDP clientUDP.c

server:
	g++ -o server server.c
serverUDP:	
	gcc -o serverUDP serverUDP.c

	
clean:
	rm -rf  *o client clientUDP server serverUDP
