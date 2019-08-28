#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <endian.h>
#include <limits.h>
#include <ncurses.h>

#define DUNGEON_X              80
#define DUNGEON_Y              21
#define ROOMS                  5
#define ROOM_MIN_X             5 //Max: 8
#define ROOM_MIN_Y             4 //Max: 9
#define MONSTERS               10

struct Room {
  int xsize;
  int ysize;
  int xpos;
  int ypos;
};

struct PC {
  int xpos;
  int ypos;
  char lastPos;
  int speed;
  int priority;
  int score;
};

struct Monster {
  int canTunnel;
  char lastPos;
  int xpos;
  int ypos;
  int speed;
  int priority;
  int score;
};

/**
 *  Global Variables
 */
char dungeon[DUNGEON_Y][DUNGEON_X];
int hardnessMap[DUNGEON_Y][DUNGEON_X];
struct Room rooms[ROOMS];
struct PC pc;
struct Monster monsters[MONSTERS];
int canTunnelPath[DUNGEON_Y][DUNGEON_X];
int cantTunnelPath[DUNGEON_Y][DUNGEON_X];
int score[MONSTERS+1];
int gameOver;
int quit;
int displayMonsterList;
int moveScroll;

/**
 *  Function list
 */
void initDungeon(char board[DUNGEON_Y][DUNGEON_X]);
void swap(struct Room *x, struct Room *y);
void shuffle(struct Room rooms[ROOMS]);
void genDungeon(char board[DUNGEON_Y][DUNGEON_X]);
void genHardness(int hardness[DUNGEON_Y][DUNGEON_X]);
void printDungeon(char board[DUNGEON_Y][DUNGEON_X]);
void printHardness(int hardness[DUNGEON_Y][DUNGEON_X]);

void load_state(char board[DUNGEON_Y][DUNGEON_X], int hardness[DUNGEON_Y][DUNGEON_X]);
void save_state(char board[DUNGEON_Y][DUNGEON_X], int hardness[DUNGEON_Y][DUNGEON_X]);

void genPC(char board[DUNGEON_Y][DUNGEON_X]);
void recCanTunnelPath(int y, int x, int val, int hardness[DUNGEON_Y][DUNGEON_X]);
void genCanTunnelPath();
void printCanTunnelPath();
void recCantTunnelPath(int y, int x, int val, int hardness[DUNGEON_Y][DUNGEON_X]);
void genCantTunnelPath();
void printCantTunnelPath();

void randMovePC(char board[DUNGEON_Y][DUNGEON_X]);
void initMonsters(struct Monster monsters[MONSTERS]);
int isValidRoom(int mon_x, int mon_y);
void moveMonster(int order);
void initScore();
void makeMoveRandPC();

void initTerminal();
void genStairs(int count);
void moveMonsterList();
void keyboardListener();
void movePC(int x, int y);
void printMonsterList(int move);
void makeMove();
void printDungeonToTerminal(char board[DUNGEON_Y][DUNGEON_X]);

int main(int argc, char * argv[]);

/**
 *  This function generates the border around the dungeon board and fills
 *  the rest with spaces (' ')
 */
void initDungeon(char board[DUNGEON_Y][DUNGEON_X])
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      
      if (i == 0 || i == DUNGEON_Y - 1)
	board[i][j] = '-';
      else if (j == 0 || j == DUNGEON_X - 1)
	board[i][j] = '|';
      else
	board[i][j] = ' ';
    }
  }
}

/**
 *  Swaps one room with another. Used for the shuffle function
 */
void swap(struct Room *x, struct Room *y)
{
  struct Room tmp;
  tmp = *x;
  *x = *y;
  *y = tmp;
}

/**
 *  Shuffles the rooms around
 */
void shuffle(struct Room rooms[ROOMS])
{  
  swap(&rooms[0], &rooms[3]);
  swap(&rooms[1], &rooms[0]);
  swap(&rooms[4], &rooms[2]);
}

/**
 *  This function generates the rooms and corridors. After generating them,
 *  it puts them onto the dungeon board. It also randomly places a PC on
 *  a room or corridor.
 */
