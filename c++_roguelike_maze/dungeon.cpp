#include <cstdio>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <endian.h>
#include <limits.h>
#include <ncurses.h>
#include "room.hpp"
#include "pc.hpp"
#include "monster.hpp"
#include "object.hpp"
#include <fstream>
#include <string>
#include <vector>

#define DUNGEON_X              80
#define DUNGEON_Y              21
#define ROOMS                  5
#define MONSTERS               10
#define OBJECTS                10

/**
 *  Global Variables
 */
char dungeon[DUNGEON_Y][DUNGEON_X];
int hardnessMap[DUNGEON_Y][DUNGEON_X];
Room rooms[ROOMS];
PC pc;
int canTunnelPath[DUNGEON_Y][DUNGEON_X];
int cantTunnelPath[DUNGEON_Y][DUNGEON_X];
bool gameOver;
bool quit;
bool displayMonsterList;
int moveScroll;
char fogMap[DUNGEON_Y][DUNGEON_X];
bool displayFogMap = true;
bool teleporting;
vector<Monster> monstersParsed;
vector<Monster> monsters;
vector<Object> objectsParsed;
vector<Object> objects;
vector<int> scores;
bool displayObject;
bool displayInventory;
bool displayEquipment;
bool PC_won;

/**
 *  Function list
 */
void initDungeon(char board[DUNGEON_Y][DUNGEON_X]);
void swap(Room *x, Room *y);
void shuffle(Room rooms[ROOMS]);
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

bool isValidRoom(int mon_x, int mon_y);
void initScore();

void initTerminal();
void genStairs(int count);
void moveMonsterList();
void keyboardListener();
void movePC(int x, int y);
void printMonsterList(int move);
void makeMove();
void printToTerminal(char board[DUNGEON_Y][DUNGEON_X]);

void toggleFog();
void updateFogMap();
void deleteNonVisibleCharacters();
void teleportPC();
void teleportRandomly();

void parseMonFile();
void parseObjFile();

void selectMonstersForDungeon();
void initMonsters();
void moveMonster(unsigned order);
int diceValue(int b, int d, int s);
int getDiceVal(string diceStr);
short getColor(char s, int x, int y);
char getObjSymb(string line);
void selectObjectsForDungeon();
void initObjects();
char pickUpObject(char s);

void wearItem();
void equipItem(int carry);
void takeOffItem();
void dropItem();
void expungeItem();
void inspectItem();
int selectCarry();
int selectEquip();
int calculatePCDamage();
int calculatePCSpeed();
void listPCInventory();
void listPCEquipment();
void displayObj(int itemPos);

int main(int argc, char * argv[]);

/**
 *  Generates the border around the dungeon board and fills
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
void swap(Room *x, Room *y)
{
  Room tmp;
  tmp = *x;
  *x = *y;
  *y = tmp;
}

/**
 *  Shuffles the rooms around
 */
void shuffle(Room rooms[ROOMS])
{
  swap(&rooms[0], &rooms[3]);
  swap(&rooms[1], &rooms[0]);
  swap(&rooms[2], &rooms[4]);
}

/**
 *  This function generates the rooms and corridors. After generating them,
 *  it puts them onto the dungeon board. It also randomly places a PC on
 *  a room or corridor.
 */
void genDungeon(char board[DUNGEON_Y][DUNGEON_X])
{
  //Randomly generates the room positions and sizes
  int i, lastx;
  rooms[0].xpos = (rand() % 5) + 2; //2-6
  for (i = 0; i < ROOMS; i++) {

    if (i != 0) {
      lastx = rooms[i-1].xpos + rooms[i-1].xsize;
      rooms[i].xpos = lastx + (rand() % 5) + 4; //Lastx + 4-8
    }
    rooms[i].ypos = (rand() % (DUNGEON_Y/2-2)) + 2; //2-10
    rooms[i].xsize = (rand() % 4) + 5; //5-8
    rooms[i].ysize = (rand() % 6) + 4; //4-9
  }

  shuffle(rooms);

  //Puts the rooms on the board
  int row, col;
  for (i = 0; i < ROOMS; i++)
    for (row = rooms[i].ypos; row < rooms[i].ypos + rooms[i].ysize; row++)
      for (col = rooms[i].xpos; col < rooms[i].xpos + rooms[i].xsize; col++)
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

      if (i == 0 || i == DUNGEON_Y-1 || j == 0 || j == DUNGEON_X-1)
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
  path = (char*) malloc(length);
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

      if ((hardness[i][j] == 255 && i == 0) || (hardness[i][j] == 255 && i == DUNGEON_Y - 1))
	board[i][j] = '-';
      else if ((hardness[i][j] == 255 && j == 0) || (hardness[i][j] == 255 && j == DUNGEON_X - 1))
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
    for (row = rooms[i].ypos; row < rooms[i].ypos + rooms[i].ysize; row++)
      for (col = rooms[i].xpos; col < rooms[i].xpos + rooms[i].xsize; col++)
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
  char *path = (char*) malloc(length);
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
  int pc_x, pc_y, hp;

  pc_x = rand() % DUNGEON_X;
  pc_y = rand() % DUNGEON_Y;

  if (board[pc_y][pc_x] == '.') {
    pc.lastPos = board[pc_y][pc_x];
    pc.xpos = pc_x;
    pc.ypos = pc_y;
    pc.speed = 10;
    pc.score = 1000/pc.speed;
    pc.priority = 1000/pc.speed;
    hp = pc.hp;
    pc.hp = 1000;
    if (hp < pc.hp && hp != 0)
      pc.hp = hp;

    pc.dam = "0+1d4";
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
      if (hardnessMap[i][j] == 255 || (pc.ypos == i && pc.xpos == j))
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
 *  This function makes it so monsters do NOT spawn in the same room as the PC. mon_x and mon_y are the
 *  monster's x and y coordinates and they get checked if they are in the same room as the PC.
 */
bool isValidRoom(int mon_x, int mon_y)
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
    return false;
  }
  return true;
}

/**
 *  Initializes the score which determines who should go next whether it's a monster or the PC.
 */
void initScore()
{
  unsigned i;
  scores.erase(scores.begin(), scores.end());
  for (i = 0; i < monsters.size(); i++) {
    scores.push_back(monsters.at(i).priority);
  }
  scores.push_back(pc.priority);
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
  start_color();
}

/**
 *  Generates count/2 '>' and count/2 '<' randomly in a room or corridor.
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
  selectMonstersForDungeon();
  initMonsters();
  selectObjectsForDungeon();
  initObjects();
  initScore();
  initDungeon(fogMap);
}

/**
 *  Handles input coming from the keyboard. Recursively calling keyboardListener() will prevent
 *  making the PC use up a turn
 */
