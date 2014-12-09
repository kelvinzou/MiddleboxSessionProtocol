all: MiddleboxAgent updateNotify

updateNotify:
	g++ -o updateNotify updateNotify.c  
MiddleboxAgent:
	g++ -o MiddleboxAgent MiddleboxAgent.c -lpthread

    
clean:
	rm -rf  *o MiddleboxAgent updateNotify
