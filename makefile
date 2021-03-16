	
all: swp.cpp
	gcc -o swp swp.cpp -lstdc++ -w 2>build
	clear