void keyboardListener()
{
  switch(getch())
    {
    case '1':              //Lower Left
      movePC(pc.xpos - 1, pc.ypos + 1);
      break;
    case '2':              //Down
      movePC(pc.xpos, pc.ypos + 1);
      break;
    case '3':              //Lower Right
      movePC(pc.xpos + 1, pc.ypos + 1);
      break;
    case '4':              //Left
      movePC(pc.xpos - 1, pc.ypos);
      break;
    case '5':              //Center
      movePC(pc.xpos, pc.ypos);
      break;
    case '6':              //Right
      movePC(pc.xpos + 1, pc.ypos);
      break;
    case '7':              //Upper Left
      movePC(pc.xpos - 1, pc.ypos - 1);
      break;
    case '8':              //Up
      movePC(pc.xpos, pc.ypos - 1);
      break;
    case '9':              //Upper Right
      movePC(pc.xpos + 1, pc.ypos - 1);
      break;
    case 'Q':              //Quit
      quit = true;
      break;
    case 'm':              //Display monster list
      moveScroll = 0;
      displayMonsterList = true;
      printMonsterList(moveScroll);
      break;
    case 27:               //Display the dungeon
      displayMonsterList = false;
      displayObject = false;
      displayInventory = false;
      displayEquipment = false;
      //Clear out space
      wmove(stdscr, 0, 0);
      clrtoeol();
      unsigned i;
      for (i = 0; i < DUNGEON_Y+2; i++) {
	wmove(stdscr, i + 1, 0);
	clrtoeol();
      }

      if (!displayFogMap)
	printToTerminal(dungeon);
      else
	printToTerminal(fogMap);
      break;
    case KEY_UP:           //Scroll up
      if (displayMonsterList) {
	if (moveScroll < 0)
	  moveScroll++;
	printMonsterList(moveScroll);
      }
      break;
    case KEY_DOWN:         //Scroll down
      if (displayMonsterList) {
	if (moveScroll > -1 * ((int)monsters.size() - DUNGEON_Y -
			       ((int)monsters.size() - DUNGEON_Y-76)))
	  moveScroll--;
	printMonsterList(moveScroll);
      }
      break;
    case 'f':              //Toggle fog of war
      toggleFog();
      break;
    case 'g':              //Teleport PC and choose location
      teleportPC();
      break;
    case 'r':              //Teleport randomly if teleport is true
      teleportRandomly();
      break;
    case 'w':
      if (!pc.carry.empty()) {
	displayInventory = true;
	listPCInventory();
	wearItem();
      }
      break;
    case 't':
      for (i = 0; i < 12; i++) {
	if (!pc.equip[i].name.empty()) {
	  displayEquipment = true;
	  listPCEquipment();
	  takeOffItem();
	  break;
	}
      }
      break;
    case 'd':
      if (!pc.carry.empty() && (pc.lastPos == '.' || pc.lastPos == '#')) {
	displayInventory = true;
	listPCInventory();
	dropItem();
      }
      break;
    case 'x':
      if (!pc.carry.empty()) {
	displayInventory = true;
	listPCInventory();
	expungeItem();
      }
      break;
    case 'i':
      displayInventory = true;
      listPCInventory();
      break;
    case 'e':
      displayEquipment = true;
      listPCEquipment();
      break;
    case 'I':
      if (!pc.carry.empty()) {
	displayInventory = true;
	listPCInventory();
	inspectItem();
      }
      break;
    default:               //Don't do anything if incorrect key was pressed
      break;
    }
}

/**
 *  Moves the PC according to the keyboardListener() function
 */
void movePC(int x, int y)
{
  char tempChar;
  unsigned i;

  if (!displayMonsterList && !displayInventory && !displayEquipment && !displayObject) {
    if (teleporting && hardnessMap[y][x] != 255) {
      tempChar = dungeon[y][x];
      if (tempChar=='*')
	tempChar = pc.lastPos;
      dungeon[pc.ypos][pc.xpos] = pc.lastPos;
      pc.xpos = x;
      pc.ypos = y;
      dungeon[pc.ypos][pc.xpos] = '*';
      pc.lastPos = tempChar;
      printToTerminal(dungeon);
    }
    else if (dungeon[y][x] == '>' || dungeon[y][x] == '<' || pc.lastPos == '>' || pc.lastPos == '<') {
      initStairDungeon();
      pc.hasMoved = true;
    }
    else if (dungeon[y][x] == '.' || dungeon[y][x] == '#' || dungeon[y][x] == '@') {
      tempChar = dungeon[y][x];
      if (tempChar=='@')
	tempChar = pc.lastPos;
      dungeon[pc.ypos][pc.xpos] = pc.lastPos;
      pc.xpos = x;
      pc.ypos = y;
      dungeon[y][x] = '@';
      pc.lastPos = tempChar;
      pc.hasMoved = true;
    }
    //Must be a monster or object
    else {
      for (i = 0; i < monsters.size(); i++) {
	if (y == monsters.at(i).ypos && x == monsters.at(i).xpos && !teleporting) {
	  pc.damInt = getDiceVal(pc.dam) + calculatePCDamage();
	  monsters.at(i).hp -= pc.damInt;

	  wmove(stdscr, DUNGEON_Y+2, 0);
	  clrtoeol();
	  mvprintw(DUNGEON_Y+2, 0, "Monster %c took %d damage. HP is now %d\n",
		   monsters.at(i).symb, pc.damInt, monsters.at(i).hp);
	  refresh();

	  if (monsters.at(i).hp <= 0) {
	    tempChar = monsters.at(i).lastPos;
	    dungeon[pc.ypos][pc.xpos] = pc.lastPos;
	    pc.xpos = x;
	    pc.ypos = y;
	    dungeon[y][x] = '@';
	    pc.lastPos = tempChar;
	    pc.hasMoved = true;

	    if (monsters.at(i).isBoss) {
	      gameOver = true;
	      PC_won = true;
	    }

	    monsters.erase(monsters.begin() + i);
	    scores.erase(scores.begin() + i);
	  }

	  pc.hasMoved = true;
	}
      }
      for (i = 0; i < objects.size(); i++) {
	if (dungeon[y][x] == objects.at(i).symb && x == objects.at(i).xpos && y == objects.at(i).ypos) {
	  dungeon[pc.ypos][pc.xpos] = pc.lastPos;
	  pc.xpos = x;
	  pc.ypos = y;
	  tempChar = objects.at(i).lastPos;
	  pickUpObject(objects.at(i).symb);
	  if (tempChar=='@')
	    tempChar = pc.lastPos;
	  dungeon[y][x] = '@';
	  pc.lastPos = tempChar;
	  pc.hasMoved = true;
	  break;
	}
      }
    }
  }
}

/**
 *  Generates and prints the monster positions relative to the PC in ncurses terminal.
 *  The parameter 'move' is used for scrolling the list up and down.
 */
