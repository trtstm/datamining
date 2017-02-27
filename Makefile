all: main

main: Apriori.o
	g++ Apriori.o -o apriori
	
Apriori.o: Apriori.cpp Apriori.hpp
	g++ Apriori.cpp -c -g