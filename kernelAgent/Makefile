all: 
	g++ -o  RAW_SOCKET  RAW_SOCKET.cc
	g++ -o  RECEIVE_RAW RECEIVE_RAW.cc
	gcc -o nfqnl_test nfqnl_test.c  -lnfnetlink -lnetfilter_queue
clean:
	rm -rf RAW_SOCKET RECEIVE_RAW  nfqnl_test  *.o
