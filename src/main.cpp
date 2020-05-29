/*
 * *****************************************************************************
 * PTY-SEMI-AUTOMATIC-RIG
 * *****************************************************************************
 * Program for a semi-automatic endurance test rig for a mechanical tool
 * *****************************************************************************
 * Michael Wettstein
 * May 2020, Zürich
 * *****************************************************************************
 * TODO:
 * 
 * 
 * ? Implement on "on init" function in abstract class Cycle_step
 * Check if traffic light display updates only when a state change happend
 * Implement stepper motor driver library and code
 * Delete AccelStepper library from arduino_sketch folder
 * eeprom counter is not a counter but a rememberer!
 * Split insomnia in two libraries (delay and timeout)
 * Install code check tools
 * Find out why empty constructor of insomnia does not work with default value
 * ? Implement sub step possibility
 * ? Implement timeout possibility, if smart, to abstract cycle step class
 * Read:  
 * https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 */

// INCLUDE HEADERS *************************************************************

#include <ArduinoSTL.h> //          https://github.com/mike-matera/ArduinoSTL
//#include <Controllino.h> // PIO Controllino Library, comment out for Arduino
// deactivate input pullup when using controllino
#include <AccelStepper.h> //        PIO library // https://www.airspayce.com/mikem/arduino/AccelStepper
#include <Cylinder.h> //            https://github.com/chischte/cylinder-library
#include <Debounce.h> //            https://github.com/chischte/debounce-library
#include <EEPROM_Counter.h> //      https://github.com/chischte/eeprom-counter-library
#include <Insomnia.h> //            https://github.com/chischte/insomnia-delay-library
#include <Nextion.h> //             PIO Nextion library
#include <SD.h> //                  PIO Adafruit SD library
#include <alias_colino.h> //        aliases when using an Arduino instead of a Controllino
#include <cycle_step.h> //          blueprint of a cycle step
#include <state_controller.h> //    keeps track of machine states
#include <traffic_light.h> //       keeps track of user infos, manages text and colors

// DECLARE FUNCTIONS IF NEEDED FOR THE COMPILER: *******************************

void clear_text_field(String textField);
void hide_info_field();
void page_0_push(void *ptr);
void page_1_push(void *ptr);
void page_2_push(void *ptr);
void display_loop_page_1_left_side();
void display_loop_page_1_right_side();
void display_loop_page_2_left_side();
void display_loop_page_2_right_side();
void update_traffic_light_field();
void set_traffic_light_field_text(String text);
void set_traffic_light_field_color(String color);
void update_cycle_name();
void update_upper_slider_value();
void update_lower_slider_value();
void update_upper_counter_value();
void update_lower_counter_value();
void reset_lower_counter_value();
void increase_slider_value(int eeprom_value_number);
void decrease_slider_value(int eeprom_value_number);
void update_field_values_page_2();
String get_display_string();
String add_suffix_to_eeprom_value(int eeprom_value_number, String suffix);

// DEFINE NAMES FOR THE CYCLE COUNTER ******************************************

enum counter {
  longtime_counter, //
  shorttime_counter, //
  upper_strap_feed, // [mm]
  lower_strap_feed, // [mm]
  end_of_counter_enum // keep this entry
};
int counter_no_of_values = end_of_counter_enum;

// DECLARE VARIABLES, PINS AND OBJECTS FOR THE STEPPER MOTORS ******************
const byte micro_step_factor = 2;
const byte stepper_direction_factor = 1; // set -1 to change direction

const byte UPPER_MOTOR_STEP_PIN = CONTROLLINO_D0;
const byte UPPER_MOTOR_DIRECTION_PIN = CONTROLLINO_D1;
const byte UPPER_MOTOR_ENABLE_PIN = CONTROLLINO_D2;
AccelStepper upper_motor(1, UPPER_MOTOR_STEP_PIN,
                         UPPER_MOTOR_DIRECTION_PIN); // 1 = driver mode

