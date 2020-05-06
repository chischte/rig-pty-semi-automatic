#ifndef CYCLESTEP_H
#define CYCLESTEP_H
#include "Arduino.h"

class CycleStep
{
public:
  // VARIABLES:
  static int objectCount;

  // FUNTIONS:
  CycleStep();
  // Every derived class must implement this method:
  virtual void doStuff() = 0;
  // SETTER:
  void set_solved();
  // GETTER:
  bool is_completed();
  virtual String getDisplayString() = 0;

private:
  // VARIABLES:
  bool _completed;

  // FUNCTIONS:
  //n.A.
};

#endif
