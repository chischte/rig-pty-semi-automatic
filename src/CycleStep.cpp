

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
  void CycleStep::setCycleStepNo(int cycleStepNo)
  {
    _cycleStepNo = cycleStepNo;
  }
  // GETTER:
  String CycleStep::getDisplayString()
  {
    Serial.println(_displayString);
    return _displayString;
  }
  void CycleStep::getCycleStepNo()
  {
    Serial.println(_cycleStepNo);
  }