void printMonsterList(int move)
{
  int mon_x, mon_y, print_x, print_y;
  unsigned i;

  //Clear out the dungeon
  wmove(stdscr, 0, 0);
  clrtoeol();
  for (i = 0; i < DUNGEON_Y+2; i++) {
    wmove(stdscr, i + 1, 0);
    clrtoeol();
  }

  //Display the monster list
  mvprintw(0, 0, "Displaying Monsters List");
  for (i = 0; i < monsters.size(); i++) {
    mon_x = monsters.at(i).xpos;
    mon_y = monsters.at(i).ypos;
    print_x = abs(mon_x - pc.xpos);
    print_y = abs(mon_y - pc.ypos);

    mvprintw(i + 1 + move, 0, "%3d. %c:", i+1,  monsters.at(i).symb);
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
  refresh();
}

/**
 *  Prints the terminal using ncurses. Also handles displaying game over
 */
void printToTerminal(char board[DUNGEON_Y][DUNGEON_X])
{
  unsigned i, j;
  short curColor;

  wmove(stdscr, 0, 0);
  clrtoeol();

  //Display the board
  if (teleporting)
    mvprintw(0, 0, "Teleporting");
  else if (displayFogMap)
    mvprintw(0, 0, "Displaying Fog of War");
  else
    mvprintw(0, 0, "Displaying Dungeon");

  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      //PC
      if (board[i][j] == '@' || board[i][j] == '*') {
	init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
	attron(COLOR_PAIR(COLOR_CYAN));
	mvaddch(i + 1, j, board[i][j]);
	attroff(COLOR_PAIR(COLOR_CYAN));
      }
      //Monster or object
      else if (board[i][j] != '.' && board[i][j] != '#' && board[i][j] != ' ' &&
	       i != 0 && i != DUNGEON_Y-1 && j != 0 && j != DUNGEON_X-1 &&
	       board[i][j] != '>' && board[i][j] != '<') {
	curColor = getColor(board[i][j], j, i);
	init_pair(curColor, curColor, COLOR_BLACK);
	attron(COLOR_PAIR(curColor));
	mvaddch(i + 1, j, board[i][j]);
	attroff(COLOR_PAIR(curColor));
      }
      //Rest of the board
      else
	mvaddch(i + 1, j, board[i][j]);
    }
  }
  if (gameOver && !PC_won) {
    wmove(stdscr, 0, 0);
    clrtoeol();
    mvprintw(0, 0, "RIP PC. You were brutally killed by in %d turns. Game over :(\n", pc.score / 100 - 1);
    refresh();
    sleep(3);
  }
  else if (gameOver && PC_won) {
    wmove(stdscr, 0, 0);
    clrtoeol();
    mvprintw(0, 0, "PC won. Congratulations!\n");
    refresh();
    sleep(3);
  }
}

/**
 *  Toggles to display the fogMap of the Dungeon
 */
void toggleFog()
{
  if (displayFogMap) {
    displayFogMap = false;
    printToTerminal(dungeon);
  }
  else {
    displayFogMap = true;
    printToTerminal(fogMap);
  }
}

/**
 *  Updates the fogMap whenever the PC moves
 */
void updateFogMap()
{
  int i, j;
  for (i = pc.ypos-2; i <= pc.ypos+2; i++) {
    for (j = pc.xpos-2; j <= pc.xpos+2; j++) {
      fogMap[i][j] = dungeon[i][j];
    }
  }

  deleteNonVisibleCharacters();
}

/**
 *  Deletes characters that are not within view of the PC for fogMap
 */
void deleteNonVisibleCharacters()
{
  unsigned i, j, k;
  for (i = 0; i < DUNGEON_Y; i++) {
    for (j = 0; j < DUNGEON_X; j++) {
      for (k = 0; k < monsters.size(); k++) {
	if ((fogMap[i][j] == monsters.at(k).symb || fogMap[i][j] == '@') &&
	    fogMap[i][j] != dungeon[i][j]) {
	  fogMap[i][j] = dungeon[i][j];
	}
      }
    }
  }

  for (i = 0; i < monsters.size(); i++) {
    if ((monsters.at(i).xpos < pc.xpos-2 || monsters.at(i).xpos > pc.xpos+2 ||
	 monsters.at(i).ypos < pc.ypos-2 || monsters.at(i).ypos > pc.ypos+2) &&
	fogMap[monsters.at(i).ypos][monsters.at(i).xpos] != ' ') {
      fogMap[monsters.at(i).ypos][monsters.at(i).xpos] = monsters.at(i).lastPos;
    }
  }
}

/**
 *  Handles teleporting the PC. When 't' gets pressed the first
 *  time, teleport is set to true. If 't' gets pressed again, teleport becomes false
 */
void teleportPC()
{
  unsigned i;

  if (teleporting) {
    //Teleported on top of a monster
    teleporting = false;
    for (i = 0; i < monsters.size(); i++) {
      if (pc.lastPos == monsters.at(i).symb) {
	return;
      }
    }

    for (i = 0; i < objects.size(); i++) {
      if (pc.lastPos == objects.at(i).symb && pc.xpos == objects.at(i).xpos && pc.ypos == objects.at(i).ypos) {
	pickUpObject(objects.at(i).symb);
	break;
      }
    }

    dungeon[pc.ypos][pc.xpos] = '@';
    updateFogMap();
    if(displayFogMap)
      printToTerminal(fogMap);
    else
      printToTerminal(dungeon);
  }
  else {
    teleporting = true;
    dungeon[pc.ypos][pc.xpos] = '*';
    printToTerminal(dungeon);
  }
}

/**
 *  Handles randomly teleporting the PC. Only works if teleport
 *  is true
 */
void teleportRandomly()
{
  unsigned i;

  if (teleporting) {
    dungeon[pc.ypos][pc.xpos] = pc.lastPos;
    pc.xpos = (rand() % (DUNGEON_X-2)) + 1; //1-79
    pc.ypos = (rand() % (DUNGEON_Y-2)) + 1; //1-20
    pc.lastPos = dungeon[pc.ypos][pc.xpos];
    //Teleported on top of a monster
    for (i = 0; i < monsters.size(); i++) {
      if (pc.lastPos == monsters.at(i).symb) {
	teleportRandomly();
	return;
      }
    }
    //Teleported on top of an object
    for (i = 0; i < objects.size(); i++) {
      if (pc.lastPos == objects.at(i).symb) {
	pickUpObject(objects.at(i).symb);
      }
    }
    dungeon[pc.ypos][pc.xpos] = '@';
    teleporting = false;
    updateFogMap();

    if(displayFogMap)
      printToTerminal(fogMap);
    else
      printToTerminal(dungeon);
  }
}

/**
 *  Makes a move according to the monster's and PC's score. If the PC moves, the dungeon gets refreshed.
 */
void makeMove()
{
  int turn;
  unsigned i, order;

  //Figure out whose turn it is
  turn = scores.at(0);
  order = 0;
  for (i = 0; i < scores.size()-1; i++) {
    if (turn > scores.at(i+1)) {
      turn = scores.at(i+1);
      order = i+1;
    }
  }
  //Move the player
  if (order == scores.size()-1 || pc.score == 100) {

    updateFogMap();
    if(displayFogMap)
      printToTerminal(fogMap);
    else
      printToTerminal(dungeon);

    while (!pc.hasMoved && !quit && !gameOver) {
      keyboardListener();
    }
    pc.hasMoved = false;
    pc.priority = 1000/calculatePCSpeed();
    pc.score += pc.priority;
    scores.at(scores.size()-1) = pc.score;

    genCanTunnelPath();
    genCantTunnelPath();
  }
  //Move monster
  else {
    moveMonster(order);
    monsters.at(order).score += monsters.at(order).priority;
    scores.at(order) = monsters.at(order).score;
  }
}

/**
 *  Constructs a monster from parseMonFile()
 */
Monster::Monster(string n, string de, char s, string c, int sp, string a, int h, string da, int r)
{
  name = n;
  desc = de;
  symb = s;
  color = c;
  speed = sp;
  abil = a;
  hp = h;
  dam = da;
  rrty = r;
}

