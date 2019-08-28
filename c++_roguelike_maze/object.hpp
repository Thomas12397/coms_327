#ifndef OBJECT_H
#define OBJECT_H

#include <string>

using namespace std;

class Object {

 public:
  char lastPos;
  int xpos;
  int ypos;

  string name;
  string desc;
  string type;
  char symb;
  string color;
  string dam;
  int hit;
  int dodge;
  int def;
  int weight;
  int speed;
  int attr;
  int val;
  int art;
  int rrty;

  Object();
  Object(string n, string d, string t, char s, string c, string da,
	 int h, int dod, int de, int w, int sp, int at, int v, int ar, int r); 
  ~Object() {};
};

#endif
