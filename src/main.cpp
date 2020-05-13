/*
 * ****************************************************************************
 * PTY SEMI-AUTOMATIC-RIG
 * ****************************************************************************
 * Program for a semi-automatic endurance test rig for a mechanical tool
 * ****************************************************************************
 * Michael Wettstein
 * May 2020, Zürich
 * ****************************************************************************
 */

//#include <Controllino.h>   // PIO Controllino Library // Comment out when using an Arduino
#include <ArduinoSTL.h>      // https://github.com/mike-matera/ArduinoSTL
#include <Nextion.h>         // PIO Nextion Library
#include <SD.h>              // PIO Adafruit SD Library
#include <AliasColino.h>     // Aliases when using an Arduino instead of a Controllino
#include <Cylinder.h>        // https://github.com/chischte/cylinder-library
#include <Debounce.h>        // https://github.com/chischte/debounce-library
#include <Insomnia.h>        // https://github.com/chischte/insomnia-delay-library
#include <EEPROM_Counter.h>  // https://github.com/chischte/eeprom-counter-library
#include <CycleStep.h>       // TODO ADD TO LIBRARY
#include <StateController.h> // https://github.com/chischte/state-controller-library.git


// DECLARE FUNCTIONS:
//*****************************************************************************
void clearTextField(String textField);
void hideInfoField();

// DEFINE NAMES CYCLE COUNTER:
//*****************************************************************************
enum counter
{
  longtime_counter,   //
  shorttime_counter,  //
  bandvorschub_oben,  // [mm]
  bandvorschub_unten, // [mm]
  end_of_counter_enum // Keep this entry
};
int counter_no_of_values = end_of_counter_enum;

// GENERATE INSTANCES OF CLASSES:
//*****************************************************************************
Cylinder zylinder_schlitten(CONTROLLINO_D3);
Cylinder zylinder_entlueften(CONTROLLINO_D4);
Cylinder motor_band_oben(CONTROLLINO_D5);
Cylinder motor_band_unten(CONTROLLINO_D6);
Cylinder motor_bremse(CONTROLLINO_D7);
Cylinder zylinder_messer(CONTROLLINO_D8);

Debounce EndSwitchRight(CONTROLLINO_A0);
Debounce StrapDetectionSensor(A1);
Debounce EndSwitchLeft(CONTROLLINO_A2);

unsigned long blink_delay = 600;
Insomnia nex_reset_button_timeout;
Insomnia errorBlinkTimer;
Insomnia resetTimeout; // reset rig after 40 seconds inactivity
Insomnia reset_delay;

State_controller state_controller;
EEPROM_Counter eeprom_counter;

// NEXTION DISPLAY - OBJECTS
//*****************************************************************************
// PAGE 0:
NexPage nex_page_0 = NexPage(0, 0, "page0");
//PAGE 1 - LEFT SIDE:
NexPage nex_page_1 = NexPage(1, 0, "page1");
NexButton button_previous_step = NexButton(1, 6, "b1");
NexButton button_next_step = NexButton(1, 7, "b2");
NexButton button_reset_cycle = NexButton(1, 5, "b0");
NexDSButton button_play_pause_ds = NexDSButton(1, 3, "bt0");
NexDSButton button_modeswitch_ds = NexDSButton(1, 4, "bt1");
// PAGE 1 - RIGHT SIDE
NexDSButton button_entlueften_ds = NexDSButton(1, 13, "bt3");
NexDSButton button_klemmen_ds = NexDSButton(1, 10, "bt5");
NexButton button_schlitten = NexButton(1, 1, "b6");
NexButton button_motor_oben = NexButton(1, 9, "b4");
NexButton button_schneiden = NexButton(1, 14, "b5");
NexButton button_motor_unten = NexButton(1, 8, "b3");

// PAGE 2 - LEFT SIDE:
NexPage nex_page_2 = NexPage(2, 0, "page2");
NexButton button_slider_1_left = NexButton(2, 5, "b1");
NexButton button_slider_1_right = NexButton(2, 6, "b2");
NexButton button_slider_2_left = NexButton(2, 16, "b5");
NexButton button_slider_2_right = NexButton(2, 17, "b6");

// PAGE 2 - RIGHT SIDE:
NexButton button_reset_shorttime_counter = NexButton(2, 12, "b4");