/**
 *  Parses a monster file that will create monsters for the dungeon.
 */
void parseMonFile()
{
  string path;
  string line;
  bool beginMonFound, nameFound, symbFound, colorFound, descFound, speedFound, damFound, hpFound,
    rrtyFound, abilFound, endFound, isDesc;

  //Monster variables to be used later
  string name, color, desc, speed, dam, hp, abil;
  int hpInt, speedInt, rrty;
  char symb;

  //Get the file
  path = getenv("HOME");
  path.append("/Documents/coms_327/c++_roguelike_maze/monster_desc.txt");
  ifstream infile(path.c_str());

  //Make sure it's open
  if (!infile.is_open()) {
    fprintf(stderr, "File in path: %s  was not found. Exited\n", path.c_str());
    return;
  }

  //Exit if the file doesn't begin with statement below
  getline(infile, line);
  if (line.find("RLG327 MONSTER DESCRIPTION 1") == string::npos) {
    fprintf(stderr, "Monster file did not contain \"RLG327 MONSTER DESCRIPTION 1\" on line 1. Exited\n");
    return;
  }

  while (getline(infile, line)) {
    if (line.empty()){}
    else if (line.find("BEGIN MONSTER") != string::npos) {
      beginMonFound = true;
    }
    else if (line.find("NAME") != string::npos) {
      nameFound = true;
      name = line.substr(5);
    }
    else if (line.find("SYMB") != string::npos) {
      symbFound = true;
      symb = line[5];
    }
    else if (line.find("COLOR") != string::npos) {
      colorFound = true;
      color = line.substr(6);
    }
    else if (line.find("SPEED") != string::npos) {
      speedFound = true;
      speed = line.substr(6);
    }
    else if (line.find("DAM") != string::npos) {
      damFound = true;
      dam = line.substr(4);
    }
    else if (line.find("HP") != string::npos) {
      hpFound = true;
      hp = line.substr(3);
    }
    else if (line.find("RRTY") != string::npos) {
      rrtyFound = true;
      rrty = atoi(line.substr(5).c_str());
    }
    else if (line.find("ABIL") != string::npos) {
      abilFound = true;
      abil = line.substr(5);
    }
    else if (isDesc || line.find("DESC") != string::npos) {
      descFound = true;
      if (!strcmp(line.c_str(), ".")) {
	isDesc = false;
      }
      else if (!strcmp(line.c_str(), "DESC")) {
	isDesc = true;
      }
      else {
	desc.append(line);
	desc.push_back('\n');
      }
    }
    else if (line.find("END") != string::npos) {
      endFound = true;

      //Print out monster values if it can be successfully constructed
      if (beginMonFound && nameFound && symbFound && colorFound && descFound && speedFound && damFound &&
	  hpFound && rrtyFound && abilFound && endFound) {

	hpInt = getDiceVal(hp);
	speedInt = getDiceVal(speed);
	Monster monster(name, desc, symb, color, speedInt, abil, hpInt, dam, rrty);
	monster.canTunnel = monster.abil.find("TUNNEL") != string::npos;
	monster.isUnique = monster.abil.find("UNIQ") != string::npos;
	monster.isBoss = monster.abil.find("BOSS") != string::npos;
	monstersParsed.push_back(monster);
      }
      else {
	fprintf(stderr, "The current monster being parsed contained an error or was missing an attribute\n\n");
      }

      //Reset monster parse
      beginMonFound = nameFound = symbFound = colorFound = descFound = speedFound = damFound
	= hpFound = rrtyFound = abilFound = endFound = isDesc = false;
      desc = "";
    }
  } //end while
  infile.close();
}

/**
 *  Selects monsters from a vector of monsters parsed in. The number of monsters that spawn in
 *  depends on a macro MONSTERS. Also, a monster's rrty determines its likelihood of being chosen.
 */
void selectMonstersForDungeon()
{
  int rarity, randMon;

  while (monsters.size() < MONSTERS && monstersParsed.size() > 0) {

    rarity = rand() % 100;
    randMon = rand() % (int)monstersParsed.size();

    if (rarity < monstersParsed.at(randMon).rrty) {
      monsters.push_back(monstersParsed.at(randMon));
      if (monstersParsed.at(randMon).isUnique)
	monstersParsed.erase(monstersParsed.begin() + randMon);
    }
  }
}

/**
 *  Initializes the monsters from the parsed monsters. Selects at random up to MONSTERS macro.
 */
void initMonsters()
{
  int  mon_x, mon_y;
  bool valid_pos;
  unsigned i;

  for (i = 0; i < monsters.size(); i++) {

    monsters.at(i).score = 1000/monsters.at(i).speed;
    monsters.at(i).priority = 1000/monsters.at(i).speed;

    valid_pos = false;
    while (!valid_pos) {
      mon_x = rand() % DUNGEON_X;
      mon_y = rand() % DUNGEON_Y;

      if (dungeon[mon_y][mon_x] == '.' && isValidRoom(mon_x, mon_y)) {
	monsters.at(i).lastPos = dungeon[mon_y][mon_x];
	monsters.at(i).xpos = mon_x;
	monsters.at(i).ypos = mon_y;
	dungeon[mon_y][mon_x] = monsters.at(i).symb;
	valid_pos = true;
      }
    }
  }
}

/**
 *  Moves a monster closer to the PC and does so in a random fashion. The parameter 'order' is used to
 *  define which monster to move in the array i.e. monsters[order].
 */
