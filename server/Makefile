server.out: src/main.o
	g++-14 -std=c++23 src/main.o -o server.out -lfmt -lboost_program_options

src/main.o: src/main.cpp
	g++-14 -std=c++23 -c src/main.cpp -o src/main.o 

src/main.cpp:

clean:
	rm server.out
	rm src/main.o