// NEXTION DISPLAY - TOUCH EVENT LIST
//*****************************************************************************
NexTouch *nex_listen_list[] = { //
    // PAGE 0:
    &nex_page_0,
    // PAGE 1 LEFT:
    &nex_page_1, &button_previous_step, &button_next_step, &button_reset_cycle, &button_play_pause_ds,
    &button_modeswitch_ds,
    // PAGE 1 RIGHT:
    &button_schneiden, &button_klemmen_ds, &button_entlueften_ds, &button_schlitten,
    &button_motor_oben, &button_motor_unten,
    // PAGE 2 LEFT:
    &nex_page_2, &button_slider_1_left, &button_slider_1_right,&nex_page_2, &button_slider_2_left, &button_slider_2_right,
    // PAGE 2 RIGHT:
    &button_reset_shorttime_counter,
      // END OF LISTEN LIST:
    NULL};

// GLOBAL VARIABLES:
//*****************************************************************************

// KNOBS AND POTENTIOMETERS:
const byte TEST_SWITCH_PIN = 2;
Debounce test_switch(TEST_SWITCH_PIN);
const byte MOTOR_RELAY_PIN = 50;

// NEXTION VARIABLES

int current_page=0;
bool reset_stopwatch_is_active;
unsigned int stoppedButtonPushtime;
long nex_prev_bandvorschub_oben;
long nex_prev_shorttime_counter;
long nex_prev_longtime_counter;
long buttonPushStopwatch;
long counter_reset_stopwatch;
byte nex_prev_cycle_step;
byte errorLogPage = 0;
bool nextionPlayPauseButtonState;
bool counterReseted = false;
int currentPage = 0;
unsigned long counterResetStopwatch;

// NEXTION SWITCHSTATES
bool nex_state_entlueftung;
bool nex_state_motorbremse;
bool nex_motor_oben;
bool nex_state_schlitten;
bool nex_state_messer;
bool nex_state_machine_running;
bool nex_state_motor_unten;
bool nex_prev_step_mode = true;

// SENSORS:
// n.a.

// OTHER GLOBAL VARIABLES:
bool strap_detected;
bool error_blink_state = false;
byte timeout_detected = 0;
int cycle_time_in_seconds = 30; // Estimated value for the timout timer

// NON NEXTION FUNCTIONS
//*****************************************************************************
void resetCylinderStates()
{
  zylinder_schlitten.set(0);
  motor_band_oben.set(0);
  motor_band_unten.set(0);
  motor_bremse.set(0);
  zylinder_messer.set(0);
  zylinder_entlueften.set(0);
}

void stopTestRig()
{
  resetCylinderStates();
  state_controller.set_step_mode();
  state_controller.set_machine_stop();
}

void resetTestRig()
{
    state_controller.set_machine_stop();
    resetCylinderStates();
    clearTextField("t4");
    hideInfoField();
    state_controller.set_current_step_to(0);
    
}

// NEXTION GENERAL DISPLAY FUNCTIONS
//*****************************************************************************