const byte LOWER_MOTOR_STEP_PIN = CONTROLLINO_D3;
const byte LOWER_MOTOR_DIRECTION_PIN = CONTROLLINO_D4;
const byte LOWER_MOTOR_ENABLE_PIN = CONTROLLINO_D5;
AccelStepper lower_motor(1, LOWER_MOTOR_STEP_PIN,
                         LOWER_MOTOR_DIRECTION_PIN); // 1 = driver mode

// GENERATE OBJECTS ************************************************************

EEPROM_Counter counter;
State_controller state_controller;
Traffic_light traffic_light;

// VISIER D15
// SCHLITTEN D14
// SCHLITTEN D13
// MESSER D12

Cylinder cylinder_sledge(CONTROLLINO_D13);
Cylinder cylinder_vent(CONTROLLINO_D14);
Cylinder cylinder_blade(CONTROLLINO_D12);
Cylinder cylinder_frontclap(CONTROLLINO_D15);
Cylinder motor_upper_brake(CONTROLLINO_D1);
Cylinder motor_lower_brake(CONTROLLINO_D0);

const byte TEST_SWITCH_PIN = 11; // needed for the temporary pullup
Debounce test_switch_mega(TEST_SWITCH_PIN);
Debounce sensor_upper_strap(CONTROLLINO_A0);
Debounce sensor_lower_strap(CONTROLLINO_A1);

Insomnia motor_enable_and_brake_timeout; // to prevent overheating
Insomnia nex_reset_button_timeout(3000); // pushtime to reset counter
Insomnia print_interval_timeout(500);

// NEXTION DISPLAY OBJECTS *****************************************************

// PAGE 0 ----------------------------------------------------------------------
NexPage nex_page_0 = NexPage(0, 0, "page0");
// PAGE 1 - LEFT SIDE ----------------------------------------------------------
NexPage nex_page_1 = NexPage(1, 0, "page1");
NexButton button_previous_step = NexButton(1, 6, "b1");
NexButton button_next_step = NexButton(1, 7, "b2");
NexButton button_reset_cycle = NexButton(1, 5, "b0");
NexButton button_traffic_light = NexButton(1, 15, "b8");
NexDSButton switch_step_auto_mode = NexDSButton(1, 4, "bt1");
// PAGE 1 - RIGHT SIDE ---------------------------------------------------------
NexButton button_upper_motor = NexButton(1, 9, "b4");
NexButton button_lower_motor = NexButton(1, 8, "b3");
NexButton button_blade = NexButton(1, 14, "b5");
NexButton button_sledge = NexButton(1, 1, "b6");
NexDSButton switch_motor_brake = NexDSButton(1, 10, "bt5");
NexDSButton switch_air_release = NexDSButton(1, 13, "bt3");
// PAGE 2 - LEFT SIDE ----------------------------------------------------------
NexPage nex_page_2 = NexPage(2, 0, "page2");
NexButton button_slider_1_left = NexButton(2, 5, "b1");
NexButton button_slider_1_right = NexButton(2, 6, "b2");
NexButton button_slider_2_left = NexButton(2, 16, "b5");
NexButton button_slider_2_right = NexButton(2, 17, "b6");
// PAGE 2 - RIGHT SIDE ---------------------------------------------------------
NexButton button_reset_shorttime_counter = NexButton(2, 12, "b4");

// NEXTION DISPLAY - TOUCH EVENT LIST ******************************************

NexTouch *nex_listen_list[] = { //
    // PAGE 0:
    &nex_page_0,
    // PAGE 1 LEFT:
    &nex_page_1, &button_previous_step, &button_next_step, &button_reset_cycle,
    &button_traffic_light, &switch_step_auto_mode,
    // PAGE 1 RIGHT:
    &button_blade, &switch_motor_brake, &switch_air_release, &button_sledge, &button_upper_motor,
    &button_lower_motor,
    // PAGE 2 LEFT:
    &nex_page_2, &button_slider_1_left, &button_slider_1_right, &nex_page_2, &button_slider_2_left,
    &button_slider_2_right,
    // PAGE 2 RIGHT:
    &button_reset_shorttime_counter,
    // END OF LISTEN LIST:
    NULL};

// VARIABLES TO MONITOR NEXTION DISPLAY STATES *********************************

