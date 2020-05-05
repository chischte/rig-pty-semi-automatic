#include "Arduino.h"
#include "CycleStep.h"

CycleStep::CycleStep()
{
  objectCount++;
}

// SETTER:
void CycleStep::setDisplayString(String displayString)
{
  _displayString = displayString;
}
// GETTER:
String CycleStep::getDisplayString()
{
  Serial.println(_displayString);
  return _displayString;
}
