#include "Arduino.h"
#include "CycleStep.h"

CycleStep::CycleStep()
{
  objectCount++;
}

// SETTER:
void CycleStep::set_solved()
{
  Serial.println("SET SOLVED");
  _completed = true;
}
// GETTER:
// This is a "one time flag", state will be reseted after fist inquiry:
bool CycleStep::is_completed()
{
  if (_completed)
  {
    _completed = false;
    return true;
    Serial.begin(115200);
    Serial.println("step completed");
  }
  else
  {
    Serial.begin(115200);
    Serial.println("step not completed");
    return false;        
  }
}