bool nex_state_air_release;
bool nex_state_upper_motor;
bool nex_state_lower_motor;
bool nex_state_motor_brake;
bool nex_state_sledge;
bool nex_state_blade;
bool nex_state_machine_running;
bool nex_state_step_mode = true;
byte nex_prev_cycle_step;
byte nex_current_page = 0;
long nex_upper_strap_feed;
long nex_lower_strap_feed;
long nex_shorttime_counter;
long nex_longtime_counter;

// CREATE VECTOR CONTAINER FOR THE CYCLE STEPS OBJECTS ************************

int Cycle_step::object_count = 0; // enable object counting
std::vector<Cycle_step *> cycle_steps;

// NON NEXTION FUNCTIONS *******************************************************

void reset_cylinder_states() {
  cylinder_sledge.set(0);
  //motor_upper_strap.set(0);
  //motor_lower_strap.set(0);
  motor_upper_brake.set(0);
  cylinder_blade.set(0);
  cylinder_vent.set(0);
}

void stop_machine() {
  reset_cylinder_states();
  state_controller.set_step_mode();
  state_controller.set_machine_stop();
}

void reset_machine() {
  state_controller.set_machine_stop();
  reset_cylinder_states();
  clear_text_field("t4");
  hide_info_field();
  state_controller.set_current_step_to(0);
}

void motor_enable_and_brake_enable() {
  digitalWrite(UPPER_MOTOR_ENABLE_PIN, HIGH);
  digitalWrite(LOWER_MOTOR_ENABLE_PIN, HIGH);
  motor_enable_and_brake_timeout.reset_time();
}

void motor_enable_and_brake_disable() {
  digitalWrite(UPPER_MOTOR_ENABLE_PIN, LOW);
  digitalWrite(LOWER_MOTOR_ENABLE_PIN, LOW);
}

void motor_enable_and_brake_toggle() {
  if (motor_upper_brake.get_state()) {
    motor_enable_and_brake_disable();
  } else {
    motor_enable_and_brake_enable();
  }
}

void monitor_motor_enable_and_brake() {
  if (motor_enable_and_brake_timeout.has_timed_out()) {
    motor_enable_and_brake_disable();
  }
}

void start_upper_motor() {
  motor_enable_and_brake_enable();
  upper_motor.move(99999999 * micro_step_factor * stepper_direction_factor);
}

void stop_upper_motor() {

  motor_enable_and_brake_enable();
  upper_motor.move(300 * micro_step_factor * stepper_direction_factor);
}

void start_lower_motor() {
  motor_enable_and_brake_enable();
  lower_motor.move(99999999 * micro_step_factor * stepper_direction_factor);
}

void stop_lower_motor() {
  motor_enable_and_brake_enable();
  lower_motor.move(300 * micro_step_factor * stepper_direction_factor);
}

void print_cylinder_states() {
  Serial.println("ZYLINDER_STATES: " + String(cylinder_vent.get_state()) +
                 cylinder_blade.get_state() + cylinder_frontclap.get_state() +
                 cylinder_sledge.get_state() + motor_upper_brake.get_state() +
                 motor_lower_brake.get_state());
}

void manage_traffic_light() {
  // GO TO SLEEP:
  if (traffic_light.is_in_user_do_stuff_state() && motor_enable_and_brake_timeout.has_timed_out()) {
    traffic_light.set_info_sleep();
  }
  // WAKE UP:
  if (traffic_light.is_in_sleep_state() && !motor_enable_and_brake_timeout.has_timed_out()) {
    traffic_light.set_info_user_do_stuff();
  }
}

// NEXTION GENERAL DISPLAY FUNCTIONS *******************************************

