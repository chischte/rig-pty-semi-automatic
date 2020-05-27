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
 * ? Implement on "on init" function in abstract class Cycle_step
 * Check if traffic light display updates only when a state change happend
 * Implement stepper motor driver library and code
 * Fix bug traffic light not updating on page change
 * eeprom counter is not a counter but a rememberer!
 * Split insomnia in two libraries (delay and timeout)
 * Install code check tools
 * ? Implement sub step possibility
 * ? Implement timeout possibility, if smart, to abstract cycle step class
 * Read:
 * https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 */

// INCLUDE HEADERS *************************************************************

#include <Controllino.h> // PIO Controllino Library, comment out for Arduino
#include <ArduinoSTL.h> //          https://github.com/mike-matera/ArduinoSTL
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
void set_play_pause_button_pause();
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

// GENERATE OBJECTS ************************************************************

EEPROM_Counter counter;
State_controller state_controller;
Traffic_light traffic_light;

// VISIER D15
// SCHLITTEN D14
// SCHLITTEN D13
// MESSER D12


Cylinder zylinder_schlitten(CONTROLLINO_D13);
Cylinder zylinder_entlueften(CONTROLLINO_D14);
Cylinder zylinder_messer(CONTROLLINO_D12);
Cylinder zylinder_visier(CONTROLLINO_D15);
Cylinder motor_band_oben(CONTROLLINO_D3);
Cylinder motor_band_unten(CONTROLLINO_D2);
Cylinder motor_bremse_oben(CONTROLLINO_D1);
Cylinder motor_bremse_unten(CONTROLLINO_D0);

const byte TEST_SWITCH_PIN = 2; // needed for the temporary pullup
Debounce test_switch_mega(TEST_SWITCH_PIN);
Debounce sensor_upper_strap(CONTROLLINO_A0);
Debounce sensor_lower_strap(CONTROLLINO_A1);

Insomnia nex_reset_button_timeout(3000); // pushtime to reset counter
Insomnia brake_timeout(5000); // to prevent overheating
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
    &button_blade, &switch_motor_brake, &switch_air_release, &button_sledge,
    &button_upper_motor, &button_lower_motor,
    // PAGE 2 LEFT:
    &nex_page_2, &button_slider_1_left, &button_slider_1_right, &nex_page_2,
    &button_slider_2_left, &button_slider_2_right,
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
  zylinder_schlitten.set(0);
  motor_band_oben.set(0);
  motor_band_unten.set(0);
  motor_bremse_oben.set(0);
  zylinder_messer.set(0);
  zylinder_entlueften.set(0);
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

void motor_brake_enable() {
  motor_bremse_oben.set(1);
  motor_bremse_unten.set(1);
  brake_timeout.reset_time();
}

void motor_brake_disable() {
  motor_bremse_oben.set(0);
  motor_bremse_oben.set(0);
}

void motor_brake_toggle() {
  if (motor_bremse_oben.get_state()) {
    motor_brake_disable();
  } else {
    motor_brake_enable();
  }
}

void monitor_motor_brake() {
  if (brake_timeout.has_timed_out()) {
    motor_brake_disable();
  }
}

void print_cylinder_states() {
  Serial.println("ZYLINDER_STATES: " + String(zylinder_entlueften.get_state()) +
                 zylinder_messer.get_state() + zylinder_visier.get_state() +
                 zylinder_schlitten.get_state() + motor_band_oben.get_state() +
                 motor_band_unten.get_state() + motor_bremse_oben.get_state() +
                 motor_bremse_unten.get_state());
}

void manage_traffic_light() {
  // GO TO SLEEP:
  if (traffic_light.is_in_user_do_stuff_state() &&
      brake_timeout.has_timed_out()) {
    Serial.println("HAUDI GANG");
    traffic_light.set_info_sleep();
  }
  // WAKE UP:
  if (traffic_light.is_in_sleep_state() && !brake_timeout.has_timed_out()) {
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
    motor_brake_enable(); // wakes the tool up
  }
}

void switch_step_auto_mode_push(void *ptr) {
  // step modes is not implemented on this machine
  // make the button being toggled back:
  nex_state_step_mode = !state_controller.is_in_step_mode();
}
void button_stepback_push(void *ptr) {
  if (state_controller.get_current_step() > 0) {
    state_controller.set_current_step_to(state_controller.get_current_step() -
                                         1);
  }
}
void button_next_step_push(void *ptr) {
  state_controller.switch_to_next_step();
}
void button_reset_cycle_push(void *ptr) {
  state_controller.set_reset_mode(1);
  clear_text_field("t4");
  hide_info_field();
}

// TOUCH EVENT FUNCTIONS PAGE 1 - RIGHT SIDE -----------------------------------

