/* This program finds the solution to the problem of placing
 * a queen on an 8x8 board and prints the result to the console.
 * There are a total of 92 solutions and will be in the following
 * format: a2b4c6d8e3f1g7h5, where a-h is left-to-right and
 * 1-8 is bottom-to-top.
 */

#include<stdio.h>

// Prints the solution from the integer board to the position board.
void printSolution(int board[8][8], char * pos[8][8])
{
  int i, j;
  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
      if (board[i][j]) {
	printf("%s", pos[i][j]);
      }
    }
  printf("\n");
}

/* This function checks if a queen at row, col can be placed
without any conflict. This means it cannot share a space with
another queen that is on the same row, col, or either diagonal.
*/
int isSafe(int board[8][8], int row, int col)
{
	int i, j;

	//Check the row
	for (i = 0; i < col; i++)
		if (board[row][i])
			return 0;

	//Check the / diagonal
	for (i=row, j=col; i>=0 && j>=0; i--, j--)
		if (board[i][j])
			return 0;

	//Check the \ diagonal
	for (i=row, j=col; j>=0 && i<8; i++, j--)
		if (board[i][j])
			return 0;
        
        //It is safe
	return 1;
}

/* This recursive function places a '1',which represents a queen,
on the board if it does not touch another queen's territory, then uses 
a backtracking algorithm to print all of the solutions.
*/
void solve8QRec(int board[8][8], char * pos[8][8], int col)
{
        int i;
        
        //Base case: 8 queens have been placed, print solution.
	if (col == 8)
	       printSolution(board, pos);
        
        //Go through the rows
	for (i = 0; i < 8; i++)
	{
	        //If safe, place a queen and go to the next col
		if (isSafe(board, i, col))
		{
			board[i][col] = 1;

                        //Recursion
			solve8QRec(board, pos, col + 1);

                        //Backtrack
		       	board[i][col] = 0;
		}
	}
}

/* This function creates the two boards being used to solve the
problem. One is an integer board while the other is the positions
of the intger board. The integer board will get filled with a 1 
to signify a queen.
*/
void solve8Q()
{
	int board[8][8] = { 
	   {0, 0, 0, 0, 0, 0, 0, 0},
	   {0, 0, 0, 0, 0, 0, 0, 0},
	   {0, 0, 0, 0, 0, 0, 0, 0},
	   {0, 0, 0, 0, 0, 0, 0, 0},
	   {0, 0, 0, 0, 0, 0, 0, 0},
	   {0, 0, 0, 0, 0, 0, 0, 0},
	   {0, 0, 0, 0, 0, 0, 0, 0},
	   {0, 0, 0, 0, 0, 0, 0, 0},
	};

	char * boardSolPos[8][8] = { 
	   {"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"},
	   {"a7", "b7", "c7", "d7", "e7", "f7", "g7", "g8"},
	   {"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6"},
	   {"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5"},
	   {"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4"},
	   {"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3"},
	   {"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2"},
	   {"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"},
	};

	solve8QRec(board, boardSolPos, 0);
}

int main(int argc, char * argv[])
{
	solve8Q();
	return 0;
}
