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
  void setDisplayString(String displayString);
  // GETTER:
  String getDisplayString();

private:
  // VARIABLES:
  String _displayString = "n.a.";
  // FUNCTIONS:
  //n.A.
};

#endif