void genDungeon(char board[DUNGEON_Y][DUNGEON_X])
{
  srand(time(NULL));
  
  //Randomly generates the room positions and sizes
  //Largest case: x: 6 + 5(8) + 4(8) = 78
  //            y: 9 + 10 = 19
  int i, lastx;
  rooms[0].xpos = (rand() % 5) + 2; //2-6
  for (i = 0; i < ROOMS; i++) {
    
    if (i != 0) {
      lastx = rooms[i-1].xpos + rooms[i-1].xsize;
      rooms[i].xpos = lastx + (rand() % 5) + 4; //Lastx + 4-8
    }    
    rooms[i].ypos = (rand() % 9) + 2; //2-10
    rooms[i].xsize = (rand() % 4) + ROOM_MIN_X; //5-8
    rooms[i].ysize = (rand() % 6) + ROOM_MIN_Y; //4-9
  }
  
  if (ROOMS==5)
    shuffle(rooms);
  
  //Puts the rooms on the board
  int row, col;
  for (i = 0; i < ROOMS; i++)
    for (row = rooms[i].ypos; row < rooms[i].ypos+rooms[i].ysize; row++)
      for (col = rooms[i].xpos; col < rooms[i].xpos+rooms[i].xsize; col++) 
	board[row][col] = '.';

  //Generates the corridors
  int curx, cury, nextx, nexty;
  for (i = 0; i < ROOMS-1; i++) {
    curx = rooms[i].xpos;
    cury = rooms[i].ypos;
    nextx = rooms[i+1].xpos;
    nexty = rooms[i+1].ypos;

    int j, xdir, ydir;
    if (abs(nextx - curx)) {

      xdir = (nextx-curx)/abs(nextx-curx);

      for (j = curx; j != nextx; j += xdir) 
	if (board[cury][j] != '.')
	  board[cury][j] = '#';
    }
    if (abs(nexty - cury)) {

      ydir = (nexty-cury)/abs(nexty-cury);

      for (j = cury; j != nexty; j += ydir)
	if (board[j][nextx] != '.')
	  board[j][nextx] = '#';
    }
  }
}

/**
 *  Generates the hardness for the dungeon board
 */
void genHardness(int hardness[DUNGEON_Y][DUNGEON_X])
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      
      if (i == 0 || i == DUNGEON_Y - 1 || j == 0 || j == DUNGEON_X - 1)
	hardness[i][j] = 255;
      else if (dungeon[i][j] == '.' || dungeon[i][j] == '#')
	hardness[i][j] = 0;
      else
	hardness[i][j] = (rand() % 254) + 1;
    }
  }
}

/**
 *  Prints the board to the terminal
 */
void printDungeon(char board[DUNGEON_Y][DUNGEON_X])
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      printf("%c", board[i][j]);
    }
    printf("\n");
  }

  if (gameOver)
    printf("RIP PC. You were brutally killed in %d turns. Game over :(\n", pc.score / 100 - 1);
}

/**
 *  Prints the hardness to the terminal. Used for testing purposes
 */
void printHardness(int hardness[DUNGEON_Y][DUNGEON_X])
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      printf("%d", hardness[i][j] % 10);
    }
    printf("\n");
  }
}

/**
 *  Loads the state of the dungeon and hardness from a file
 */
void load_state(char board[DUNGEON_Y][DUNGEON_X], int hardness[DUNGEON_Y][DUNGEON_X])
{
  FILE *f;
  char *path;
  char *home = getenv("HOME");
  char semantic[12];
  int version, size, length,*s, *v, i, j;

  //Gets the address and reads the file
  s = &size;
  v = &version;
  length = strlen(home) + strlen("/.rlg327/dungeon") + 1;
  path = malloc(length);
  strcpy(path, home);
  strcat(path, "/.rlg327/dungeon");
  f = fopen(path, "r");
  free(path);

  //Read in semantic, version, and size of file
  fread(&semantic, 12, 1, f);
  fread(v, 4, 1, f);
  version = be32toh(version);
  fread(s, 4, 1, f);
  size = be32toh(size);
  int numRooms = (*s-1700) / 4;

  //Read in the hardness. Makes rooms and corridors a '#'
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      
      fread(&hardness[i][j], 1, 1, f);
      
      if (hardness[i][j] == 255 && i == 0 || hardness[i][j] == 255 && i == DUNGEON_Y - 1)
	board[i][j] = '-';
      else if (hardness[i][j] == 255 && j == 0 || hardness[i][j] == 255 && j == DUNGEON_X - 1)
	board[i][j] = '|';
      else if (hardness[i][j] == 0)
	board[i][j] = '#';
      else
	board[i][j] = ' ';
    }
  }

  //Read in the rooms
  for (i = 0; i < numRooms; i++) {
    fread(&rooms[i].xpos, 1, 1, f);
    fread(&rooms[i].ypos, 1, 1, f);
    fread(&rooms[i].xsize, 1, 1, f);
    fread(&rooms[i].ysize, 1, 1, f);
  }

  //Change '#' to '.' for the room locations
  int row, col;
  for (i = 0; i < numRooms; i++) 
    for (row = rooms[i].ypos; row < rooms[i].ypos+rooms[i].ysize; row++) 
      for (col = rooms[i].xpos; col < rooms[i].xpos+rooms[i].xsize; col++) 
	board[row][col] = '.';

  fclose(f);
}

/**
 *  This method saves the state of the dungeon and the hardness map associated with it.
 *  It will write the necessary data to a file
 */
