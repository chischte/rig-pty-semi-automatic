#ifndef CYCLESTEP_H
#define CYCLESTEP_H
#include "Arduino.h"

class CycleStep {
public:
  // VARIABLES:
  static int objectCount;
  // n.a.

  // FUNTIONS:
  CycleStep();
  // Every derived class must implement this method:
  virtual void doStuff() = 0;
  // SETTER:
  void setDisplayString(String displayString);
  void setCycleStepNo(int cycleStepNo);
  // GETTER:
  String getDisplayString();
  void getCycleStepNo();

private:
  // VARIABLES:
  String _displayString = "n.a.";
  int _cycleStepNo;
  // FUNCTIONS:
 



};



#endif