void send_to_nextion() {
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

void update_display_counter() {
  long new_value = counter.get_value(longtime_counter);
  Serial2.print("t0.txt=");
  Serial2.print("\"");
  Serial2.print(new_value);
  Serial2.print("\"");
  send_to_nextion();
}

void show_info_field() {
  if (nex_current_page == 1) {
    Serial2.print("vis t4,1");
    send_to_nextion();
  }
}

void display_text_in_info_field(String text) {
  Serial2.print("t4");
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(text);
  Serial2.print("\"");
  send_to_nextion();
}

void hide_info_field() {
  if (nex_current_page == 1) {
    Serial2.print("vis t4,0");
    send_to_nextion();
  }
}

void clear_text_field(String textField) {
  Serial2.print(textField);
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(""); // erase text
  Serial2.print("\"");
  send_to_nextion();
}

void display_value_in_field(int value, String valueField) {
  Serial2.print(valueField);
  Serial2.print(".val=");
  Serial2.print(value);
  send_to_nextion();
}

void print_current_step() {
  Serial.print(state_controller.get_current_step());
  Serial.print(" ");
  // Serial.println(cycleName[state_controller.currentCycleStep()]);
}

void display_text_in_field(String text, String textField) {
  Serial2.print(textField);
  Serial2.print(".txt=");
  Serial2.print("\"");
  Serial2.print(text);
  Serial2.print("\"");
  send_to_nextion();
}

void toggle_ds_switch(String button) {
  Serial2.print("click " + button + ",1");
  send_to_nextion();
}

void set_momentary_button_high_or_low(String button, bool state) {
  Serial2.print("click " + button + "," + state);
  send_to_nextion();
}

// NEXTION TOUCH EVENT FUNCTIONS ***********************************************

// TOUCH EVENT FUNCTIONS PAGE 1 - LEFT SIDE ------------------------------------

void button_traffic_light_push(void *ptr) {

  if (traffic_light.is_in_start_state()) {
    state_controller.set_machine_running();
    nex_state_machine_running = !nex_state_machine_running;
  }
  if (traffic_light.is_in_sleep_state()) {
    motor_enable_and_brake_enable(); // wakes the tool up
  }
}

void switch_step_auto_mode_push(void *ptr) {
  // step modes is not implemented on this machine
  // make the button being toggled back:
  nex_state_step_mode = !state_controller.is_in_step_mode();
}
void button_stepback_push(void *ptr) {
  if (state_controller.get_current_step() > 0) {
    state_controller.set_current_step_to(state_controller.get_current_step() - 1);
  }
}
void button_next_step_push(void *ptr) { state_controller.switch_to_next_step(); }
void button_reset_cycle_push(void *ptr) {
  state_controller.set_reset_mode(1);
  clear_text_field("t4");
  hide_info_field();
}

// TOUCH EVENT FUNCTIONS PAGE 1 - RIGHT SIDE -----------------------------------

void switch_motor_brake_push(void *ptr) {
  motor_enable_and_brake_toggle();
  nex_state_motor_brake = !nex_state_motor_brake;
}
void button_motor_oben_push(void *ptr) { //
  start_upper_motor();
}
void button_motor_oben_pop(void *ptr) { //
  stop_upper_motor();
}
void button_motor_unten_push(void *ptr) { //
  start_lower_motor();
}
void button_motor_unten_pop(void *ptr) { //
  stop_lower_motor();
}
void switch_air_release_push(void *ptr) {
  cylinder_vent.toggle();
  nex_state_air_release = !nex_state_air_release;
}
void button_schneiden_push(void *ptr) {
  cylinder_blade.set(1);
  cylinder_frontclap.set(0);
}
void button_schneiden_pop(void *ptr) {
  cylinder_blade.set(0);
  cylinder_frontclap.set(1);
}
void button_schlitten_push(void *ptr) { //
  cylinder_sledge.set(1);
}
void button_schlitten_pop(void *ptr) { //
  cylinder_sledge.set(0);
}

// TOUCH EVENT FUNCTIONS PAGE 2 - LEFT SIDE ------------------------------------

void button_upper_slider_left_push(void *ptr) { decrease_slider_value(upper_strap_feed); }
void button_upper_slider_right_push(void *ptr) { increase_slider_value(upper_strap_feed); }
void button_lower_slider_left_push(void *ptr) { decrease_slider_value(lower_strap_feed); }
void button_lower_slider_right_push(void *ptr) { increase_slider_value(lower_strap_feed); }
void increase_slider_value(int eeprom_value_number) {
  long max_value = 200; // [mm]
  long interval = 5;
  long current_value = counter.get_value(eeprom_value_number);

  if (current_value <= (max_value - interval)) {
    counter.set_value(eeprom_value_number, (current_value + interval));
  } else {
    counter.set_value(eeprom_value_number, max_value);
  }
}
void decrease_slider_value(int eeprom_value_number) {
  long min_value = 0; // [mm]
  long interval = 5;
  long current_value = counter.get_value(eeprom_value_number);

  if (current_value >= (min_value + interval)) {
    counter.set_value(eeprom_value_number, (current_value - interval));
  } else {
    counter.set_value(eeprom_value_number, min_value);
  }
}

// TOUCH EVENT FUNCTIONS PAGE 2 - RIGHT SIDE -----------------------------------

void button_reset_shorttime_counter_push(void *ptr) {
  counter.set_value(shorttime_counter, 0);

  // ACTIVATE TIMEOUT TO RESET LONGTIME COUNTER:
  nex_reset_button_timeout.reset_time();
  nex_reset_button_timeout.set_flag_activated(1);
}
void button_reset_shorttime_counter_pop(void *ptr) {
  nex_reset_button_timeout.set_flag_activated(0);
}

// PAGE CHANGING EVENTS (TRIGGER UPDATE OF ALL DISPLAY ELEMENTS) ---------------

void page_0_push(void *ptr) { nex_current_page = 0; }
void page_1_push(void *ptr) {
  nex_current_page = 1;
  hide_info_field();

  // REFRESH BUTTON STATES:
  nex_prev_cycle_step = !state_controller.get_current_step();
  nex_state_step_mode = true;
  nex_state_air_release = 0;
  nex_state_motor_brake = 0;
  nex_state_upper_motor = 0;
  nex_state_sledge = 0;
  nex_state_blade = 0;
  nex_state_lower_motor = 0;
  nex_state_machine_running = 0;
  traffic_light.set_info_has_changed();
}
void page_2_push(void *ptr) {
  nex_current_page = 2;
  update_field_values_page_2();
}
void update_field_values_page_2() {
  nex_upper_strap_feed = counter.get_value(nex_upper_strap_feed) - 1;
  nex_lower_strap_feed = counter.get_value(nex_lower_strap_feed) - 1;
  nex_shorttime_counter = counter.get_value(nex_upper_strap_feed) - 1;
  nex_longtime_counter = counter.get_value(nex_upper_strap_feed) - 1;
}

// DECLARE DISPLAY EVENT LISTENERS *********************************************

void setup_display_event_callback_functions() {
  // PAGE 0 PUSH ONLY:
  nex_page_0.attachPush(page_0_push);
  // PAGE 1 PUSH ONLY:
  nex_page_1.attachPush(page_1_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  button_reset_cycle.attachPush(button_reset_cycle_push);
  button_previous_step.attachPush(button_stepback_push);
  button_next_step.attachPush(button_next_step_push);
  button_traffic_light.attachPush(button_traffic_light_push);
  switch_step_auto_mode.attachPush(switch_step_auto_mode_push);
  switch_motor_brake.attachPush(switch_motor_brake_push);
  switch_air_release.attachPush(switch_air_release_push);
  // PAGE 1 PUSH AND POP:
  button_upper_motor.attachPush(button_motor_oben_push);
  button_upper_motor.attachPop(button_motor_oben_pop);
  button_lower_motor.attachPush(button_motor_unten_push);
  button_lower_motor.attachPop(button_motor_unten_pop);
  button_blade.attachPush(button_schneiden_push);
  button_blade.attachPop(button_schneiden_pop);
  button_sledge.attachPush(button_schlitten_push);
  button_sledge.attachPop(button_schlitten_pop);
  // PAGE 2 PUSH ONLY:
  nex_page_2.attachPush(page_2_push);
  button_slider_1_left.attachPush(button_upper_slider_left_push);
  button_slider_1_right.attachPush(button_upper_slider_right_push);
  button_slider_2_left.attachPush(button_lower_slider_left_push);
  button_slider_2_right.attachPush(button_lower_slider_right_push);
  // PAGE 2 PUSH AND POP:
  button_reset_shorttime_counter.attachPush(button_reset_shorttime_counter_push);
  button_reset_shorttime_counter.attachPop(button_reset_shorttime_counter_pop);
}

// DISPLAY SETUP ***************************************************************

void nextion_display_setup() {

  Serial2.begin(9600);

  // RESET NEXTION DISPLAY: (refresh display after PLC restart)
  send_to_nextion(); // needed to start communication
  Serial2.print("rest"); // Reset
  send_to_nextion();

  setup_display_event_callback_functions();
  traffic_light.set_info_start();

  delay(2000);
  sendCommand("page 1"); // switch display to page x
  send_to_nextion();
}

// DISPLAY LOOPS ***************************************************************

void nextion_display_loop() {
  //****************************************************************************
  nexLoop(nex_listen_list); // check for any touch event

  if (nex_current_page == 1) {
    display_loop_page_1_left_side();
    display_loop_page_1_right_side();
  }

  if (nex_current_page == 2) {
    display_loop_page_2_left_side();
    display_loop_page_2_right_side();
  }
}

// DISPLAY LOOP PAGE 1 LEFT SIDE: -----------------------------------------------

void display_loop_page_1_left_side() {

  update_cycle_name();
  update_traffic_light_field();

  // UPDATE SWITCHSTATE "STEP"/"AUTO"-MODE:
  if (nex_state_step_mode != state_controller.is_in_step_mode()) {
    toggle_ds_switch("bt1");
    nex_state_step_mode = state_controller.is_in_step_mode();
  }
}

void update_cycle_name() {
  if (nex_prev_cycle_step != state_controller.get_current_step()) {
    String number = String(state_controller.get_current_step() + 1);
    String name = get_display_string();
    Serial.println(number + " " + name);
    display_text_in_field(number + " " + name, "t0");
    nex_prev_cycle_step = state_controller.get_current_step();
  }
}

String get_display_string() {
  int current_step = state_controller.get_current_step();
  char *display_text_cycle_name = cycle_steps[current_step]->get_display_text();
  return display_text_cycle_name;
}

void update_traffic_light_field() {
  if (traffic_light.info_has_changed()) {
    String color = traffic_light.get_info_color();
    set_traffic_light_field_color(color);
    String text = traffic_light.get_info_text();
    set_traffic_light_field_text(text);
  }
}

void set_traffic_light_field_text(String text) { display_text_in_field(text, "b8"); }

void set_traffic_light_field_color(String color) {
  Serial2.print("b8.bco=" + color);
  send_to_nextion();
}

// DISPLAY LOOP PAGE 1 RIGHT SIDE: ---------------------------------------------

void display_loop_page_1_right_side() {

  // UPDATE SWITCHES:
  if (cylinder_vent.get_state() != nex_state_air_release) {
    toggle_ds_switch("bt3");
    nex_state_air_release = !nex_state_air_release;
  }
  if (motor_upper_brake.get_state() != nex_state_motor_brake) {
    toggle_ds_switch("bt5");
    nex_state_motor_brake = !nex_state_motor_brake;
  }

  // UPDATE BUTTONS:
  if (cylinder_sledge.get_state() != nex_state_sledge) {
    bool state = cylinder_sledge.get_state();
    String button = "b6";
    set_momentary_button_high_or_low(button, state);
    nex_state_sledge = cylinder_sledge.get_state();
  }
  if (upper_motor.isRunning() != nex_state_upper_motor) {
    bool state = upper_motor.isRunning();
    String button = "b4";
    set_momentary_button_high_or_low(button, state);
    nex_state_upper_motor = upper_motor.isRunning();
  }
  if (cylinder_blade.get_state() != nex_state_blade) {
    bool state = cylinder_blade.get_state();
    String button = "b5";
    set_momentary_button_high_or_low(button, state);
    nex_state_blade = cylinder_blade.get_state();
  }
  if (lower_motor.isRunning() != nex_state_lower_motor) {
    bool state = lower_motor.isRunning();
    String button = "b3";
    set_momentary_button_high_or_low(button, state);
    nex_state_lower_motor = lower_motor.isRunning();
  }
}

// DIPLAY LOOP PAGE 2 LEFT SIDE: -----------------------------------------------

void display_loop_page_2_left_side() {
  update_upper_slider_value();
  update_lower_slider_value();
}

void update_upper_slider_value() {
  if (counter.get_value(upper_strap_feed) != nex_upper_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(upper_strap_feed, "mm"), "t4");
    nex_upper_strap_feed = counter.get_value(upper_strap_feed);
  }
}
void update_lower_slider_value() {
  if (counter.get_value(lower_strap_feed) != nex_lower_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(lower_strap_feed, "mm"), "t2");
    nex_lower_strap_feed = counter.get_value(lower_strap_feed);
  }
}
String add_suffix_to_eeprom_value(int eeprom_value_number, String suffix) {
  String value = String(counter.get_value(eeprom_value_number));
  String space = " ";
  String suffixed_string = value + space + suffix;
  return suffixed_string;
}