void moveMonster(unsigned order)
{
  int x, y, lastPos, newPos, newX, newY;
  bool valid_pos;
  char tempChar;
  unsigned i;

  //If the monster can tunnel. Move accordingly.
  if (monsters.at(order).canTunnel) {
    x = monsters.at(order).xpos;
    y = monsters.at(order).ypos;
    lastPos = canTunnelPath[y][x];

    valid_pos = false;
    while (!valid_pos) {
      newX = x + (rand() % 3) - 1;
      newY = y + (rand() % 3) - 1;
      newPos = canTunnelPath[newY][newX];

      for (i = 0; i < monsters.size(); i++) {
	if (newY == monsters.at(i).ypos && newX == monsters.at(i).xpos)
	  return;
      }

      if (lastPos > newPos && hardnessMap[newY][newX] != 255 && dungeon[newY][newX] != '@') {
	if (hardnessMap[newY][newX] > 85) {
	  hardnessMap[newY][newX] -= 85;
	  valid_pos = true;
	  continue;
	}
	else
	  hardnessMap[newY][newX] = 0;

	monsters.at(order).xpos = newX;
	monsters.at(order).ypos = newY;
	if (dungeon[newY][newX] == ' ')
	  tempChar = '#';
	else
	  tempChar = dungeon[newY][newX];

	dungeon[newY][newX] = monsters.at(order).symb;
	hardnessMap[newY][newX] = 0;
	dungeon[y][x] = monsters.at(order).lastPos;
	monsters.at(order).lastPos = tempChar;
	valid_pos = true;
      }

      if (dungeon[newY][newX] == '@') {
	monsters.at(order).damInt = getDiceVal(monsters.at(order).dam);
	pc.hp -= monsters.at(order).damInt;

	wmove(stdscr, DUNGEON_Y+1, 0);
	clrtoeol();
	mvprintw(DUNGEON_Y+1, 0, "PC took %d damage from Monster %c. HP is now %d\n",
		 monsters.at(order).damInt, monsters.at(order).symb, pc.hp);
	refresh();

	if (pc.hp <= 0) {
	  monsters.at(order).xpos = newX;
	  monsters.at(order).ypos = newY;
	  dungeon[newY][newX] = monsters.at(order).symb;
	  dungeon[y][x] = monsters.at(order).lastPos;
	  gameOver = true;
	}
      }
    }
  }
  //Otherwise the monster can't tunnel. Move accordingly.
  else {
    x = monsters.at(order).xpos;
    y = monsters.at(order).ypos;
    lastPos = cantTunnelPath[y][x];

    valid_pos = false;
    while (!valid_pos) {
      newX = x + (rand() % 3) - 1;
      newY = y + (rand() % 3) - 1;
      newPos = cantTunnelPath[newY][newX];

      for (i = 0; i < monsters.size(); i++) {
	if (newY == monsters.at(i).ypos && newX == monsters.at(i).xpos)
	  return;
      }

      if (lastPos > newPos && hardnessMap[newY][newX] == 0 && dungeon[newY][newX] != '@') {
	monsters.at(order).xpos = newX;
	monsters.at(order).ypos = newY;
	tempChar = dungeon[newY][newX];
	dungeon[newY][newX] = monsters.at(order).symb;
	hardnessMap[newY][newX] = 0;
	dungeon[y][x] = monsters.at(order).lastPos;
	monsters.at(order).lastPos = tempChar;
	valid_pos = true;
      }

      if (dungeon[newY][newX] == '@') {
	monsters.at(order).damInt = getDiceVal(monsters.at(order).dam);
	pc.hp -= monsters.at(order).damInt;

	wmove(stdscr, DUNGEON_Y+1, 0);
	clrtoeol();
	mvprintw(DUNGEON_Y+1, 0, "PC took %d damage from Monster %c. HP is now %d\n",
		 monsters.at(order).damInt, monsters.at(order).symb, pc.hp);
	refresh();

	if (pc.hp <= 0) {
	  monsters.at(order).xpos = newX;
	  monsters.at(order).ypos = newY;
	  dungeon[newY][newX] = monsters.at(order).symb;
	  dungeon[y][x] = monsters.at(order).lastPos;
	  gameOver = true;
	}
      }
    }
  }
}

Object::Object() {}

/**
 *  Constructs an object
 */
Object::Object(string n, string d, string t, char s, string c, string da, int h, int dod,
	 int de, int w, int sp, int at, int v, int ar, int r)
{
  name = n;
  desc = d;
  symb = s;
  type = t;
  color = c;
  dam = da;
  hit = h;
  dodge = dod;
  def = de;
  weight = w;
  speed = sp;
  attr = at;
  val = v;
  art = ar;
  rrty = r;
}

/**
 *  Parses an object file that will create objects for the dungeon.
 */
void parseObjFile()
{
  string path;
  string line;
  bool beginObjFound, nameFound, descFound, typeFound, colorFound, hitFound, damFound, dodgeFound,
    defFound, weightFound, speedFound, attrFound, valFound, artFound, rrtyFound, endFound, isDesc;

  //Object variables to be used later
  string name, desc, type, color, hit, dam, dodge, def, weight, speed, attr, val, art;
  int speedInt, hitInt, dodgeInt, weightInt, defInt, attrInt, valInt, artInt, rrty;
  char symbChar;

  //Get the file
  path = getenv("HOME");
  path.append("/Documents/coms_327/c++_roguelike_maze/object_desc.txt");
  ifstream infile(path.c_str());

  //Make sure it's open
  if (!infile.is_open()) {
    fprintf(stderr, "File in path: %s  was not found. Exited\n", path.c_str());
    return;
  }

  //Exit if the file doesn't begin with statement below
  getline(infile, line);
  if (line.find("RLG327 OBJECT DESCRIPTION 1") == string::npos) {
    fprintf(stderr, "Object file did not contain \"RLG327 OBJECT DESCRIPTION 1\" on line 1. Exited\n");
    return;
  }

  while (getline(infile, line)) {
    if (line.empty()){}
    else if (line.find("BEGIN OBJECT") != string::npos) {
      beginObjFound = true;
    }
    else if (line.find("NAME") != string::npos) {
      nameFound = true;
      name = line.substr(5);
    }
    else if (line.find("TYPE") != string::npos) {
      typeFound = true;
      type = line.substr(5);
    }
    else if (line.find("COLOR") != string::npos) {
      colorFound = true;
      color = line.substr(6);
    }
    else if (line.find("HIT") != string::npos) {
      hitFound = true;
      hit = line.substr(4);
    }
    else if (line.find("DAM") != string::npos) {
      damFound = true;
      dam = line.substr(4);
    }
    else if (line.find("DODGE") != string::npos) {
      dodgeFound = true;
      dodge = line.substr(6);
    }
    else if (line.find("DEF") != string::npos) {
      defFound = true;
      def = line.substr(4);
    }
    else if (line.find("WEIGHT") != string::npos) {
      weightFound = true;
      weight = line.substr(7);
    }
    else if (line.find("SPEED") != string::npos) {
      speedFound = true;
      speed = line.substr(6);
    }
    else if (line.find("ATTR") != string::npos) {
      attrFound = true;
      attr = line.substr(5);
    }
    else if (line.find("VAL") != string::npos) {
      valFound = true;
      val = line.substr(4);
    }
    else if (line.find("ART") != string::npos) {
      artFound = true;
      art = line.substr(4);
    }
    else if (line.find("RRTY") != string::npos) {
      rrtyFound = true;
      rrty = atoi(line.substr(5).c_str());
    }
    else if (isDesc || line.find("DESC") != string::npos) {
      descFound = true;
      if (!strcmp(line.c_str(), ".")) {
	isDesc = false;
      }
      else if (!strcmp(line.c_str(), "DESC")) {
	isDesc = true;
      }
      else {
	desc.append(line);
	desc.push_back('\n');
      }
    }
    else if (line.find("END") != string::npos) {
      endFound = true;

      //Print out object values if it can be successfully constructed
      if (beginObjFound && nameFound && descFound && typeFound && colorFound && hitFound && damFound &&
	  dodgeFound && defFound && weightFound && speedFound && attrFound && valFound && artFound &&
	  rrtyFound && endFound) {

	symbChar = getObjSymb(type);
	hitInt = getDiceVal(hit);
	speedInt = getDiceVal(speed);
	dodgeInt = getDiceVal(dodge);
	weightInt = getDiceVal(weight);
	defInt = getDiceVal(def);
	attrInt = getDiceVal(attr);
	valInt = getDiceVal(val);
	artInt = getDiceVal(art);

	Object obj(name, desc, type, symbChar, color, dam, hitInt, dodgeInt, defInt, weightInt,
		   speedInt, attrInt, valInt, artInt, rrty);
	objectsParsed.push_back(obj);

      }
      else {
	fprintf(stderr, "The current object being parsed contained an error or was missing an attribute\n");
      }

      //Reset object parse
      beginObjFound = nameFound = descFound = typeFound = colorFound = hitFound = damFound
	= dodgeFound = defFound = weightFound = speedFound = attrFound = valFound = artFound
	= rrtyFound = endFound = false;
      desc = "";
    }
  } //end while
  infile.close();
}

