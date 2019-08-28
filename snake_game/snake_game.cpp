#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<ncurses.h>

enum Direction {Up, Right, Down, Left};

/**
 *  The Snake Head
 */
class Snake
{

private:
  int x, y;
  char ch;

public:
  Snake()
  { 
    x = y = 0; 
    ch = 'O';
  }

  Snake(int given_x, int given_y)
  {
    x = given_x;
    y = given_y;
    ch = 'O';
  }

  Snake(const Snake &s)
  {
    ch = s.ch;
    x = s.x;
    y = s.y;
  }

  char getChar()
  {
    return ch;
  }

  int getX()
  {
    return x;
  }

  int getY()
  {
    return y;
  }

  void setChar(char x)
  {
    ch = x;
  }

  void setX(int n)
  {
    x = n;
  }

  void setY(int n)
  {
    y = n;
  }
};

/**
 *  A node represents the snake head and its next node reference
 */
class Node
{

private:
  Node * next;
  Snake s;

public:
  Snake getSnake()
  {
    return s;
  }

  Node* getNext()
  {
    return next;
  }

  void setSnake(Snake snake)
  {
    s = snake;
  }

  void setNext(Node *n)
  {
    next = n;
  }
};

/**
 *  Snake head and body
 */
class SnakeList
{

private:
  Node *head;
  int length;

public:
  SnakeList()
  {
    head = NULL;
    length = 0;
  }

  void add(Snake s)
  {
    Node *n = new Node;
    n->setSnake(s);
    n->setNext(head);
    head = n;
    length++;
  }

  int listLength()
  {
    return length;
  }

  Snake get(int n)
  {
    Node *temp = head;
    int count = 1;
    while(count != n && temp != NULL)
      {
	count++;
	temp = temp->getNext();
      } 
    return temp->getSnake();
  }

  void remove()
  {
    Node *temp = head->getNext();
    Node *t2 = head;
    while(temp->getNext() != NULL)
      {
	temp = temp->getNext();
	t2 = t2->getNext();
      }
    t2->setNext(NULL);
    delete temp;
    length--;
  }

  void display()
  {
    int i = 0;
    Node *temp = head;
    while(temp != NULL)
      {
	mvaddch(10, 10 + i, temp->getSnake().getChar());
	refresh();
	temp = temp->getNext();
      }
  }

  ~SnakeList()
  {
    while(head != NULL)
      {
	Node* n = head;
	head = head->getNext();
	delete n;
      }
    length = 0;
  }
};

/**
 *  Game event that contains a box, snake, and food
 */
class Game
{

private:
  int score, max_y, max_x, food_x, food_y;
  Direction dir, prev_dir;
  SnakeList sl;

public:
  Game()
  {
    score = max_y = max_x = food_x = food_y = 0;
    prev_dir = dir = Right;
  }
  void launch();
  void play();
  void map();
  void genFood();
  void setSnake();
  void moveSnake(Direction dir);
  bool checkForCollision();
};

void Game:: launch()
{
  initscr();
  start_color();
  cbreak();
  curs_set(FALSE);
  keypad(stdscr, TRUE);
  noecho();
  refresh();
  attron(A_BOLD);
  mvprintw(LINES/2 - 2, COLS/2 - 11, "WELCOME TO SNAKE GAME");
  mvprintw(LINES/2, COLS/2 - 12, "PRESS SPACE TO CONTINUE");

  if(getch() == 32)
    {
      getmaxyx(stdscr, max_y, max_x);
      play();
    }
  else 
    {
      clear();
      mvprintw(LINES/2, COLS/2 - 16, "YOU DID NOT PRESS SPACE. EXITING...");
      refresh();
      sleep(3);
    }
  attroff(A_BOLD);
  endwin();
}  

void Game:: genFood()
{
  srand(time(NULL));
  food_y = random() % (max_y - 6) + 4;
  food_x = random() % (max_x - 4) + 2;
}

void Game:: map()
{
  box(stdscr, 0, 0);
  mvprintw(1, 1, "SCORE: %d", score);      
  mvprintw(2, 1, "PRESS 'q' TO QUIT");
  refresh();
}

