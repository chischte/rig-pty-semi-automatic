#include <ArduinoSTL.h>      // https://github.com/mike-matera/ArduinoSTL
#include <Cylinder.h>        // https://github.com/chischte/cylinder-library
#include <Debounce.h>        // https://github.com/chischte/debounce-library
#include <Insomnia.h>        // https://github.com/chischte/insomnia-delay-library
#include <EEPROM_Counter.h>  // https://github.com/chischte/eeprom-counter-library
#include <CycleStep.h>       //
#include <StateController.h> // https://github.com/chischte/state-controller-library.git

//******************************************************************************
// DEFINE NAMES AND SET UP VARIABLES FOR THE CYCLE COUNTER:
//******************************************************************************
enum counter
{
  longtime_counter,   //
  shorttime_counter,  //
  cooling_time,       // [s]
  end_of_counter_enum // Keep this entry
};

int counter_no_of_values = end_of_counter_enum;

//******************************************************************************
// DECLARATION OF VARIABLES
//******************************************************************************
bool strap_detected;
bool error_blink_state = false;
byte timeout_detected = 0;
int cycle_time_in_seconds = 30; // Estimated value for the timout timer

//******************************************************************************
// GENERATE INSTANCES OF CLASSES:
//******************************************************************************
Cylinder cylinder_schlitten(5);
Cylinder cylinder_bandklemme(6);

Insomnia error_blink_timer;
unsigned long blink_delay = 600;
Insomnia nex_reset_button_timeout;

State_controller state_controller;
EEPROM_Counter eeprom_counter;
//******************************************************************************
// WRITE CLASSES FOR THE MAIN CYCLE STEPS
//******************************************************************************
// TODO: WRITE CLASSES FOR MAIN CYCLE STEPS:
// Step_feed_upper_strap
// Step_feed_lower_strap
// Step_apply_tension
// -->include timeout function
// -->manually activate brake
// Step_release_tension
// Step_cut_strap
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
  state_controller.set_machine_running(1);
  Serial.println("EXIT SETUP");
  //------------------------------------------------
}
void loop()
{

  // TODO:
  // Fix bug picture does not show in README.md
  // Read: https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
  // Implement Nextion, make button state monitoring more elegant
  // Implement sub step possibility
  // Implement timeout possibility, if smart, to abstract cycle step class

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

  delay(1000);
}