// DIPLAY LOOP PAGE 2 RIGHT SIDE: ----------------------------------------------

void display_loop_page_2_right_side() {
  update_upper_counter_value();
  update_lower_counter_value();
  reset_lower_counter_value();
}

void update_upper_counter_value() {
  if (nex_longtime_counter != counter.get_value(longtime_counter)) {
    display_text_in_field(String(counter.get_value(longtime_counter)), "t10");
    nex_longtime_counter = counter.get_value(longtime_counter);
  }
}
void update_lower_counter_value() {
  // UPDATE LOWER COUNTER:
  if (nex_shorttime_counter != counter.get_value(shorttime_counter)) {
    display_text_in_field(String(counter.get_value(shorttime_counter)), "t12");
    nex_shorttime_counter = counter.get_value(shorttime_counter);
  }
}
void reset_lower_counter_value() {
  if (nex_reset_button_timeout.is_marked_activated()) {
    Serial.println("HAUDI");
    if (nex_reset_button_timeout.has_timed_out()) {
      Serial.println("GAUDI");
      counter.set_value(longtime_counter, 0);
    }
  }
}

// CLASSES FOR THE MAIN CYCLE STEPS ********************************************

// TODO: WRITE CLASSES FOR MAIN CYCLE STEPS:
// crimp (tool can work)
// release air
// release brake
// move sledge back
// cut (open gate, then cut))S
// feed_upper_strap
// feed_lower_strap
//------------------------------------------------------------------------------
class Feed_upper_strap : public Cycle_step {
public:
  char *get_display_text() {
    char *display_text = (char *)malloc(20);
    strcpy(display_text, "BAND OBEN");
    return display_text;
  }
  void do_stuff() {
    start_upper_motor();
    stop_lower_motor();
    cylinder_vent.set(1);
    cylinder_blade.set(1);
    cylinder_frontclap.set(1);
    cylinder_sledge.set(1);
    //motor_upper_strap.set(1);
    //motor_lower_strap.set(1);

    if (test_switch_mega.switchedLow()) {
      traffic_light.set_info_user_do_stuff();
      std::cout << "STEP COMPLETED\n";
      set_completed();
    }
  }
};
//------------------------------------------------------------------------------
class Feed_lower_strap : public Cycle_step {
public:
  char *get_display_text() {
    char *display_text = (char *)malloc(20);
    strcpy(display_text, "BAND UNTEN");
    return display_text;
  }
  void do_stuff() {
    stop_upper_motor();
    start_lower_motor();
    if (test_switch_mega.switchedLow()) {
      std::cout << "STEP COMPLETED\n";
      traffic_light.set_info_machine_do_stuff();
      set_completed();
    }
  }
};
//------------------------------------------------------------------------------
class Brake : public Cycle_step {
public:
  char *get_display_text() {
    char *display_text = (char *)malloc(20);
    strcpy(display_text, "BREMSEN");
    return display_text;
  }
  void do_stuff() {

    cylinder_vent.set(0);
    cylinder_blade.set(0);
    cylinder_frontclap.set(0);
    cylinder_sledge.set(0);
    //motor_lower_strap.set(0);

    if (test_switch_mega.switchedLow()) {
      std::cout << "STEP COMPLETED\n";
      set_completed();
      traffic_light.set_info_machine_do_stuff();
      counter.count_one_up(longtime_counter);
      counter.count_one_up(shorttime_counter);
    }
  }
};
//------------------------------------------------------------------------------