void save_state(char board[DUNGEON_Y][DUNGEON_X], int hardness[DUNGEON_Y][DUNGEON_X])
{
  FILE *f;

  //Gets the directory address for save file
  char *home = getenv("HOME");
  int length = strlen(home) + strlen("/.rlg327/dungeon") + 1;
  char *path = malloc(length);
  strcpy(path, home);
  strcat(path, "/.rlg327/dungeon");
  printf("Saved to path: %s\n", path);

  //Open the file
  f = fopen(path, "w");
  free(path);

  char semantic[] = "RLG327-s2018";
  fwrite(semantic, 12, 1, f);
  
  //Gets the version
  int version = 0;
  int be;
  be = htobe32(version);
  fwrite(&be, sizeof(int), 1, f);

  //Saves the size of the file
  int size = 12 + 4 + 4 + DUNGEON_X*DUNGEON_Y + 4*ROOMS;
  be = htobe32(size);
  fwrite(&be, sizeof(int), 1, f);

  //Saves the hardness
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++)
    for (j = 0; j < DUNGEON_X; j++)
      fwrite(&hardness[i][j], 1, 1, f);

  //Saves the dungeon rooms
  for (i = 0; i < ROOMS; i++) {  
    fwrite(&rooms[i].xpos, 1, 1, f);
    fwrite(&rooms[i].ypos, 1, 1, f);
    fwrite(&rooms[i].xsize, 1, 1, f);
    fwrite(&rooms[i].ysize, 1, 1, f);
  }
  
  fclose(f);
}

/**
 *  Generates the PC randomly into a room on the dungeon.
 *  Uses recursion to find a suitable spot. Sets the PC's x, y,, speed, score, and priority.
 */
void genPC(char board[DUNGEON_Y][DUNGEON_X])
{
  int pc_x, pc_y;

  pc_x = rand() % DUNGEON_X;
  pc_y = rand() % DUNGEON_Y;
  
  if (board[pc_y][pc_x] == '.') {
    pc.lastPos = board[pc_y][pc_x];
    pc.xpos = pc_x;
    pc.ypos = pc_y;
    pc.speed = 10;
    pc.score = 1000/pc.speed;
    pc.priority = 1000/pc.speed;
    board[pc_y][pc_x] = '@';
    hardnessMap[pc_y][pc_x] = 0;
    return;
  }
  else
    genPC(board);
}

/**
 *  Generates recursively the path for monsters that can tunnel
 */
void recCanTunnelPath(int y, int x, int val, int hardness[DUNGEON_Y][DUNGEON_X])
{
  
  if (canTunnelPath[y+1][x] > val + (hardness[y+1][x]/85)+1 && hardness[y+1][x] != 255) {
    canTunnelPath[y+1][x] = val + (hardness[y+1][x]/85)+1;
    recCanTunnelPath(y+1, x, canTunnelPath[y+1][x], hardness);
  }
  if (canTunnelPath[y-1][x] > val + (hardness[y-1][x]/85)+1 && hardness[y-1][x] != 255) {
    canTunnelPath[y-1][x] = val + (hardness[y-1][x]/85)+1;
    recCanTunnelPath(y-1, x, canTunnelPath[y-1][x], hardness);
  }
  if (canTunnelPath[y][x+1] > val + (hardness[y][x+1]/85)+1 && hardness[y][x+1] != 255) {
    canTunnelPath[y][x+1] = val + (hardness[y][x+1]/85)+1;
    recCanTunnelPath(y, x+1, canTunnelPath[y][x+1], hardness);
  }
  if (canTunnelPath[y][x-1] > val + (hardness[y][x-1]/85)+1 && hardness[y][x-1] != 255) {
    canTunnelPath[y][x-1] = val + (hardness[y][x-1]/85)+1;
    recCanTunnelPath(y, x-1, canTunnelPath[y][x-1], hardness);
  }
  if (canTunnelPath[y+1][x+1] > val + (hardness[y+1][x+1]/85)+1 && hardness[y+1][x+1] != 255) {
    canTunnelPath[y+1][x+1] = val + (hardness[y+1][x+1]/85)+1;
    recCanTunnelPath(y+1, x+1, canTunnelPath[y+1][x+1], hardness);
  }
  if (canTunnelPath[y+1][x-1] > val + (hardness[y+1][x-1]/85)+1 && hardness[y+1][x-1] != 255) {
    canTunnelPath[y+1][x-1] = val + (hardness[y+1][x-1]/85)+1;
    recCanTunnelPath(y+1, x-1, canTunnelPath[y+1][x-1], hardness);
  }
  if (canTunnelPath[y-1][x+1] > val + (hardness[y-1][x+1]/85)+1 && hardness[y-1][x+1] != 255) {
    canTunnelPath[y-1][x+1] = val + (hardness[y-1][x+1]/85)+1;
    recCanTunnelPath(y-1, x+1, canTunnelPath[y-1][x+1], hardness);
  }
  if (canTunnelPath[y-1][x-1] > val + (hardness[y-1][x-1]/85)+1 && hardness[y-1][x-1] != 255) {
    canTunnelPath[y-1][x-1] = val + (hardness[y-1][x-1]/85)+1;
    recCanTunnelPath(y-1, x-1, canTunnelPath[y-1][x-1], hardness);
  }
}