void Game:: setSnake()
{
  clear();
  map();
  for(int i = 0; i < 8; i++)
    {
      Snake s((COLS/2) - 8 + i, LINES/2);
      sl.add(s);
      init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
      attron(COLOR_PAIR(COLOR_GREEN));
      mvprintw(s.getY(), s.getX(), "%c", s.getChar());
      attroff(COLOR_PAIR(COLOR_GREEN));
    }
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  attron(COLOR_PAIR(COLOR_RED));
  mvaddch(food_y, food_x, 'F');
  refresh();
  attroff(COLOR_PAIR(COLOR_RED));
}

bool Game:: checkForCollision()
{
  bool collided = false;
  Snake s = sl.get(1);
  int head_x = s.getX();
  int head_y = s.getY();
  if(head_x == max_x - 1 || head_y == 0 || head_x == 0 || head_y == max_y - 1)
    return true;
  else 
    {
      int len = sl.listLength();
      for(int i = 4; i < len; i++)
	{
	  s = sl.get(i);
	  if(head_x == s.getX() && head_y == s.getY())
            {
	      collided = true;
	      break;
            }
        }
      return collided;
    }
}

void Game:: moveSnake(Direction dir)
{
  Snake k;
  k = sl.get(1);
  int x = k.getX(), y = k.getY();
  if(dir == Up)
    y--;
  else if(dir == Left)
    x--;
  else if(dir == Down)
    y++;
  else 
    x++;
  Snake s(x,y);
  sl.add(s);
  if(x == food_x && y == food_y)
    {
      score++; 
      genFood();
      init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
      attron(COLOR_PAIR(COLOR_RED));
      mvaddch(food_y, food_x, 'F');
      attroff(COLOR_PAIR(COLOR_RED));
    }
  else
    sl.remove();
  int len = sl.listLength();
  for(int i = 1; i <= len; i++)
    {
      k = sl.get(i);
      init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
      attron(COLOR_PAIR(COLOR_GREEN));
      mvaddch(k.getY(), k.getX(), k.getChar());
      refresh();
      attroff(COLOR_PAIR(COLOR_GREEN));
    }
}

void Game:: play()
{
  int ch = 0;
  int countdown = 3;

  genFood();
  setSnake();

  //Pauses game at the beginning for user to get ready
  mvprintw(LINES/2 - 2, COLS/2 - 15, "READY IN ");
  for (int i = 9; i < 21; i++) {
    if (i % 4 == 1) {
      mvprintw(LINES/2 - 2, COLS/2 - 15 + i, "%d", countdown);
      countdown--;
    }
    else {
      mvprintw(LINES/2 - 2, COLS/2 - 15 + i, ".");
    }
    //Don't sleep on the last one
    refresh();
    usleep(250000);
  }

  //Speed of the game
  timeout(75);

  while((ch = getch()) != 'q')
    {
      switch(ch)
        {
	case KEY_UP:
	  dir = Up; 
	  break;
	case KEY_DOWN:
	  dir = Down;
	  break;
	case KEY_RIGHT:
	  dir = Right;
	  break;
	case KEY_LEFT:
	  dir = Left;
	  break;
        }

      clear();
      map();
      init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
      attron(COLOR_PAIR(COLOR_RED));
      mvaddch(food_y, food_x, 'F');
      attroff(COLOR_PAIR(COLOR_RED));
      if(dir + 2 == prev_dir || prev_dir + 2 == dir)
	dir = prev_dir;
      moveSnake(dir);
      refresh();
      if(checkForCollision())
        {
	  clear();
	  mvprintw(max_y/2 - 2, max_x/2 - 8, "GAME OVER");
	  mvprintw(max_y/2, max_x/2 - 9, "YOUR SCORE %d", score);
	  refresh();
	  sleep(2);
	  break;
        }
      prev_dir = dir;
    } //End while
}

int main()
{
  Game g;
  g.launch();
  return 0;
}