/**
 *  Gets the symbol for an object from an object's type
 */
char getObjSymb(string line)
{
  char result;

  if (line.find("WEAPON") != string::npos) {
    result = '|';
  }
  else if (line.find("OFFHAND") != string::npos) {
    result = ')';
  }
  else if (line.find("RANGED") != string::npos) {
    result = '}';
  }
  else if (line.find("ARMOR") != string::npos) {
    result = '[';
  }
  else if (line.find("HELMET") != string::npos) {
    result = ']';
  }
  else if (line.find("CLOAK") != string::npos) {
    result = '(';
  }
  else if (line.find("GLOVES") != string::npos) {
    result = '{';
  }
  else if (line.find("BOOTS") != string::npos) {
    result = 92;
  }
  else if (line.find("RING") != string::npos) {
    result = '=';
  }
  else if (line.find("AMULET") != string::npos) {
    result = '"';
  }
  else if (line.find("LIGHT") != string::npos) {
    result = '_';
  }
  else if (line.find("SCROLL") != string::npos) {
    result = '~';
  }
  else if (line.find("BOOK") != string::npos) {
    result = '?';
  }
  else if (line.find("FLASK") != string::npos) {
    result = '!';
  }
  else if (line.find("GOLD") != string::npos) {
    result = '$';
  }
  else if (line.find("AMMUNITION") != string::npos) {
    result = '/';
  }
  else if (line.find("FOOD") != string::npos) {
    result = ',';
  }
  else if (line.find("WAND") != string::npos) {
    result = '-';
  }
  else if (line.find("CONTAINER") != string::npos) {
    result = '%';
  }
  else {
    result = '*';
  }

  return result;
}

/**
 *  Selects monsters from a vector of monsters parsed in. The number of monsters that spawn in
 *  depends on a macro MONSTERS. Also, a monster's rrty determines its likelihood of being chosen.
 */
void selectObjectsForDungeon()
{
  int rarity, randObj;

  while (objects.size() < OBJECTS && objectsParsed.size() > 0) {

    rarity = rand() % 100;
    randObj = rand() % (int)objectsParsed.size();

    if (rarity < objectsParsed.at(randObj).rrty) {
      objects.push_back(objectsParsed.at(randObj));
      objectsParsed.erase(objectsParsed.begin() + randObj);
    }
  }
}

/**
 *  Initializes the monsters from the parsed monsters. Selects at random up to MONSTERS macro.
 */
void initObjects()
{
  int  obj_x, obj_y;
  bool valid_pos;
  unsigned i;

  for (i = 0; i < objects.size(); i++) {

    valid_pos = false;
    while (!valid_pos) {
      obj_x = rand() % DUNGEON_X;
      obj_y = rand() % DUNGEON_Y;

      if (dungeon[obj_y][obj_x] == '.' || dungeon[obj_y][obj_x] == '#') {
	objects.at(i).lastPos = dungeon[obj_y][obj_x];
	objects.at(i).xpos = obj_x;
	objects.at(i).ypos = obj_y;
	dungeon[obj_y][obj_x] = objects.at(i).symb;
	valid_pos = true;
      }
    }
  }
}

/**
 *  Converts a given dice string into an integer
 */
int getDiceVal(string diceStr)
{
  int base, dice, sides;

  base = atoi(diceStr.substr(0, diceStr.find('+')).c_str());
  diceStr.erase(0, diceStr.find('+')+1);
  dice = atoi(diceStr.substr(0, diceStr.find('d')).c_str());
  diceStr.erase(0, diceStr.find('d')+1);
  sides = atoi(diceStr.c_str());

  return diceValue(base, dice, sides);
}

/**
 *  Returns the dice value of 3 integers passed in with the form <base> + <dice>d<sides>
 *  b = <base>, <dice> = d, <sides> = s
 */
int diceValue(int b, int d, int s)
{
  return b + (rand() % (d * s - d + 1)) + d;
}

/**
 *  RED, GREEN, BLUE, CYAN, YELLOW, MAGENTA, WHITE, BLACK
 */
short getColor(char s, int x, int y)
{
  unsigned i;
  string curColor;
  short result = COLOR_WHITE;

  for (i = 0; i < monsters.size(); i++) {
    if (monsters.at(i).symb == s && monsters.at(i).xpos == x && monsters.at(i).ypos == y) {
      curColor = monsters.at(i).color.substr(0, monsters.at(i).color.find(' '));
      break;
    }
  }

  for (i = 0; i < objects.size(); i++) {
    if (objects.at(i).symb == s && objects.at(i).xpos == x && objects.at(i).ypos == y) {
      curColor = objects.at(i).color.substr(0, objects.at(i).color.find(' '));
      break;
    }
  }

  if (curColor.find("BLACK") != string::npos) {
    result = COLOR_WHITE;
  }
  else if (curColor.find("RED") != string::npos) {
    result = COLOR_RED;
  }
  else if (curColor.find("GREEN") != string::npos) {
    result = COLOR_GREEN;
  }
  else if (curColor.find("BLUE") != string::npos) {
    result = COLOR_BLUE;
  }
  else if (curColor.find("CYAN") != string::npos) {
    result = COLOR_CYAN;
  }
  else if (curColor.find("YELLOW") != string::npos) {
    result = COLOR_YELLOW;
  }
  else if (curColor.find("MAGENTA") != string::npos) {
    result = COLOR_MAGENTA;
  }
  else if (curColor.find("WHITE") != string::npos) {
    result = COLOR_WHITE;
  }

  return result;
}

/**
 *  Allows for the PC to pick up an object and put it in their inventory.
 */
char pickUpObject(char s)
{
  unsigned i;
  char toRemove;

  if (!teleporting && pc.carry.size() < 10) {
    for (i = 0; i < objects.size(); i++) {
      if (objects.at(i).symb == s && pc.xpos == objects.at(i).xpos && pc.ypos == objects.at(i).ypos) {
	pc.carry.push_back(objects.at(i));
	pc.lastPos = objects.at(i).lastPos;
	toRemove = objects.at(i).symb;
	objects.erase(objects.begin() + i);
      }
    }
  }
  else
    toRemove = '*';

  return toRemove;
}

/**
 *  Allows for the PC to wear an item he/she picked up
 */
void wearItem()
{
  int itemPos;

  wmove(stdscr, 0, 0);
  clrtoeol();
  mvprintw(0, 0, "Select which item you would like to equip. Must be a number between 0 and %d",
	   pc.carry.size()-1);

  itemPos = selectCarry();

  if (itemPos < (int)pc.carry.size()) {
    equipItem(itemPos);
  }
  else {
    wearItem();
  }
}

/**
 *  Equips the item selected by the user from PC's carry inventory
 */