/**
 *  Initializes the path for monsters that can tunnel. Calls the recursive function above.
 */
void genCanTunnelPath()
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      canTunnelPath[i][j] = INT_MAX;
      if (hardnessMap[i][j] == 255)
	canTunnelPath[i][j] = ' ';
    }
  }

  recCanTunnelPath(pc.ypos, pc.xpos, 0, hardnessMap);
  canTunnelPath[pc.ypos][pc.xpos] = '@';
}

/**
 *  Prints the path for monsters that can tunnel
 */
void printCanTunnelPath()
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      if (hardnessMap[i][j] == 255 || pc.ypos == i && pc.xpos == j)
	printf("%c", canTunnelPath[i][j]);
      else
	printf("%d", canTunnelPath[i][j] % 10);
    }
    printf("\n");
  }
}

/**
 *  Generates recursively the path for monsters that can't tunnel
 */
void recCantTunnelPath(int y, int x, int val, int hardness[DUNGEON_Y][DUNGEON_X])
{
  if (cantTunnelPath[y+1][x] > val+1 && hardness[y+1][x] == 0) {
    cantTunnelPath[y+1][x] = val+1;
    recCantTunnelPath(y+1, x, val+1, hardness);
  }
  if (cantTunnelPath[y-1][x] > val+1 && hardness[y-1][x] == 0) {
    cantTunnelPath[y-1][x] = val+1;
    recCantTunnelPath(y-1, x, val+1, hardness);
  }
  if (cantTunnelPath[y][x+1] > val+1 && hardness[y][x+1] == 0) {
    cantTunnelPath[y][x+1] = val+1;
    recCantTunnelPath(y, x+1, val+1, hardness);
  }
  if (cantTunnelPath[y][x-1] > val+1 && hardness[y][x-1] == 0) {
    cantTunnelPath[y][x-1] = val+1;
    recCantTunnelPath(y, x-1, val+1, hardness);
  }
  if (cantTunnelPath[y+1][x+1] > val+1 && hardness[y+1][x+1] == 0) {
    cantTunnelPath[y+1][x+1] = val+1;
    recCantTunnelPath(y+1, x+1, val+1, hardness);
  }
  if (cantTunnelPath[y+1][x-1] > val+1 && hardness[y+1][x-1] == 0) {
    cantTunnelPath[y+1][x-1] = val+1;
    recCantTunnelPath(y+1, x-1, val+1, hardness);
  }
  if (cantTunnelPath[y-1][x+1] > val+1 && hardness[y-1][x+1] == 0) {
    cantTunnelPath[y-1][x+1] = val+1;
    recCantTunnelPath(y-1, x+1, val+1, hardness);
  }
  if (cantTunnelPath[y-1][x-1] > val+1 && hardness[y-1][x-1] == 0) {
    cantTunnelPath[y-1][x-1] = val+1;
    recCantTunnelPath(y-1, x-1, val+1, hardness);
  }
}

/**
 *  Initializes the path for monsters that can't tunnel. Calls the recursive function above.
 */
void genCantTunnelPath()
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      cantTunnelPath[i][j] = -1;
      if (hardnessMap[i][j] == 0)
	cantTunnelPath[i][j] = INT_MAX;
    }
  }

  recCantTunnelPath(pc.ypos, pc.xpos, 0, hardnessMap);
  cantTunnelPath[pc.ypos][pc.xpos] = '@';
}

/**
 *  Prints the path for monsters that can't tunnel
 */
void printCantTunnelPath()
{
  int i, j;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      if (cantTunnelPath[i][j] == -1) {
	cantTunnelPath[i][j] = ' ';
	printf("%c", cantTunnelPath[i][j]);
      }
      else if (cantTunnelPath[i][j] == '@')
	printf("%c", cantTunnelPath[i][j]);
      else
	printf("%d", cantTunnelPath[i][j] % 10);
    }
    printf("\n");
  }
}

/**
 *  Randomly moves the PC around the rooms and corridors. It can move in all 8 directions but only
 *  in rooms and corridors.
 */
