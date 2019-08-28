all: main

main: dungeon.o
	g++ -g -Wall dungeon.o -o main -lncurses

dungeon.o: dungeon.cpp
	g++ -g -Wall dungeon.cpp -c

clean:
	rm -f main dungeon.o Makefile~ dungeon.cpp~ monster.hpp~ pc.hpp~ object.hpp~ room.hpp~
