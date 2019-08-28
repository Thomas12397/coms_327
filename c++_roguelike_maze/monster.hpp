#ifndef MONSTER_H
#define MONSTER_H

#include <string>

using namespace std;

class Monster {

 public:
  bool canTunnel;
  bool isUnique;
  bool isBoss;
  char lastPos;
  int xpos;
  int ypos;
  int priority;
  int score;

  string name;
  string desc;
  char symb;
  string color;
  int speed;
  string abil;
  int hp;
  string dam;
  int damInt;
  int rrty;

  Monster();
  Monster(string n, string de, char s, string c, int sp, string a, int h, string da, int r); 
  ~Monster() {};
};

#endif
