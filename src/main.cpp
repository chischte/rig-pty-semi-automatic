#include <ArduinoSTL.h>      // https://github.com/mike-matera/ArduinoSTL
#include <Cylinder.h>        // https://github.com/chischte/cylinder-library
#include <Debounce.h>        // https://github.com/chischte/debounce-library
#include <Insomnia.h>        // https://github.com/chischte/insomnia-delay-library
#include <EEPROM_Counter.h>  // https://github.com/chischte/eeprom-counter-library
#include <EEPROM_Logger.h>   // https://github.com/chischte/eeprom-logger-library.git
#include <CycleStep.h>       //
#include <StateController.h> // https://github.com/chischte/state-controller-library.git

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
//******************************************************************************
// WRITE CLASSES FOR THE MAIN CYCLE STEPS
//******************************************************************************
class StepWippenhebel : public CycleStep
{
public:
  void doStuff() override
  {
    SchlittenZylinder.stroke(1500, 1000);
    eepromCounter.setup(0, 1023, 20);
    if (BandKlemmZylinder.stroke_completed())
    {
      stateController.switchToNextStep();
    }
    Serial.println("Class I does stuff");
  }

private:
};

class StepBandKlemmen : public CycleStep
{
public:
  void doStuff() override
  {
    Serial.println("Class II does stuff");
  }

private:
};

//******************************************************************************
// CREATE CYCLE STEPS OBJECTS AND STORE THEM IN AN ARRAY
// TODO:
//  READ LINKS:
//  https://gamedev.stackexchange.com/questions/168841/c-create-array-of-multiple-types
//  https://stackoverflow.com/questions/1579786/are-array-of-pointers-to-different-types-possible-in-c
//  http://www.infobrother.com/Tutorial/C++/C++_Pointer_Object
//******************************************************************************
// Initialize the object count variable first:
int CycleStep::objectCount = 0;
// Create vector container
std::vector<CycleStep *> pointers;

// POINTER TO OBJECT SOLUTION:

// Get number of cycles from parent class:
int noOfCycleSteps = CycleStep::objectCount;

//*****************************************************************************

void setup()
{
  // PUSH THE CYCLE STEPS INTO THE VECTOR CONTAINER:
  // THE SEQUENCE IS IMPORTANT
  pointers.push_back(new StepWippenhebel);
  pointers.push_back(new StepBandKlemmen);
  Serial.begin(115200);
  Serial.println(CycleStep::objectCount);

  // CREATE A SETUP ENTRY IN THE LOG:
  stateController.setStepMode();
  Serial.println("EXIT SETUP");
}
void loop()
{
  //pointers[0]->doStuff();
  delay(1200);
  //pointers[1]->doStuff();
  delay(1200);
}