// STEPPER MOTOR SETUP *********************************************************
void setup_stepper_motors() {

  // PINS:
  pinMode(UPPER_MOTOR_STEP_PIN, OUTPUT);
  pinMode(UPPER_MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(UPPER_MOTOR_ENABLE_PIN, OUTPUT);
  pinMode(LOWER_MOTOR_STEP_PIN, OUTPUT);
  pinMode(LOWER_MOTOR_DIRECTION_PIN, OUTPUT);
  pinMode(LOWER_MOTOR_ENABLE_PIN, OUTPUT);

  // MAX MOTOR PARAMETERS:
  long max_motor_speed = 3000 * micro_step_factor; // experimentally determined
  long max_motor_acceleration = 10000 * micro_step_factor; // experimentally determined
  upper_motor.setMaxSpeed(max_motor_speed); // [steps/s]
  upper_motor.setAcceleration(max_motor_acceleration); // [steps/s^2)
  lower_motor.setMaxSpeed(max_motor_speed); // [steps/s]
  lower_motor.setAcceleration(max_motor_acceleration); // [steps/s^2)

  // SET MAX ENABLE AND BRAKE TIME:
  motor_enable_and_brake_timeout.set_time(10000); // to prevent overheating
}
// MAIN SETUP ******************************************************************
void setup() {
  setup_stepper_motors();
  //------------------------------------------------
  // SETUP PIN MODES:
  pinMode(TEST_SWITCH_PIN, INPUT_PULLUP); // ---> DEACTIVATE FOR CONTROLLINO !!!

  //------------------------------------------------
  // PUSH THE CYCLE STEPS INTO THE VECTOR CONTAINER:
  // PUSH SEQUENCE = CYCLE SEQUENCE!
  cycle_steps.push_back(new Feed_upper_strap);
  cycle_steps.push_back(new Feed_lower_strap);
  cycle_steps.push_back(new Brake);
  //------------------------------------------------
  // CONFIGURE THE STATE CONTROLLER:
  int no_of_cycle_steps = Cycle_step::object_count;
  state_controller.set_no_of_steps(no_of_cycle_steps);
  //------------------------------------------------
  // SETUP COUNTER:
  counter.setup(0, 1023, counter_no_of_values);
  //------------------------------------------------
  Serial.begin(115200);
  state_controller.set_auto_mode(); // there is no step mode in this program
  state_controller.set_machine_stop();
  Serial.println("EXIT SETUP");
  //------------------------------------------------
  nextion_display_setup();
}

// MAIN LOOP *******************************************************************

void loop() {

  // UPDATE DISPLAY:
  nextion_display_loop();

  // MONITOR MOTOR BRAKE TO PREVENT FROM OVERHEATING
  monitor_motor_enable_and_brake();

  // RUN STEPPER MOTORS:
  upper_motor.run();
  lower_motor.run();

  // IF STEP IS COMPLETED SWITCH TO NEXT STEP:
  if (cycle_steps[state_controller.get_current_step()]->is_completed()) {
    state_controller.switch_to_next_step();
  }

  // RESET RIG IF RESET IS ACTIVATED:
  if (state_controller.reset_mode_is_active()) {
    //   reset_test_rig();
  }

  // IN STEP MODE, THE RIG STOPS AFTER EVERY COMPLETED STEP:
  if (state_controller.step_switch_has_happend()) {
    if (state_controller.is_in_step_mode()) {
      state_controller.set_machine_running(false);
    }
  }

  // IF MACHINE STATE IS "RUNNING", RUN CURRENT STEP:
  if (state_controller.machine_is_running()) {
    cycle_steps[state_controller.get_current_step()]->do_stuff();
    // std::cout << "---------------\n\n";
  }

  // MANAGE TRAFFIC LIGHT:
  manage_traffic_light();

  // DISPLAY DEBUG INFOMATION:
  if (print_interval_timeout.has_timed_out()) {
    // print_cylinder_states();
    print_interval_timeout.reset_time();
  }
}

// END OF PROGRAM **************************************************************
