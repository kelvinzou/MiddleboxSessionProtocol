all: client clientUDP server serverUDP

client:
	g++ -o client client.c
clientUDP:
	g++ -o clientUDP clientUDP.c

server:
	g++ -o server server.c
serverUDP:	
	g++ -o serverUDP serverUDP.c -lpthread

	
clean:
	rm -rf  *o client clientUDP server serverUDP
