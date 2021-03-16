	
all: sendfile.cpp
	gcc -o sendfile sendfile.cpp -lstdc++ -w 2>build
	clear