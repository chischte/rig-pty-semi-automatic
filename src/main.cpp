#include <ArduinoSTL.h>      // https://github.com/mike-matera/ArduinoSTL
#include <Cylinder.h>        // https://github.com/chischte/cylinder-library
#include <Debounce.h>        // https://github.com/chischte/debounce-library
#include <Insomnia.h>        // https://github.com/chischte/insomnia-delay-library
#include <EEPROM_Counter.h>  // https://github.com/chischte/eeprom-counter-library
#include <EEPROM_Logger.h>   // https://github.com/chischte/eeprom-logger-library.git
#include <CycleStep.h>       //
#include <StateController.h> // https://github.com/chischte/state-controller-library.git

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

String error_code[] = { //
    "n.a.",             //
    "STEUERUNG EIN",    //
    "AUTO RESET",       //
    "AUTO PAUSE",       //
    "AUTO STOP",        //
    "BAND LEER",        //
    "MANUELL START",    //
    "MANUELL STOP"};    //

int logger_no_of_logs = 50;

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
Cylinder cylinder_schlitten(5);
Cylinder cylinder_bandklemme(6);

Insomnia errorBlinkTimer;
unsigned long blinkDelay = 600;
Insomnia resetTimeout; // reset rig after 40 seconds inactivity
Insomnia resetDelay;
Insomnia coolingDelay;
Insomnia nexResetButtonTimeout;

StateController stateController;
EEPROM_Counter eepromCounter;
//******************************************************************************
// WRITE CLASSES FOR THE MAIN CYCLE STEPS
//******************************************************************************
//------------------------------------------------------------------------------
class StepWippenhebel : public CycleStep
{
public:
  String getDisplayString() { return "WIPPENHEBEL"; }
  void doStuff()
  {
    cylinder_schlitten.stroke(1500, 1000);
    eepromCounter.setup(0, 1023, 20);
    if (cylinder_bandklemme.stroke_completed())
    {
      stateController.switchToNextStep();
    }
    Serial.println("Class I bytes ya tooth");
  }

private:
};
//------------------------------------------------------------------------------
class StepBandKlemmen : public CycleStep
{
public:
  String getDisplayString() { return "KLEMMEN"; }
  void doStuff()
  {
    Serial.println("Class II bytes me teeth");
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
std::vector<CycleStep *> cycleSteps;

// POINTER TO OBJECT SOLUTION:

//*****************************************************************************

void setup()
{
  //------------------------------------------------
  // PUSH THE CYCLE STEPS INTO THE VECTOR CONTAINER:
  // THE SEQUENCE IS IMPORTANT!
  //------------------------------------------------
  cycleSteps.push_back(new StepWippenhebel);
  cycleSteps.push_back(new StepBandKlemmen);
  //------------------------------------------------
  // Get number of cycles from parent class:
  int no_of_cycle_steps = CycleStep::objectCount;
  stateController.setNumberOfSteps(no_of_cycle_steps);
  Serial.begin(115200);
  Serial.println(CycleStep::objectCount);

  // CREATE A SETUP ENTRY IN THE LOG:
    Serial.println("EXIT SETUP");
}
void loop()
{
  int no_of_cycle_steps = CycleStep::objectCount;
  // TEST LOOP TO ITERATE THROUGH ALL CYCLE STEPS:

  for (int i = 0; i < no_of_cycle_steps; i++)
  {
    std::cout << "CYCLE STEP I:";
    cycleSteps[i]->doStuff();
    Serial.println(cycleSteps[i]->getDisplayString());
    delay(3000);
  }
}