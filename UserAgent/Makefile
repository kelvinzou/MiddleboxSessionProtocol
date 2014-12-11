all: MiddleboxAgent updateNotify Buffer_packet

updateNotify:
	g++ -o updateNotify updateNotify.c  
MiddleboxAgent:
	g++ -o MiddleboxAgent MiddleboxAgent.c -lpthread
Buffer_packet:
	g++ -o Buffer_packet Buffer_packet.c  
    
clean:
	rm -rf  *o MiddleboxAgent updateNotify Buffer_packet
