#include <Arduino.h>

#include <Controllino.h>    // https://github.com/CONTROLLINO-PLC/CONTROLLINO_Library
#include <Cylinder.h>       // https://github.com/chischte/cylinder-library
#include <Debounce.h>       // https://github.com/chischte/debounce-library
#include <Insomnia.h>       // https://github.com/chischte/insomnia-delay-library
#include <EEPROM_Counter.h> // https://github.com/chischte/eeprom-counter-library
#include <EEPROM_Logger.h>  // https://github.com/chischte/eeprom-logger-library.git
#include <Nextion.h>        // https://github.com/itead/ITEADLIB_Arduino_Nextion

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
Cylinder SchlittenZylinder(CONTROLLINO_D3);
Cylinder BandKlemmZylinder(CONTROLLINO_D4);
Cylinder SpanntastenZylinder(CONTROLLINO_D5);
Cylinder SchweisstastenZylinder(CONTROLLINO_D6);
Cylinder WippenhebelZylinder(CONTROLLINO_D7);
Cylinder MesserZylinder(CONTROLLINO_D8);

Insomnia errorBlinkTimer;
unsigned long blinkDelay = 600;
Insomnia resetTimeout; // reset rig after 40 seconds inactivity
Insomnia resetDelay;
Insomnia coolingDelay;
Insomnia nexResetButtonTimeout;

StateController stateController(numberOfMainCycleSteps);

EEPROM_Counter eepromCounter;
EEPROM_Logger errorLogger;

//******************************************************************************
long mergeCurrentTime()
{
  long mergedTime = 0;

  // GET THE CURRENT TIME:
  int hour = Controllino_GetHour();
  int minute = Controllino_GetMinute();
  long second = Controllino_GetSecond();

  // MERGE (HOUR 5bit / MINUTE 6bit / SECOND 6bit):
  mergedTime = hour;
  mergedTime = (mergedTime << 6) | minute; // move 6 bits minute
  mergedTime = (mergedTime << 6) | second; // move 6 bits second
  return mergedTime;
}

void writeErrorLog(byte errorCode)
{
  long cycleNumber = eepromCounter.getValue(shorttimeCounter);
  long logTime = mergeCurrentTime();
  errorLogger.writeLog(cycleNumber, logTime, errorCode);
}

//*****************************************************************************

void setup()
{

  Serial.begin(115200);

  // CREATE A SETUP ENTRY IN THE LOG:
  writeErrorLog(toolResetError);
  stateController.setStepMode();
  resetTimeout.setTime((eepromCounter.getValue(coolingTime) + cycleTimeInSeconds) * 1000);
  Serial.println(" ");
  Serial.println("EXIT SETUP");
}
void loop()
{
}

//*****************************************************************************
// EXPERIMENT CLASS PER CYCLE STEP
//*****************************************************************************

//*******************
// ABSTRACT CLASS
//*******************
class cycleStep
{
public:
  //virtual void doStuff();
  //String displayAString;
};
//*******************
// CONCRETE CLASS
//*******************
class WippenhebelStep : public cycleStep
{
public:
  String DisplayString = "WIPPENHEBEL";

  void objectMethod()
  {
  }

  void doStuff()
  {
    WippenhebelZylinder.stroke(1500, 1000);
    eepromCounter.setup(0, 1023, 20);
    long testerer = eepromCounter.getValue(1);
    if (WippenhebelZylinder.stroke_completed())
    {
      stateController.switchToNextStep();
    }
  }
};
//*******************
// CREATE THE CLASS
//*******************
WippenhebelStep wippenhebelObject;

//*******************
// USE THE CLASS
//*******************
void useObject()
{
  wippenhebelObject.objectMethod();
  wippenhebelObject.doStuff();
}