void equipItem(int carry)
{
  switch(pc.carry.at(carry).symb)
    {
    case '|':
      if (pc.equip[0].name.empty()) {
	pc.equip[0] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[0]);
	pc.equip[0] = pc.carry.at(carry);
      }
      break;
    case ')':
      if (pc.equip[1].name.empty()) {
	pc.equip[1] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[1]);
	pc.equip[1] = pc.carry.at(carry);
      }
      break;
    case '}':
      if (pc.equip[2].name.empty()) {
	pc.equip[2] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[2]);
	pc.equip[2] = pc.carry.at(carry);
      }
      break;
    case '[':
      if (pc.equip[3].name.empty()) {
	pc.equip[3] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[3]);
	pc.equip[3] = pc.carry.at(carry);
      }
      break;
    case ']':
      if (pc.equip[4].name.empty()) {
	pc.equip[4] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[4]);
	pc.equip[4] = pc.carry.at(carry);
      }
      break;
    case '(':
      if (pc.equip[5].name.empty()) {
	pc.equip[5] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[5]);
	pc.equip[5] = pc.carry.at(carry);
      }
      break;
    case '{':
      if (pc.equip[6].name.empty()) {
	pc.equip[6] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[6]);
	pc.equip[6] = pc.carry.at(carry);
      }
      break;
    case 92:
      if (pc.equip[7].name.empty()) {
	pc.equip[7] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[7]);
	pc.equip[7] = pc.carry.at(carry);
      }
      break;
    case '"':
      if (pc.equip[8].name.empty()) {
	pc.equip[8] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[8]);
	pc.equip[8] = pc.carry.at(carry);
      }
      break;
    case '_':
      if (pc.equip[9].name.empty()) {
	pc.equip[9] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[9]);
	pc.equip[9] = pc.carry.at(carry);
      }
      break;
    case '=':
      if (pc.equip[10].name.empty()) {
	pc.equip[10] = pc.carry.at(carry);
      }
      else if (pc.equip[11].name.empty()) {
	pc.equip[11] = pc.carry.at(carry);
      }
      else {
	pc.carry.push_back(pc.equip[11]);
	pc.equip[11] = pc.carry.at(carry);
      }
      break;
    }

  pc.carry.erase(pc.carry.begin() + carry);
  listPCInventory();

  wmove(stdscr, 0, 0);
  clrtoeol();
  mvprintw(0, 0, "Item Equipped. Displaying PC Carry Slots (Inventory)");
}

/**
 *
 */
void takeOffItem()
{
  int equipPos;

  wmove(stdscr, 0, 0);
  clrtoeol();
  mvprintw(0, 0, "Select which item you would like to take off. Must be a letter between a and l");

  equipPos = selectEquip();

  if (!pc.equip[equipPos].name.empty()) {
    pc.carry.push_back(pc.equip[equipPos]);
    pc.equip[equipPos].name = "";
    listPCEquipment();

    wmove(stdscr, 0, 0);
    clrtoeol();
    mvprintw(0, 0, "Item taken off. Displaying PC Equipment");
  }
  else {
    takeOffItem();
  }
}

/**
 *  Drops an item from PC's carry inventory and places it where the PC is standing
 */
void dropItem()
{
  int itemPos;

  wmove(stdscr, 0, 0);
  clrtoeol();
  mvprintw(0, 0, "Select which item you would like to drop. Must be a number between 0 and %d",
	   pc.carry.size()-1);

  itemPos = selectCarry();

  if (itemPos < (int)pc.carry.size()) {
    objects.push_back(pc.carry.at(itemPos));
    pc.carry.erase(pc.carry.begin() + itemPos);
    listPCInventory();
    pc.lastPos = objects.at(objects.size()-1).symb;
    objects.at(objects.size()-1).xpos = pc.xpos;
    objects.at(objects.size()-1).ypos = pc.ypos;

    wmove(stdscr, 0, 0);
    clrtoeol();
    mvprintw(0, 0, "Item dropped. Displaying PC Carry Slots (Inventory)");
  }
  else {
    dropItem();
  }
}

/**
 *  Permanently deletes an item from PC's carry inventory
 */
void expungeItem()
{
  int itemPos;

  wmove(stdscr, 0, 0);
  clrtoeol();
  mvprintw(0, 0, "Select which item you would like to expunge. Must be a number between 0 and %d",
	   pc.carry.size()-1);

  itemPos = selectCarry();

  if (itemPos < (int)pc.carry.size()) {
    pc.carry.erase(pc.carry.begin() + itemPos);
    listPCInventory();

    wmove(stdscr, 0, 0);
    clrtoeol();
    mvprintw(0, 0, "Item expunged. Displaying PC Carry Slots (Inventory)");
  }
  else {
    expungeItem();
  }
}

/**
 *
 */
void inspectItem()
{
  int itemPos;

  wmove(stdscr, 0, 0);
  clrtoeol();
  mvprintw(0, 0, "Select which item you would like to inspect. Must be a number between 0 and %d",
	   pc.carry.size()-1);

  itemPos = selectCarry();

  if (itemPos < (int)pc.carry.size()) {
    displayInventory = false;
    displayObject = true;
    displayObj(itemPos);
  }
  else {
    inspectItem();
  }
}

/**
 *  Allows the user to select a carry item
 */
int selectCarry()
{
  int itemPos;

  switch(getch())
    {
    case '0':
      itemPos = 0;
      break;
    case '1':
      itemPos = 1;
      break;
    case '2':
      itemPos = 2;
      break;
    case '3':
      itemPos = 3;
      break;
    case '4':
      itemPos = 4;
      break;
    case '5':
      itemPos = 5;
      break;
    case '6':
      itemPos = 6;
      break;
    case '7':
      itemPos = 7;
      break;
    case '8':
      itemPos = 8;
      break;
    case '9':
      itemPos = 9;
      break;
    case 'Q':
      quit = true;
      break;
    default:
      selectCarry();
      break;
    }

  return itemPos;
}

/**
 *  Allows a user to select an equipped item
 */
int selectEquip()
{
   int itemPos;

  switch(getch())
    {
    case 'a':
      itemPos = 0;
      break;
    case 'b':
      itemPos = 1;
      break;
    case 'c':
      itemPos = 2;
      break;
    case 'd':
      itemPos = 3;
      break;
    case 'e':
      itemPos = 4;
      break;
    case 'f':
      itemPos = 5;
      break;
    case 'g':
      itemPos = 6;
      break;
    case 'h':
      itemPos = 7;
      break;
    case 'i':
      itemPos = 8;
      break;
    case 'j':
      itemPos = 9;
      break;
    case 'k':
      itemPos = 10;
      break;
    case 'l':
      itemPos = 11;
      break;
    case 'q':
      quit = true;
      break;
    default:
      selectEquip();
      break;
    }

  return itemPos;
}

/**
 *  Calculates the PC's damage
 */
int calculatePCDamage()
{
  int i, result;

  result = getDiceVal(pc.dam.c_str());
  for (i = 0; i < 12; i++) {
    if (!pc.equip[i].name.empty()) {
      result += getDiceVal(pc.equip[i].dam.c_str());
    }
  }

  return result;
}

/**
 *  Calculates the PC's speed
 */
int calculatePCSpeed()
{
  int i, result;

  result = 10;
  for (i = 0; i < 12; i++) {
    if (!pc.equip[i].name.empty()) {
      result += pc.equip[i].speed;
    }
  }

  return result;
}

/**
 *  Lists the PC's carry inventory
 */
