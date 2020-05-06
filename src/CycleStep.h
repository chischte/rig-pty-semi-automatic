#ifndef CYCLESTEP_H
#define CYCLESTEP_H
#include <ArduinoSTL.h>

class Cycle_step
{
public:
  // VARIABLES:
  static int object_count;

  // FUNTIONS:
  Cycle_step();
  // Every derived class must implement this method:
  virtual void do_stuff() = 0;
  // SETTER:
  void set_completed();
  // GETTER:
  bool is_completed();
  virtual char *get_display_string() = 0;

private:
  // VARIABLES:
  bool _completed;

  // FUNCTIONS:
  //n.A.
};

#endif
