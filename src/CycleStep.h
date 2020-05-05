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
  // GETTER:
  virtual String getDisplayString() = 0;

private:
  // VARIABLES:

  // FUNCTIONS:
  //n.A.
};

#endif
