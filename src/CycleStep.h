#ifndef CYCLESTEP_H
#define CYCLESTEP_H
#include "Arduino.h"

class Cycle_step
{
public:
  // VARIABLES:
  static int objectCount;

  // FUNTIONS:
  Cycle_step();
  // Every derived class must implement this method:
  virtual void do_stuff() = 0;
  // SETTER:
  void set_completed();
  // GETTER:
  bool is_completed();
  virtual String get_display_string() = 0;

private:
  // VARIABLES:
  bool _completed;

  // FUNCTIONS:
  //n.A.
};

#endif
