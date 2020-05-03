#include <Arduino.h>

#include <Cylinder.h>       // https://github.com/chischte/cylinder-library
#include <Debounce.h>       // https://github.com/chischte/debounce-library
#include <Insomnia.h>       // https://github.com/chischte/insomnia-delay-library
#include <EEPROM_Counter.h> // https://github.com/chischte/eeprom-counter-library
#include <EEPROM_Logger.h>  // https://github.com/chischte/eeprom-logger-library.git

#include <StateController.h> // contains all machine states

//******************************************************************************
// DEFINE NAMES AND SEQUENCE OF STEPS FOR THE MAIN CYCLE:
//******************************************************************************
enum mainCycleSteps
{
  WippenhebelZiehen,
  BandklemmeLoesen,
  SchlittenZurueckfahren,
  BandVorschieben,
  BandSchneiden,
  BandKlemmen,
  BandSpannen,
  Schweissen,
  endOfMainCycleEnum
};
byte numberOfMainCycleSteps = endOfMainCycleEnum;

// DEFINE NAMES TO DISPLAY ON THE TOUCH SCREEN:
String cycleName[] = {"WIPPENHEBEL", "KLEMME LOESEN", "ZURUECKFAHREN", "BAND VORSCHIEBEN",
                      "SCHNEIDEN", "KLEMMEN", "SPANNEN", "SCHWEISSEN"};

//******************************************************************************
// DEFINE NAMES AND SET UP VARIABLES FOR THE CYCLE COUNTER:
//******************************************************************************
enum counter
{
  longtimeCounter,  //
  shorttimeCounter, //
  coolingTime,      // [s]
  endOfCounterEnum
};

int counterNoOfValues = endOfCounterEnum;

//******************************************************************************
// DEFINE NAMES AND SET UP VARIABLES FOR THE ERROR LOGGER:
//******************************************************************************
enum logger
{
  emptyLog, // marks empty logs
  toolResetError,
  shortTimeoutError,
  longTimeoutError,
  shutDownError,
  magazineEmpty,
  manualOn,
  manualOff
};

String errorCode[] = { //
    "n.a.",            //
    "STEUERUNG EIN",   //
    "AUTO RESET",      //
    "AUTO PAUSE",      //
    "AUTO STOP",       //
    "BAND LEER",       //
    "MANUELL START",   //
    "MANUELL STOP"};   //

int loggerNoOfLogs = 50;

//******************************************************************************
// DECLARATION OF VARIABLES
//******************************************************************************
bool strapDetected;
bool errorBlinkState = false;
byte timeoutDetected = 0;
int cycleTimeInSeconds = 30; // estimated value for the timout timer

//******************************************************************************
// GENERATE INSTANCES OF CLASSES:
//******************************************************************************
Cylinder SchlittenZylinder(5);
Cylinder BandKlemmZylinder(6);

Insomnia errorBlinkTimer;
unsigned long blinkDelay = 600;
Insomnia resetTimeout; // reset rig after 40 seconds inactivity
Insomnia resetDelay;
Insomnia coolingDelay;
Insomnia nexResetButtonTimeout;

StateController stateController(numberOfMainCycleSteps);

EEPROM_Counter eepromCounter;

//*****************************************************************************
// EXPERIMENT CLASS PER CYCLE STEP
//*****************************************************************************

//*******************
// ABSTRACT CLASS
//*******************
class CycleStepTemplate
{
public:
  virtual void doStuff();
  void setCycleStepNo(int cycleStepNo)
  {
    _cycleStepNo = cycleStepNo;
  }
  int getCycleStepNo()
  {
    return _cycleStepNo;
    //Serial.println(_cycleStepNo);
  }
  //String displayAString;
private:
  int _cycleStepNo;
};
//*******************
// CONCRETE CLASS
//*******************
class StepWippenhebel : public CycleStepTemplate
{
public:
  String DisplayString = "WIPPENHEBEL";

  void objectMethod()
  {
  }

  void doStuff()
  {
    SchlittenZylinder.stroke(1500, 1000);
    eepromCounter.setup(0, 1023, 20);
    long testerer = eepromCounter.getValue(1);
    if (BandKlemmZylinder.stroke_completed())
    {
      stateController.switchToNextStep();
    }
  }
};

//*******************
// CREATE THE CLASS
//*******************
StepWippenhebel stepWippenhebel;

//*****************************************************************************

void setup()
{

  Serial.begin(115200);

  // CREATE A SETUP ENTRY IN THE LOG:
  stateController.setStepMode();
  Serial.println(" ");
  Serial.println("EXIT SETUP");
}
void loop()
{
  stepWippenhebel.objectMethod();
  stepWippenhebel.doStuff();
  delay(500);
}