void randMovePC(char board[DUNGEON_Y][DUNGEON_X])
{
  int pc_x, pc_y;
  char tempChar;

  pc_x = pc.xpos + (rand() % 3) - 1;
  pc_y = pc.ypos + (rand() % 3) - 1;
  if (board[pc_y][pc_x] == '.' || board[pc_y][pc_x] == '#') {
    tempChar = board[pc_y][pc_x];
    board[pc.ypos][pc.xpos] = pc.lastPos;
    pc.xpos = pc_x;
    pc.ypos = pc_y;
    board[pc_y][pc_x] = '@';
    pc.lastPos = tempChar;
  }
  else
    randMovePC(board);
}

/**
 *  Initializes the monsters on the dungeon board. Sets their tunneling ability, x and y
 *  positions, speed, score, and priority. Monsters that can tunnel are 'M', and monsters
 *  that can't tunnel are 'm'.
 */
void initMonsters(struct Monster monsters[MONSTERS])
{
  int i, mon_x, mon_y, valid_pos, pc_x, pc_y;

  pc_x = pc.xpos;
  pc_y = pc.ypos;
  
  for (i = 0; i < MONSTERS; i++) {
    
    monsters[i].canTunnel = rand() % 2;
    monsters[i].speed = (rand() % 16) + 5;
    monsters[i].score = 1000/monsters[i].speed;
    monsters[i].priority = 1000/monsters[i].speed;
    
    valid_pos = 0;
    while (!valid_pos) {
      mon_x = rand() % DUNGEON_X;
      mon_y = rand() % DUNGEON_Y;
  
      if (dungeon[mon_y][mon_x] == '.' && isValidRoom(mon_x, mon_y)) {
	monsters[i].lastPos = dungeon[mon_y][mon_x];
	monsters[i].xpos = mon_x;
	monsters[i].ypos = mon_y;
	if (monsters[i].canTunnel)
	  dungeon[mon_y][mon_x] = 'M';
	else
	  dungeon[mon_y][mon_x] = 'm';
	valid_pos = 1;
      }
    }
  }
}

/**
 *  This function makes it so monsters do NOT spawn in the same room as the PC. mon_x and mon_y are the
 *  monster's x and y coordinates and they get checked if they are in the same room as the PC.
 */
int isValidRoom(int mon_x, int mon_y)
{
  int i, x, y, pcRoom;

  x = pc.xpos;
  y = pc.ypos;

  for (i = 0; i < ROOMS; i++) {
    if (x >= rooms[i].xpos && x < rooms[i].xpos + rooms[i].xsize 
	&& y >= rooms[i].ypos && y < rooms[i].ypos + rooms[i].ysize) {
      pcRoom = i;
    }
  }

  if (mon_x >= rooms[pcRoom].xpos && mon_x <= rooms[pcRoom].xpos + rooms[pcRoom].xsize
      && mon_y >= rooms[pcRoom].ypos && mon_y <= rooms[pcRoom].ypos + rooms[pcRoom].ysize) {
    return 0;
  }
  return 1;
}

/**
 *  Moves a monster closer to the PC and does so in a random fashion. The parameter 'order' is used to
 *  define which monster to move in the array i.e. monsters[order].
 */
void moveMonster(int order)
{
  int x, y, lastPos, newPos, newX, newY, valid_pos;
  char tempChar;

  //If the monster can tunnel. Move accordingly.
  if (monsters[order].canTunnel) {
    x = monsters[order].xpos;
    y = monsters[order].ypos;
    lastPos = canTunnelPath[y][x];
    
    valid_pos = 0;
    while (!valid_pos) {
      newX = x + (rand() % 3) - 1;
      newY = y + (rand() % 3) - 1;
      newPos = canTunnelPath[newY][newX];

      if (dungeon[newY][newX] == 'm' || dungeon[newY][newX] == 'M') {
	return;
      }

      if (lastPos > newPos && hardnessMap[newY][newX] != 255) {
	if (hardnessMap[newY][newX] > 85) {
	  hardnessMap[newY][newX] -= 85;
	  valid_pos = 1;
	  continue;
	}
	else
	  hardnessMap[newY][newX] = 0;

	monsters[order].xpos = newX;
	monsters[order].ypos = newY;
	if (dungeon[newY][newX] == ' ')
	  tempChar = '#';
	else
	  tempChar = dungeon[newY][newX];

	dungeon[newY][newX] = 'M';
	hardnessMap[newY][newX] = 0;
	dungeon[y][x] = monsters[order].lastPos;
	monsters[order].lastPos = tempChar;
	valid_pos = 1;
      }

      if (newX==pc.xpos && newY==pc.ypos) {
	monsters[order].xpos = newX;
	monsters[order].ypos = newY;
	dungeon[newY][newX] = 'M';
	dungeon[y][x] = monsters[order].lastPos;
	gameOver = 1;
	return;
      }
    }
  }
  //Otherwise the monster can't tunnel. Move accordingly.
  else {
    x = monsters[order].xpos;
    y = monsters[order].ypos;
    lastPos = cantTunnelPath[y][x];

    valid_pos = 0;
    while (!valid_pos) {
      newX = x + (rand() % 3) - 1;
      newY = y + (rand() % 3) - 1;
      newPos = cantTunnelPath[newY][newX];

      if (dungeon[newY][newX] == 'm' || dungeon[newY][newX] == 'M')
	return;

      if (lastPos > newPos && (dungeon[newY][newX] == '#' || dungeon[newY][newX] == '.' || 
	  dungeon[newY][newX] == '>' || dungeon[newY][newX] == '<')) {

	monsters[order].xpos = newX;
	monsters[order].ypos = newY;
	tempChar = dungeon[newY][newX];
	dungeon[newY][newX] = 'm';
	hardnessMap[newY][newX] = 0;
	dungeon[y][x] = monsters[order].lastPos;
	monsters[order].lastPos = tempChar;
	valid_pos = 1;
      }

      if (newX==pc.xpos && newY==pc.ypos) {
	monsters[order].xpos = newX;
	monsters[order].ypos = newY;
	dungeon[newY][newX] = 'm';
	dungeon[y][x] = monsters[order].lastPos;
	gameOver = 1;
	return;
      }
    }
  }
}

