all: MiddleboxAgent updateNotify Buffer_packet MidBoxOrderPreserving notifyOrderPreserving

updateNotify:
	g++ -o updateNotify updateNotify.c  
MiddleboxAgent:
	g++ -o MiddleboxAgent MiddleboxAgent.c -lpthread
Buffer_packet:
	g++ -o Buffer_packet Buffer_packet.c  
MidBoxOrderPreserving:
	g++ -o MidBoxOrderPreserving MidBoxOrderPreserving.c   -lpthread 
updateNotify:
	g++ -o updateNotify updateNotify.c   
notifyOrderPreserving:
	g++ -o	notifyOrderPreserving notifyOrderPreserving.c
clean:
	rm -rf  *o MiddleboxAgent updateNotify Buffer_packet notifyOrderPreserving MidBoxOrderPreserving