void send_to_nextion() {
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

void updateDisplayCounter() {
  long newValue = eeprom_counter.getValue(longtime_counter);
  Serial2.print("t0.txt=");
  Serial2.print("\"");
  Serial2.print(newValue);
  Serial2.print("\"");
  send_to_nextion();
}

void showInfoField()
{
  if (current_page == 1)
  {
    Serial2.print("vis t4,1");
    send_to_nextion();
  }
}

void hideInfoField()
{
  if (current_page == 1)
  {
    Serial2.print("vis t4,0");
    send_to_nextion();
  }
}

void clearTextField(String textField)
{
  Serial2.print(textField);
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(""); // erase text
  Serial2.print("\"");
  send_to_nextion();
}

void printOnValueField(int value, String valueField)
{
  Serial2.print(valueField);
  Serial2.print(".val=");
  Serial2.print(value);
  send_to_nextion();
}

void printCurrentStep()
{
  Serial.print(state_controller.get_current_step());
  Serial.print(" ");
 // Serial.println(cycleName[state_controller.currentCycleStep()]);
}

void print_on_text_field(String text, String textField)
{
  Serial2.print(textField);
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(text);
  Serial2.print("\"");
  send_to_nextion();
}

void nex_switch_play_pausePushCallback(void *ptr) {
  counterResetStopwatch = millis();
  reset_stopwatch_is_active = true;
}

void nex_switch_play_pausePopCallback(void *ptr) {

  if (counterReseted == false) {
    state_controller.toggle_machine_running_state();
  } else {
    //counter has been reseted
    //change of machine state did not happen,
    //therefore switch the button layout back:
    Serial2.print("click bt0,1");        // click button
    send_to_nextion();
    counterReseted = false; // counter reset steps completed
  }
  reset_stopwatch_is_active = false;
}

// TOUCH EVENT FUNCTIONS PAGE 1 - LEFT SIDE
//*************************************************
void button_play_pause_ds_push(void *ptr)
{
  state_controller. toggle_machine_running_state();
  nex_state_machine_running = !nex_state_machine_running;
  
}
void button_modeswitch_ds_push(void *ptr)
{
  if (state_controller.is_in_auto_mode())
  {
    state_controller.set_step_mode();
  }
  else
  {
    state_controller.set_auto_mode();
  }
  nex_prev_step_mode = state_controller.is_in_step_mode();
}
void button_stepback_push(void *ptr)
{
  if (state_controller.get_current_step() > 0)
  {
    state_controller.set_current_step_to(state_controller.get_current_step() - 1);
  }
}
void button_next_step_push(void *ptr)
{
  state_controller.switch_to_next_step();
}
void button_reset_cycle_push(void *ptr)
{
   state_controller.set_reset_mode(1);
    clearTextField("t4");
  hideInfoField();
}

// TOUCH EVENT FUNCTIONS PAGE 1 - RIGHT SIDE
//*************************************************
void button_klemmen_ds_pop(void *ptr)
{
  motor_bremse.toggle();
  nex_state_motorbremse = !nex_state_motorbremse;
}
void button_motor_oben_push(void *ptr)
{
  motor_band_oben.set(1);
}
void button_motor_oben_pop(void *ptr)
{
  motor_band_oben.set(0);
}
void button_motor_unten_push(void *ptr)
{
  motor_band_unten.set(1);
}
void button_motor_unten_pop(void *ptr)
{
  motor_band_unten.set(0);
}
void button_entlueften_ds_push(void *ptr)
{
  zylinder_entlueften.toggle();
  nex_state_entlueftung = !nex_state_entlueftung;
}
void button_schneiden_push(void *ptr)
{
  zylinder_messer.set(1);
}
void button_schneiden_pop(void *ptr)
{
  zylinder_messer.set(0);
}
void button_schlitten_push(void *ptr)
{
  zylinder_schlitten.set(1);
}
void button_schlitten_pop(void *ptr)
{
  zylinder_schlitten.set(0);
}

// TOUCH EVENT FUNCTIONS PAGE 2 - LEFT SIDE
//*************************************************
void button_slider_1_left_push(void *ptr)
{
  byte increment = 10;
  if (eeprom_counter.getValue(bandvorschub_oben) <= 20)
  {
    increment = 5;
  }
  if (eeprom_counter.getValue(bandvorschub_oben) <= 10)
  {
    increment = 1;
  }
  eeprom_counter.set(bandvorschub_oben, eeprom_counter.getValue(bandvorschub_oben) - increment);
  if (eeprom_counter.getValue(bandvorschub_oben) < 4)
  {
    eeprom_counter.set(bandvorschub_oben, 4);
  }
 }
void button_slider_1_right_push(void *ptr)
{
  byte increment = 10;

  if (eeprom_counter.getValue(bandvorschub_oben) < 20)
  {
    increment = 5;
  }
  if (eeprom_counter.getValue(bandvorschub_oben) < 10)
  {
    increment = 1;
  }
  eeprom_counter.set(bandvorschub_oben, eeprom_counter.getValue(bandvorschub_oben) + increment);
  if (eeprom_counter.getValue(bandvorschub_oben) > 120)
  {
    eeprom_counter.set(bandvorschub_oben, 120);
  } 
}
void button_slider_2_left_push(void *ptr)
{
  byte increment = 10;
  if (eeprom_counter.getValue(bandvorschub_oben) <= 20)
  {
    increment = 5;
  }
  if (eeprom_counter.getValue(bandvorschub_oben) <= 10)
  {
    increment = 1;
  }
  eeprom_counter.set(bandvorschub_oben, eeprom_counter.getValue(bandvorschub_oben) - increment);
  if (eeprom_counter.getValue(bandvorschub_oben) < 4)
  {
    eeprom_counter.set(bandvorschub_oben, 4);
  }
 }
void button_slider_2_right_push(void *ptr)
{
  byte increment = 10;

  if (eeprom_counter.getValue(bandvorschub_oben) < 20)
  {
    increment = 5;
  }
  if (eeprom_counter.getValue(bandvorschub_oben) < 10)
  {
    increment = 1;
  }
  eeprom_counter.set(bandvorschub_oben, eeprom_counter.getValue(bandvorschub_oben) + increment);
  if (eeprom_counter.getValue(bandvorschub_oben) > 120)
  {
    eeprom_counter.set(bandvorschub_oben, 120);
  } 
}

// TOUCH EVENT FUNCTIONS PAGE 2 - RIGHT SIDE
//*************************************************
void button_reset_shorttime_counter_push(void *ptr)
{
  eeprom_counter.set(shorttime_counter, 0);

  // RESET LONGTIME COUNTER IF RESET BUTTON IS PRESSED LONG ENOUGH:
  counter_reset_stopwatch = millis();
  reset_stopwatch_is_active = true;
}
void button_reset_shorttime_counter_pop(void *ptr)
{
  reset_stopwatch_is_active = false;
}

// TOUCH EVENT FUNCTIONS PAGE CHANGES
//*************************************************
void nexPage0PushCallback(void *ptr)
{
  current_page = 0;
}
void page1_push(void *ptr)
{
  current_page = 1;
  hideInfoField();

  // REFRESH BUTTON STATES:
  nex_prev_cycle_step = !state_controller.get_current_step();
  nex_prev_step_mode = true;
  nex_state_entlueftung = 0;
  nex_state_motorbremse = 0;
  nex_motor_oben = 0;
  nex_state_schlitten = 0;
  nex_state_messer = 0;
  nex_state_motor_unten = 0;
  nex_state_machine_running = 0;
}
void nex_page_2_push(void *ptr)
{
  current_page = 2;
  // REFRESH BUTTON STATES:
  nex_prev_bandvorschub_oben = 0;
  nex_prev_shorttime_counter = 0;
  nex_prev_longtime_counter = 0;
}
//*****************************************************************************
void setupEventCallbackFunctions(){
  // PAGE 0 PUSH ONLY:
  nex_page_0.attachPush(nexPage0PushCallback);
  // PAGE 1 PUSH ONLY:
  nex_page_1.attachPush(page1_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  button_reset_cycle.attachPush(button_reset_cycle_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  button_modeswitch_ds.attachPush(button_modeswitch_ds_push);
  button_play_pause_ds.attachPush(button_play_pause_ds_push);
  button_klemmen_ds.attachPush(button_klemmen_ds_pop);
  button_entlueften_ds.attachPush(button_entlueften_ds_push);
  // PAGE 1 PUSH AND POP:
  button_motor_oben.attachPush(button_motor_oben_push);
  button_motor_oben.attachPop(button_motor_oben_pop);
  button_motor_unten.attachPush(button_motor_unten_push);
  button_motor_unten.attachPop(button_motor_unten_pop);
  button_schneiden.attachPush(button_schneiden_push);
  button_schneiden.attachPop(button_schneiden_pop);
  button_schlitten.attachPush(button_schlitten_push);
  button_schlitten.attachPop(button_schlitten_pop);
  // PAGE 2 PUSH ONLY:
  nex_page_2.attachPush(nex_page_2_push);
  button_slider_1_left.attachPush(button_slider_1_left_push);
  button_slider_1_right.attachPush(button_slider_1_right_push);
  button_slider_2_left.attachPush(button_slider_2_left_push);
  button_slider_2_right.attachPush(button_slider_2_right_push);
  // PAGE 2 PUSH AND POP:
  button_reset_shorttime_counter.attachPush(button_reset_shorttime_counter_push);
  button_reset_shorttime_counter.attachPop(button_reset_shorttime_counter_pop);
  }
//*****************************************************************************
void display_loop_page_1_left_side(){

    // UPDATE CYCLE NAME:
    if (nex_prev_cycle_step != state_controller.get_current_step())
    {
      nex_prev_cycle_step = state_controller.get_current_step();
      print_on_text_field("???","t0");
          //(state_controller.currentCycleStep() + 1) + (" " + cycleName[state_controller.currentCycleStep()]), "t0");
    }
    // UPDATE SWITCHSTATE "PLAY"/"PAUSE":
    if (nex_state_machine_running != state_controller.machine_is_running())
    {
      Serial2.print("click bt0,1");
      send_to_nextion();
      nex_state_machine_running = state_controller.machine_is_running();
    }

    // UPDATE SWITCHSTATE "STEP"/"AUTO"-MODE:
    if (nex_prev_step_mode != state_controller.is_in_step_mode())
    {
      Serial2.print("click bt1,1");
      send_to_nextion();
      nex_prev_step_mode = state_controller.is_in_step_mode();
    }
}
void display_loop_page_1_right_side(){

    //*******************
    // PAGE 1 - RIGHT SIDE:
    //*******************

    // UPDATE SWITCHBUTTON (dual state):
    if (zylinder_entlueften.request_state() != nex_state_entlueftung)
    {
      Serial2.print("click bt3,1");
      send_to_nextion();
      nex_state_entlueftung = !nex_state_entlueftung;
    }
    // UPDATE SWITCHBUTTON (dual state):
    if (motor_bremse.request_state() != nex_state_motorbremse)
    {
      Serial2.print("click bt5,1");
      send_to_nextion();
      nex_state_motorbremse = !nex_state_motorbremse;
    }

    // UPDATE BUTTON (momentary):
    if (zylinder_schlitten.request_state() != nex_state_schlitten)
    {
      if (zylinder_schlitten.request_state())
      {
        Serial2.print("click b6,1");
      }
      else
      {
        Serial2.print("click b6,0");
      }
      send_to_nextion();
      nex_state_schlitten = zylinder_schlitten.request_state();
    }

    // UPDATE BUTTON (momentary):
    if (motor_band_oben.request_state() != nex_motor_oben)
    {
      if (motor_band_oben.request_state())
      {
        Serial2.print("click b4,1");
      }
      else
      {
        Serial2.print("click b4,0");
      }
      send_to_nextion();
      nex_motor_oben = motor_band_oben.request_state();
    }

    // UPDATE BUTTON (momentary):
    if (zylinder_messer.request_state() != nex_state_messer)
    {
      if (zylinder_messer.request_state())
      {
        Serial2.print("click b5,1");
      }
      else
      {
        Serial2.print("click b5,0");
      }
      send_to_nextion();
      nex_state_messer = zylinder_messer.request_state();
    }

    // UPDATE BUTTON (momentary):
    if (motor_band_unten.request_state() != nex_state_motor_unten)
    {
      if (motor_band_unten.request_state())
      {
        Serial2.print("click b3,1");
      }
      else
      {
        Serial2.print("click b3,0");
      }
      send_to_nextion();
      nex_state_motor_unten = motor_band_unten.request_state();
    }
}
void display_loop_page_2_left_side(){

}
void display_loop_page_2_right_side(){
 {

     //*******************
    // PAGE 2 - RIGHT SIDE
    //*******************
    if (nex_prev_longtime_counter != eeprom_counter.getValue(longtime_counter))
    {

      print_on_text_field(String(eeprom_counter.getValue(longtime_counter)), "t10");
      //PrintOnTextField((eepromCounter.getValue(longtime_counter) + ("")), "t10");
      nex_prev_longtime_counter = eeprom_counter.getValue(longtime_counter);
    }
    if (nex_prev_shorttime_counter != eeprom_counter.getValue(shorttime_counter))
    {
      print_on_text_field(String(eeprom_counter.getValue(shorttime_counter)), "t12");
      nex_prev_shorttime_counter = eeprom_counter.getValue(shorttime_counter);
    }
    if (reset_stopwatch_is_active)
    {
      if (millis() - counter_reset_stopwatch > 5000)
      {
        eeprom_counter.set(longtime_counter, 0);
      }
    }
  } // END PAGE 2
}
//*****************************************************************************
void nextionSetup(){
//*****************************************************************************
  Serial2.begin(9600);

  // RESET NEXTION DISPLAY: (refresh display after PLC restart)
  send_to_nextion(); // needed for unknown reasons
  Serial2.print("rest");
  send_to_nextion();

  setupEventCallbackFunctions();
  
  delay(2000);
  sendCommand("page 1"); // switch display to page x
  send_to_nextion();
} 
//*****************************************************************************
void nextionLoop(){
//*****************************************************************************

  nexLoop(nex_listen_list); // check for any touch event

  if (current_page == 1) {
    display_loop_page_2_left_side();
    display_loop_page_2_right_side();
  }

   if (current_page == 2){
     display_loop_page_2_left_side();
     display_loop_page_2_right_side();
  }
}
//*****************************************************************************

// CLASSES FOR THE MAIN CYCLE STEPS
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
    motor_band_oben.stroke(1500, 1000);
    if (motor_band_oben.stroke_completed())
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
//*****************************************************************************
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
    //std::cout << "THIS IS WHAT IT DOES: ";
    cycle_steps[state_controller.get_current_step()]->do_stuff();
    //std::cout << "---------------\n\n";
  }
 
}
//*****************************************************************************