/**
 *  Initializes the score which determines who should go next whether it's a monster or the PC.
 */
void initScore()
{
  int i;
  for (i = 0; i < MONSTERS; i++) {
    score[i] = monsters[i].score;
  }
  score[MONSTERS] = pc.score;
}

/**
 *  Makes a move for the monsters and random PC and prints the dungeon every time the PC moves.
 */
void makeMoveRandPC()
{
  int i, turn, tmpTurn, order;

  turn = score[0];
  order = 0;
  for (i = 0; i < MONSTERS; i++) {
    if (turn > score[i+1]) {
      turn = score[i+1];
      order = i+1;
    }
  }

  //Move the player
  if (order == MONSTERS) {
    randMovePC(dungeon);
    pc.score += pc.priority;
    score[order] = pc.score;
    genCanTunnelPath();
    genCantTunnelPath();
    printDungeon(dungeon);
    usleep(500000);
  }
  //Move monster
  else {
    moveMonster(order);
    monsters[order].score = monsters[order].score + monsters[order].priority;
    score[order] = monsters[order].score;
  }
}

/**
 *  Initailizes the terminal using ncurses.
 */
void initTerminal()
{
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, 1);
}

/**
 *  Generates one '>' and one '<' randomly in a room or corridor.
 */
void genStairs(int count)
{
  int stairs_x, stairs_y;

  if (count < 1)
    return;

  stairs_x = rand() % DUNGEON_X;
  stairs_y = rand() % DUNGEON_Y;
  
  if (dungeon[stairs_y][stairs_x] == '.' || dungeon[stairs_y][stairs_x] == '#') {
    if (count % 2 == 0)
      dungeon[stairs_y][stairs_x] = '>';
    else
      dungeon[stairs_y][stairs_x] = '<';
    
    genStairs(count-1);
  }
  else
    genStairs(count);
}

/**
 *  Initializes a new dungeon when the PC steps on a stair represented by '>' or '<'
 */
void initStairDungeon()
{
  initDungeon(dungeon);
  genDungeon(dungeon);
  genHardness(hardnessMap);
  genPC(dungeon);
  genStairs(4);
  genCantTunnelPath();
  genCanTunnelPath();
  initMonsters(monsters);
  initScore();
}

/**
 *  Handles input coming from the keyboard
 */
