/*
 * ****************************************************************************
 * RIG TEMPLATR
 * ****************************************************************************
 * Program for an endurance test of a mechanical switch
 * The switch will be pushed 100'000 times or more.
 * The number of pushes will be counted and displayed.
 * ****************************************************************************
 * Michael Wettstein
 * May 2020, Zürich
 * ****************************************************************************
 */


#include <ArduinoSTL.h>      // https://github.com/mike-matera/ArduinoSTL
//#include <Controllino.h>   // PIO Controllino Library // Comment out when using an Arduino
#include <Nextion.h>         // PIO Nextion Library
#include <SD.h>              // PIO Adafruit SD Library
#include <AliasColino.h>     // Aliases when using an Arduino instead of a Controllino
#include <Cylinder.h>        // https://github.com/chischte/cylinder-library
#include <Debounce.h>        // https://github.com/chischte/debounce-library
#include <Insomnia.h>        // https://github.com/chischte/insomnia-delay-library
#include <EEPROM_Counter.h>  // https://github.com/chischte/eeprom-counter-library
#include <CycleStep.h>       // TODO ADD TO LIBRARY
#include <StateController.h> // https://github.com/chischte/state-controller-library.git

//*****************************************************************************
// DEFINE NAMES AND SET UP VARIABLES FOR THE CYCLE COUNTER:
//*****************************************************************************
enum counter
{
  longtime_counter,   //
  shorttime_counter,  //
  switch_counter,       // [s]
  end_of_counter_enum // Keep this entry
};

int counter_no_of_values = end_of_counter_enum;

//*****************************************************************************
// DECLARATION OF VARIABLES
//*****************************************************************************
bool strap_detected;
bool error_blink_state = false;
byte timeout_detected = 0;
int cycle_time_in_seconds = 30; // Estimated value for the timout timer

//*****************************************************************************
// GENERATE INSTANCES OF CLASSES:
//*****************************************************************************
Cylinder cylinder_schlitten(CONTROLLINO_D5);
Cylinder cylinder_bandklemme(CONTROLLINO_D7);

Insomnia error_blink_timer;
unsigned long blink_delay = 600;
Insomnia nex_reset_button_timeout;

State_controller state_controller;
EEPROM_Counter eeprom_counter;
//*****************************************************************************

//*****************************************************************************
// NEXTION DISPLAY - DECLARATION OF VARIABLES
//*****************************************************************************
bool resetStopwatchActive = false;
bool nextionPlayPauseButtonState;
bool counterReseted = false;
int currentPage = 0;
unsigned long counterResetStopwatch;
char buffer[100] = { 0 }; // This is needed only if you are going to receive a text from the display.
//*****************************************************************************
//*****************************************************************************
// NEXTION DISPLAY - DECLARATION OF OBJECTS TO BE READ
//*****************************************************************************
// PAGE 0:
NexDSButton nex_switch_play_pause = NexDSButton(0, 2, "bt0");
//*****************************************************************************
//*****************************************************************************
// NEXTION DISPLAY - TOUCH EVENT LIST
//*****************************************************************************
NexTouch *nex_listen_list[] = { &nex_switch_play_pause,

NULL //String terminated
        };
//*****************************************************************************
//*****************************************************************************





// KNOBS AND POTENTIOMETERS:
const byte TEST_SWITCH_PIN = 2;
Debounce test_switch(TEST_SWITCH_PIN);
const byte MOTOR_RELAY_PIN = 50;

// SENSORS:
// n.a.

// OTHER VARIABLES:
bool previousButtonState;
bool previousMachineState;
bool machineRunning = false;
bool buttonBlinkEnabled = false;




void send_to_nextion() {
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}


//*****************************************************************************
void updateDisplayCounter() {
  long newValue = eeprom_counter.getValue(longtime_counter);
  Serial2.print("t0.txt=");
  Serial2.print("\"");
  Serial2.print(newValue);
  Serial2.print("\"");
  send_to_nextion();
}

//*****************************************************************************
// NEXTION DISPLAY - TOUCH EVENT FUNCTIONS
//*****************************************************************************

void nex_switch_play_pausePushCallback(void *ptr) {
  counterResetStopwatch = millis();
  resetStopwatchActive = true;
}

void nex_switch_play_pausePopCallback(void *ptr) {

  if (counterReseted == false) {
    machineRunning = !machineRunning;
  } else {
    //counter has been reseted
    //change of machine state did not happen,
    //therefore switch the button layout back:
    Serial2.print("click bt0,1");        // click button
    send_to_nextion();
    counterReseted = false; // counter reset steps completed
  }
  resetStopwatchActive = false;
}




//*****************************************************************************
void nextionSetup()
//*****************************************************************************
{
  Serial.println("START OF NEXTION SETUP");
  Serial2.begin(9600);  // Start serial comunication at baud=9600

  //*****************************************************************************
  // NEXTION DISPLAY - REGISTER THE EVENT CALLBACK FUNCTIONS
  //*****************************************************************************
  //*****PUSH::
  // n.a.

  //*****PUSH+POP:
  nex_switch_play_pause.attachPush(nex_switch_play_pausePushCallback);
  nex_switch_play_pause.attachPop(nex_switch_play_pausePopCallback);
  //*****************************************************************************
  delay(2000);
  send_to_nextion();

  // TOGGLE APPEARANCE OF PLAY/PAUSE BUTTON:
  Serial2.print("click bt0,1");        // click button
  send_to_nextion();

  Serial.println("END OF NEXTION SETUP");

}  // END OF NEXTION SETUP

