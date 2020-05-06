#include "Arduino.h"
#include "CycleStep.h"

Cycle_step::Cycle_step()
{
  objectCount++;
}

// SETTER:
void Cycle_step::set_completed()
{
  _completed = true;
}
// GETTER:
// This is a "one time flag", state will be reseted after fist inquiry:
bool Cycle_step::is_completed()
{
  if (_completed)
  {
    _completed = false;
    return true;
  }
  else
  {
    return false;
  }
}