void keyboardListener()
{
  int i, pc_x, pc_y, mvPC;

  switch(getch())
    {
    case '1':              //Lower Left
      pc_x = pc.xpos - 1;
      pc_y = pc.ypos + 1;
      mvPC = 1;
      break;
    case '2':              //Down
      pc_x = pc.xpos;
      pc_y = pc.ypos + 1;
      mvPC = 1;
      break;
    case '3':              //Lower Right
      pc_x = pc.xpos + 1;
      pc_y = pc.ypos + 1;
      mvPC = 1;
      break; 
    case '4':              //Left
      pc_x = pc.xpos - 1;
      pc_y = pc.ypos;
      mvPC = 1;
      break;
    case '5':              //Center
      return;
    case '6':              //Right
      pc_x = pc.xpos + 1;
      pc_y = pc.ypos;
      mvPC = 1;
      break;
    case '7':              //Upper Left
      pc_x = pc.xpos - 1;
      pc_y = pc.ypos - 1;
      mvPC = 1;
      break;
    case '8':              //Up
      pc_x = pc.xpos;
      pc_y = pc.ypos - 1;
      mvPC = 1;
      break;
    case '9':              //Upper Right
      pc_x = pc.xpos + 1;
      pc_y = pc.ypos - 1;
      mvPC = 1;
      break;
    case 'Q':              //Quit
      quit = 1;
      return;
    case 'm':              //Display monster list
      moveScroll = 0;
      displayMonsterList = 1;
      return;
    case 27:               //Don't display monster list
      displayMonsterList = 0;
      move(0, 0);
      clrtoeol();
      mvprintw(0, 0, "Displaying Dungeon");
      for (i = 0; i < MONSTERS; i++) {
	move(i + 1, 0);
	clrtoeol();
      }
      printDungeonToTerminal(dungeon);
      keyboardListener();
      return;
    case KEY_UP:
      if (moveScroll < 0)
	moveScroll++;
      printMonsterList(moveScroll);
      keyboardListener();
      return;
    case KEY_DOWN:
      if (moveScroll > -1 * (MONSTERS - DUNGEON_Y - 2))
	moveScroll--;
      printMonsterList(moveScroll);
      keyboardListener();
      return;
    default:               //Don't do anything if incorrect key was pressed
      keyboardListener();
      return;
    }

  if (mvPC) {
    movePC(pc_x, pc_y);
    mvPC = 0;
  }
}

/**
 *  Moves the PC according to the keyboardListener() function
 */
void movePC(int x, int y)
{
  char tempChar;

  if (!displayMonsterList) {
    if (dungeon[y][x] == '>' || dungeon[y][x] == '<') {
      initStairDungeon();
    }
    else if (dungeon[y][x] == 'm') {
      dungeon[y][x] = 'm';
      dungeon[pc.ypos][pc.xpos] = pc.lastPos;
      gameOver = 1;
    }
    else if (dungeon[y][x] == 'M') {
      dungeon[y][x] == 'M';
      dungeon[pc.ypos][pc.xpos] = pc.lastPos;
      gameOver = 1;
    }
    else if (dungeon[y][x] == '.' || dungeon[y][x] == '#') {
      tempChar = dungeon[y][x];
      dungeon[pc.ypos][pc.xpos] = pc.lastPos;
      pc.xpos = x;
      pc.ypos = y;
      dungeon[y][x] = '@';
      pc.lastPos = tempChar;
    }
  }
}

/**
 *  Generates and prints the monster positions relative to the PC in ncurses terminal.
 *  The parameter 'move' is used for scrolling the list up and down.
 */
void printMonsterList(int move)
{
  int i, mon_x, mon_y, print_x, print_y;
  char mon_char;

  if (displayMonsterList) {
    
    //Clear out the dungeon
    move(0, 0);
    clrtoeol();
    for (i = 0; i < DUNGEON_Y; i++) {
      move(i + 1, 0);
      clrtoeol();
    }

    //Display the monster list
    mvprintw(0, 0, "Displaying Monsters List");
    for (i = 0; i < MONSTERS; i++) {
      mon_x = monsters[i].xpos; 
      mon_y = monsters[i].ypos;
      mon_char = dungeon[mon_y][mon_x];
      print_x = abs(mon_x - pc.xpos);
      print_y = abs(mon_y - pc.ypos);

      mvprintw(i + 1 + move, 0, "%3d. %c:", i+1,  mon_char);
      if (mon_y - pc.ypos <= 0) {
	mvprintw(i + 1 + move, 7, "%3d north", print_y);
      }
      else if (mon_y - pc.ypos > 0) {
	mvprintw(i + 1 + move, 7, "%3d south", print_y);
      }
      if (mon_x - pc.xpos <= 0) {
	mvprintw(i + 1 + move, 16, ", %2d west ", print_x);
      }
      else if (mon_x - pc.xpos > 0) {
	mvprintw(i + 1 + move, 16, ", %2d east ", print_x);
      }
    }
  }
  refresh();
}

/**
 *  Makes a move according to the monster's and PC's score. If the PC moves, the dungeon gets refreshed.
 */
void makeMove()
{
  int i, turn, order;

  //Figure out whose turn it is
  turn = score[0];
  order = 0;
  for (i = 0; i < MONSTERS; i++) {
    if (turn > score[i+1]) {
      turn = score[i+1];
      order = i+1;
    }
  }
  //Move the player
  if (order == MONSTERS || pc.score == 100) {
    keyboardListener();
    if (displayMonsterList)
      return;
    pc.score += pc.priority;
    score[order] = pc.score;
    genCanTunnelPath();
    genCantTunnelPath();
  }
  //Move monster
  else {
    moveMonster(order);
    monsters[order].score = monsters[order].score + monsters[order].priority;
    score[order] = monsters[order].score;
  }
}

/**
 *  Prints the terminal using ncurses. If the game is over,
 */