//*****************************************************************************
void nextionLoop()
//*****************************************************************************
{
  nexLoop(nex_listen_list); //check for any touch event

  if (currentPage == 0) {
    //********************************
    // PAGE 0:
    //********************************
    // RESET COUNTER:
    if (resetStopwatchActive == true) {
      if (millis() - counterResetStopwatch > 3000) {
        eeprom_counter.set(longtime_counter, 0);
        updateDisplayCounter();
        counterReseted = true;
      }
    }
  }
}    //END OF NEXTION LOOP


//*****************************************************************************
// END OF TOUCH EVENT FUNCTIONS
//*****************************************************************************

//******************************************************************************
// WRITE CLASSES FOR THE MAIN CYCLE STEPS
//******************************************************************************
// TODO: WRITE CLASSES FOR MAIN CYCLE STEPS:
// feed_upper_strap
// feed_lower_strap
// brake (tool can work)
// release air 
// cut (open gate, then cut))
// release brake
// move sledge back
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class Step_wippenhebel : public Cycle_step
{
public:
  char *get_display_text()
  {
    char *display_text = (char *)malloc(20);
    strcpy(display_text, "WIPPENHEBEL");
    return display_text;
  }
  void do_stuff()
  {
    cylinder_schlitten.stroke(1500, 1000);
    if (cylinder_bandklemme.stroke_completed())
    {
      std::cout << "Class I bytes ya tooth\n";
      set_completed();
    }
  }

private:
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// class Step_band_klemmen : public Cycle_step
// {
// public:
//   char* get_display_string() { return "KLEMMEN"; }
//   void do_stuff()
//   {
//     Serial.println("Class II bytes me teeth");
//     set_completed();
//   }

// private:
// };
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//******************************************************************************
// CREATE VECTOR CONTAINER FOR THE CYCLE STEPS OBJECTS
//******************************************************************************
int Cycle_step::object_count = 0; // enable object counting
std::vector<Cycle_step *> cycle_steps;
//*****************************************************************************

void setup()
{
  //------------------------------------------------
  // PUSH THE CYCLE STEPS INTO THE VECTOR CONTAINER:
  // PUSH SEQUENCE = CYCLE SEQUENCE!
  cycle_steps.push_back(new Step_wippenhebel);
  //cycle_steps.push_back(new Step_band_klemmen);
  //------------------------------------------------
  // CONFIGURE THE STATE CONTROLLER:
  int no_of_cycle_steps = Cycle_step::object_count;
  state_controller.set_no_of_steps(no_of_cycle_steps);
  //------------------------------------------------
  // SETUP COUNTER:
  eeprom_counter.setup(0, 1023, counter_no_of_values);
  //------------------------------------------------
  Serial.begin(115200);
  state_controller.set_auto_mode();
  state_controller.set_machine_running();
  pinMode(TEST_SWITCH_PIN,INPUT_PULLUP); // DEACTIVATE FOR CONTROLLINO!
  Serial.println("EXIT SETUP");
  //------------------------------------------------
   nextionSetup();
}
void loop()
{

  // TODO:
  // Read: https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
  // Implement Nextion, make button state monitoring more elegant
  // Implement sub step possibility
  // Implement timeout possibility, if smart, to abstract cycle step class

   // GET INFOS FROM TOUCH DISPLAY:
  nextionLoop();

  // IF STEP IS COMPLETED SWITCH TO NEXT STEP:
  if (cycle_steps[state_controller.get_current_step()]->is_completed())
  {
    int current_step = state_controller.get_current_step();
    state_controller.switch_to_next_step();
    char *display_text = cycle_steps[current_step]->get_display_text();
    std::cout << "NEXT STEP NUMBER: " << current_step << "\n";
    std::cout << "NEXT STEP NAME: " << display_text << "\n";
  }

  // RESET RIG IF RESET IS ACTIVATED:
  if (state_controller.reset_mode_is_active())
  {
    //   reset_test_rig();
  }

  // IN STEP MODE, THE RIG STOPS AFTER EVERY COMPLETED STEP:
  if (state_controller.step_switch_has_happend())
  {
    if (state_controller.is_in_step_mode())
    {
      state_controller.set_machine_running(false);
    }
  }

  // IF MACHINE STATE IS "RUNNING", RUN CURRENT STEP:
  if (state_controller.machine_is_running())
  {
    std::cout << "THIS IS WHAT IT DOES: ";
    cycle_steps[state_controller.get_current_step()]->do_stuff();
    std::cout << "---------------\n\n";
  }

  // GET SIGNAL FROM TEST SWITCH AND COUNT IT:
  bool debouncedButtonState = test_switch.requestButtonState();
  if (previousButtonState != debouncedButtonState) {
    if (debouncedButtonState == LOW) {
      eeprom_counter.countOneUp(longtime_counter);
      updateDisplayCounter();
      
    }
    previousButtonState = debouncedButtonState;
  }


 delay(1000);
}