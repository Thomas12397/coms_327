#ifndef PC_H
#define PC_H

#include <vector>
#include <string>
#include "object.hpp"

class PC {
  
 public:
  int xpos;
  int ypos;
  char lastPos;
  int speed;
  int priority;
  int score;
  bool hasMoved;
  int hp;
  string dam;
  int damInt;
  Object equip[12];
  vector<Object> carry;
};

#endif

