all: client clientUDP server serverUDP netperf_server netperf_client

client:
	g++ -o client client.c
clientUDP:
	g++ -o clientUDP clientUDP.c
netperf_server:	
	g++ -o netperf_server netperf_server.c
netperf_client:	
	g++ -o netperf_client netperf_client.c
server:
	g++ -o server server.c
serverUDP:	
	g++ -o serverUDP serverUDP.c -lpthread

	
clean:
	rm -rf  *o client clientUDP server serverUDP netperf_server netperf_client