void listPCInventory()
{
  unsigned i;

  //Clear out the dungeon
  wmove(stdscr, 0, 0);
  clrtoeol();
  for (i = 0; i < DUNGEON_Y+2; i++) {
    wmove(stdscr, i + 1, 0);
    clrtoeol();
  }

  //Display the monster list
  mvprintw(0, 0, "Displaying PC Carry Slots (Inventory)");
  for (i = 0; i < pc.carry.size(); i++) {
    mvprintw(i + 1, 0, "%d. NAME: %-30s TYPE: %s", i, pc.carry.at(i).name.c_str(), pc.carry.at(i).type.c_str());
  }
  refresh();
}

/**
 *  Lists the PC's equipped items inventory
 */
void listPCEquipment()
{
  string curInv, name;
  unsigned i;

  //Clear out the dungeon
  wmove(stdscr, 0, 0);
  clrtoeol();
  for (i = 0; i < DUNGEON_Y+2; i++) {
    wmove(stdscr, i + 1, 0);
    clrtoeol();
  }

  //Display the monster list
  mvprintw(0, 0, "Displaying PC Equipped Items");
  for (i = 0; i < 12; i++) {
    name = "";
    switch(i)
      {
      case 0:
	curInv = "WEAPON";
	name = pc.equip[0].name;
	break;
      case 1:
	curInv = "OFFHAND";
	name = pc.equip[1].name;
	break;
      case 2:
	curInv = "RANGED";
	name = pc.equip[2].name;
	break;
      case 3:
	curInv = "ARMOR";
	name = pc.equip[3].name;
	break;
      case 4:
	curInv = "HELMET";
	name = pc.equip[4].name;
	break;
      case 5:
	curInv = "CLOAK";
	name = pc.equip[5].name;
	break;
      case 6:
	curInv = "GLOVES";
	name = pc.equip[6].name;
	break;
      case 7:
	curInv = "BOOTS";
	name = pc.equip[7].name;
	break;
      case 8:
	curInv = "AMULET";
	name = pc.equip[8].name;
	break;
      case 9:
	curInv = "LIGHT";
	name = pc.equip[9].name;
	break;
      case 10:
	curInv = "RING";
	name = pc.equip[10].name;
	break;
      case 11:
	curInv = "RING";
	name = pc.equip[11].name;
	break;
      }
    mvprintw(i + 1, 0, "%c. %7s: %s", i+97, curInv.c_str(), name.c_str());
  }
  refresh();
}

/**
 *  Displays the object the user decided to inspect from PC's carry inventory
 */
void displayObj(int itemPos)
{
  int i;

  //Clear out terminal
  wmove(stdscr, 0, 0);
  clrtoeol();
  for (i = 0; i < DUNGEON_Y+2; i++) {
    wmove(stdscr, i + 1, 0);
    clrtoeol();
  }

  //Display the Object
  mvprintw(0, 0, "Displaying Object Description");
  mvprintw(1, 0, "Symbol: %c", pc.carry.at(itemPos).symb);
  mvprintw(2, 0, "Name: %s", pc.carry.at(itemPos).name.c_str());
  mvprintw(3, 0, "Type: %s", pc.carry.at(itemPos).type.c_str());
  mvprintw(4, 0, "Speed: %d", pc.carry.at(itemPos).speed);
  mvprintw(5, 0, "Damage: %s", pc.carry.at(itemPos).dam.c_str());
  mvprintw(6, 0, pc.carry.at(itemPos).desc.c_str());
}

/**
 *  Main function for running the dungeon and decides what to do with
 *  the different flags given in the terminal
 */
int main(int argc, char * argv[])
{
  srand(time(NULL));

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
    else if (!strcmp(argv[1], "--play")) {
      initDungeon(dungeon);
      genDungeon(dungeon);
      genHardness(hardnessMap);
      genPC(dungeon);
      genStairs(4);
      parseMonFile();
      selectMonstersForDungeon();
      initMonsters();
      initScore();
      parseObjFile();
      selectObjectsForDungeon();
      initObjects();
      initTerminal();
      initDungeon(fogMap);
      updateFogMap();
      printToTerminal(fogMap);

      while (!gameOver) {

	makeMove();

	if (quit) {
	  endwin();
	  return 0;
	}
      }
      //Game over, print out terminal
      printToTerminal(dungeon);
      endwin();
    }
    else if (!strcmp(argv[1], "--parse_mon")) {
      parseMonFile();

      unsigned i;
      for (i = 0; i < monstersParsed.size(); i++) {
	printf("Name: %s\n",       monstersParsed.at(i).name.c_str());
	printf("Description:\n%s", monstersParsed.at(i).desc.c_str());
	printf("Symbol: %c\n",     monstersParsed.at(i).symb);
	printf("Color: %s\n",      monstersParsed.at(i).color.c_str());
	printf("Speed: %d\n",      monstersParsed.at(i).speed);
	printf("Ability: %s\n",    monstersParsed.at(i).abil.c_str());
	printf("HP: %d\n",         monstersParsed.at(i).hp);
	printf("Damage: %s\n",     monstersParsed.at(i).dam.c_str());
	printf("Rarity: %d\n",     monstersParsed.at(i).rrty);
	printf("\n");
      }
    }
    else if (!strcmp(argv[1], "--parse_obj")) {
      parseObjFile();

      unsigned i;
      for (i = 0; i < objectsParsed.size(); i++) {
	printf("Name: %s\n",         objectsParsed.at(i).name.c_str());
	printf("Description:\n%s",   objectsParsed.at(i).desc.c_str());
	printf("Type: %s\n",         objectsParsed.at(i).type.c_str());
	printf("Symbol: %c\n",       objectsParsed.at(i).symb);
	printf("Color: %s\n",        objectsParsed.at(i).color.c_str());
	printf("Hit: %d\n",          objectsParsed.at(i).hit);
	printf("Damage: %s\n",       objectsParsed.at(i).dam.c_str());
	printf("Dodge: %d\n",        objectsParsed.at(i).dodge);
	printf("Defense: %d\n",      objectsParsed.at(i).def);
	printf("Weight: %d\n",       objectsParsed.at(i).weight);
	printf("Speed: %d\n",        objectsParsed.at(i).speed);
	printf("Attribute: %d\n",    objectsParsed.at(i).attr);
	printf("Value: %d\n",        objectsParsed.at(i).val);
	printf("Artifact: %d\n",     objectsParsed.at(i).art);
	printf("Rarity: %d\n",       objectsParsed.at(i).rrty);
	printf("\n");
      }
    }
    else if (!strcmp(argv[1], "--test")) {

    }
  }
  //There are two arguments
  if (argc == 3) {

    if ((!strcmp(argv[1], "--save") && !strcmp(argv[2], "--load")) ||
	(!strcmp(argv[1], "--load") && !strcmp(argv[2], "--save"))) {
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
    else if (!strcmp(argv[1], "--load") && !strcmp(argv[2], "--play")) {
      load_state(dungeon, hardnessMap);
      genPC(dungeon);
      genStairs(4);
      parseMonFile();
      initMonsters();
      initScore();
      initTerminal();
      initDungeon(fogMap);
      updateFogMap();
      printToTerminal(fogMap);

      while (!gameOver) {

	makeMove();

	if (quit) {
	  endwin();
	  return 0;
	}
      }
      //Game over, print out terminal
      printToTerminal(dungeon);
      endwin();
    }
  }
  return 0;
}