void printDungeonToTerminal(char board[DUNGEON_Y][DUNGEON_X])
{
  int i, j;

  if (!displayMonsterList) {
    
    //Clear out monster list
    move(0, 0);
    clrtoeol();
    for (i = 0; i < MONSTERS; i++) {
      move(i + 1, 0);
      clrtoeol();
    }

    //Display the dungeon
    mvprintw(0, 0, "Displaying dungeon");
    for (i = 0; i < DUNGEON_Y; i++) {
      for (j = 0; j < DUNGEON_X; j++) {
	mvaddch(i + 1, j, board[i][j]);
      }
    }
    if (gameOver) {
      move(0, 0);
      clrtoeol();
      mvprintw(0, 0, "RIP PC. You were brutally killed in %d turns. Game over :(\n", pc.score / 100 - 1);
      refresh();
      sleep(5);
    }
  }
}

/**
 *  Main function for running the dungeon and decides what to do with
 *  the different flags given in the terminal
 */
int main(int argc, char * argv[])
{
  if (argc > 4) {
    fprintf(stderr, "Too many arguments\n");
    return 1;
  }
  
  //No arguments, make and print the dungeon
  if (argc == 1) {
    initDungeon(dungeon);
    genDungeon(dungeon);
    genHardness(hardnessMap);
    printDungeon(dungeon);
  }

  //There is one argument
  if (argc == 2) {
    
    if (!strcmp(argv[1], "--load")) {
      load_state(dungeon, hardnessMap);
      printDungeon(dungeon);
    } 
    else if (!strcmp(argv[1], "--save")) {
      initDungeon(dungeon);
      genDungeon(dungeon);
      genHardness(hardnessMap);
      printDungeon(dungeon);
      save_state(dungeon, hardnessMap);
    }
    else if (!strcmp(argv[1], "--pthf")) {
      initDungeon(dungeon);
      genDungeon(dungeon);
      genHardness(hardnessMap);
      genPC(dungeon);
      printDungeon(dungeon);
      genCantTunnelPath();
      printCantTunnelPath();
      genCanTunnelPath();
      printCanTunnelPath();
    }
    else if (!strcmp(argv[1], "--mons")) {
      initDungeon(dungeon);
      genDungeon(dungeon);
      genHardness(hardnessMap);
      genPC(dungeon);
      genCantTunnelPath();
      genCanTunnelPath();
      initMonsters(monsters);
      initScore();
      printDungeon(dungeon);
      
      while (!gameOver) {
	makeMoveRandPC();
      }
      printDungeon(dungeon);
    }
    else if (!strcmp(argv[1], "--play")) {
      initDungeon(dungeon);
      genDungeon(dungeon);
      genHardness(hardnessMap);
      genPC(dungeon);
      genStairs(4);
      initMonsters(monsters);
      initScore();
      initTerminal();
      printDungeonToTerminal(dungeon);
      
      while (!gameOver) {
	
	makeMove();

	if (quit) {
	  endwin();
	  return 0;
	}
	
	printDungeonToTerminal(dungeon);
	printMonsterList(0);
      }

      endwin();
    }
  }
  //There are two arguments
  if (argc == 3) {
   
    if (!strcmp(argv[1], "--save") && !strcmp(argv[2], "--load") ||
	!strcmp(argv[1], "--load") && !strcmp(argv[2], "--save")) {
      load_state(dungeon, hardnessMap);
      printDungeon(dungeon);
      save_state(dungeon, hardnessMap);
    }
    else if (!strcmp(argv[1], "--load") && !strcmp(argv[2], "--pthf")) {
      load_state(dungeon, hardnessMap);
      genPC(dungeon);
      printDungeon(dungeon);
      genCantTunnelPath();
      genCanTunnelPath();
      printCantTunnelPath();
      printCanTunnelPath();
    }
    else if (!strcmp(argv[1], "--load") && !strcmp(argv[2], "--mons")) {
      load_state(dungeon, hardnessMap);
      genPC(dungeon);
      genCantTunnelPath();
      genCanTunnelPath();
      initMonsters(monsters);
      initScore();
      printDungeon(dungeon);
      
      while (!gameOver) {
	makeMoveRandPC();
      }
      printDungeon(dungeon);
    }
    else if (!strcmp(argv[1], "--load") && !strcmp(argv[2], "--play")) {
      load_state(dungeon, hardnessMap);
      genPC(dungeon);
      genStairs(4);
      initMonsters(monsters);
      initScore();
      initTerminal();
      printDungeonToTerminal(dungeon);
      
      while (!gameOver) {
	
	makeMove();

	if (quit) {
	  endwin();
	  return 0;
	}
	
	printDungeonToTerminal(dungeon);
	printMonsterList(0);
      }

      endwin();
    }
  }

  return 0;
}
