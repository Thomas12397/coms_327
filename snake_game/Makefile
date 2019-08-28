all: snake

snake: snake_game.o
	g++ -g -Wall snake_game.o -o snake -lncurses

snake_game.o: snake_game.cpp
	g++ -g -Wall snake_game.cpp -c

clean:
	rm -f snake snake_game.o Makefile~ snake_game.cpp~
