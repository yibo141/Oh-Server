Oh-Server: httpserver.o http_process.o http_parser.o locker.o
	g++ -pthread -std=c++11 -o Oh-Server httpserver.o http_process.o http_parser.o locker.o
	
httpserver.o: httpserver.cpp http_parser.h http_process.h threadpool.h
	g++ -pthread -c httpserver.cpp
	
http_process.o: http_process.cpp http_parser.h
	g++ -std=c++11 -c http_process.cpp
	
http_parser.o: http_parser.cpp http_parser.h
	g++ -std=c++11 -c http_parser.cpp
	
locker.o: locker.cpp locker.h
	g++ -pthread -c locker.cpp
	
clean:
	rm *.o Oh-Server