void switch_motor_brake_push(void *ptr) {
  motor_brake_toggle();
  nex_state_motor_brake = !nex_state_motor_brake;
}
void button_motor_oben_push(void *ptr) { //
  motor_band_oben.set(1);
}
void button_motor_oben_pop(void *ptr) { //
  motor_band_oben.set(0);
}
void button_motor_unten_push(void *ptr) { //
  motor_band_unten.set(1);
}
void button_motor_unten_pop(void *ptr) { //
  motor_band_unten.set(0);
}
void switch_air_release_push(void *ptr) {
  zylinder_entlueften.toggle();
  nex_state_air_release = !nex_state_air_release;
}
void button_schneiden_push(void *ptr) {
  zylinder_messer.set(1);
  zylinder_visier.set(0);
}
void button_schneiden_pop(void *ptr) {
  zylinder_messer.set(0);
  zylinder_visier.set(1);
}
void button_schlitten_push(void *ptr) { //
  zylinder_schlitten.set(1);
}
void button_schlitten_pop(void *ptr) { //
  zylinder_schlitten.set(0);
}

// TOUCH EVENT FUNCTIONS PAGE 2 - LEFT SIDE ------------------------------------

void button_upper_slider_left_push(void *ptr) {
  decrease_slider_value(upper_strap_feed);
}
void button_upper_slider_right_push(void *ptr) {
  increase_slider_value(upper_strap_feed);
}
void button_lower_slider_left_push(void *ptr) {
  decrease_slider_value(lower_strap_feed);
}
void button_lower_slider_right_push(void *ptr) {
  increase_slider_value(lower_strap_feed);
}
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
  update_traffic_light_field();

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
  button_reset_shorttime_counter.attachPush(
      button_reset_shorttime_counter_push);
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

void set_traffic_light_field_text(String text) {
  display_text_in_field(text, "b8");
}

void set_traffic_light_field_color(String color) {
  Serial2.print("b8.bco=" + color);
  send_to_nextion();
}

// DISPLAY LOOP PAGE 1 RIGHT SIDE: ---------------------------------------------

void display_loop_page_1_right_side() {

  // UPDATE SWITCHES:
  if (zylinder_entlueften.get_state() != nex_state_air_release) {
    toggle_ds_switch("bt3");
    nex_state_air_release = !nex_state_air_release;
  }
  if (motor_bremse_oben.get_state() != nex_state_motor_brake) {
    toggle_ds_switch("bt5");
    nex_state_motor_brake = !nex_state_motor_brake;
  }

  // UPDATE BUTTONS:
  if (zylinder_schlitten.get_state() != nex_state_sledge) {
    bool state = zylinder_schlitten.get_state();
    String button = "b6";
    set_momentary_button_high_or_low(button, state);
    nex_state_sledge = zylinder_schlitten.get_state();
  }
  if (motor_band_oben.get_state() != nex_state_upper_motor) {
    bool state = motor_band_oben.get_state();
    String button = "b4";
    set_momentary_button_high_or_low(button, state);
    nex_state_upper_motor = motor_band_oben.get_state();
  }
  if (zylinder_messer.get_state() != nex_state_blade) {
    bool state = zylinder_messer.get_state();
    String button = "b5";
    set_momentary_button_high_or_low(button, state);
    nex_state_blade = zylinder_messer.get_state();
  }
  if (motor_band_unten.get_state() != nex_state_lower_motor) {
    bool state = motor_band_unten.get_state();
    String button = "b3";
    set_momentary_button_high_or_low(button, state);
    nex_state_lower_motor = motor_band_unten.get_state();
  }
}

// DIPLAY LOOP PAGE 2 LEFT SIDE: -----------------------------------------------

void display_loop_page_2_left_side() {
  update_upper_slider_value();
  update_lower_slider_value();
}

void update_upper_slider_value() {
  if (counter.get_value(upper_strap_feed) != nex_upper_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(upper_strap_feed, "mm"),
                          "t4");
    nex_upper_strap_feed = counter.get_value(upper_strap_feed);
  }
}
void update_lower_slider_value() {
  if (counter.get_value(lower_strap_feed) != nex_lower_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(lower_strap_feed, "mm"),
                          "t2");
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
    zylinder_entlueften.set(1);
    zylinder_messer.set(1);
    zylinder_visier.set(1);
    zylinder_schlitten.set(1);
    motor_band_oben.set(1);
    motor_band_unten.set(1);
    motor_brake_enable();

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
    zylinder_entlueften.set(0);
    zylinder_messer.set(0);
    zylinder_visier.set(0);
    zylinder_schlitten.set(0);
    motor_band_oben.set(0);
    motor_band_unten.set(0);
    motor_brake_disable();
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

// MAIN SETUP ******************************************************************

void setup() {
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
  state_controller.set_auto_mode();
  state_controller.set_machine_stop();
  //pinMode(TEST_SWITCH_PIN, INPUT_PULLUP); // ---> DEACTIVATE FOR CONTROLLINO !!!
  Serial.println("EXIT SETUP");
  //------------------------------------------------
  nextion_display_setup();
}

// MAIN LOOP *******************************************************************

void loop() {

  // UPDATE DISPLAY:
  nextion_display_loop();

  // MONITOR MOTOR BRAKE TO PREVENT FROM OVERHEATING
  monitor_motor_brake();

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
