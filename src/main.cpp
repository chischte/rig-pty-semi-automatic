/*
 * ****************************************************************************
 * PTY SEMI-AUTOMATIC-RIG
 * ****************************************************************************
 * Program for a semi-automatic endurance test rig for a mechanical tool
 * ****************************************************************************
 * Michael Wettstein
 * May 2020, Zürich
 * ****************************************************************************
 * TODO:
 * 
 * UPDATE TRAFFIC LIGHT ONLY WHEN STATE CHANGED
 * Implement user info "WARTEN" in red and "CRIMPEN" in green
 * Refactor star- an dashlines, update code guidelines
  * eeprom counter is not a counter but a rememberer!
 * Find unused variables ...how?
 * Clean up code
 * Split insomnia in two libraries (delay and timeout)
 * Implement stepper motor driver library and code
 * Install code check tools
 * 
 * For all submodule libraries:
 *    Make a "legacy branch" at the state of the current master branch
 *    Merge the current develop branch to master  
 * 
 * Read:
 * https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 * Implement Nextion, make button state monitoring more elegant
 * Implement sub step possibility
 * Implement timeout possibility, if smart, to abstract cycle step class
 * Implement Stepper motors! ...and possibility to change [mm]
 */

//#include <Controllino.h> // PIO Controllino Library, comment out when using an Arduino
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
#include <traffic_light.h> //       keeps track of colored display infos

// DECLARE FUNCTIONS FOR THE COMPILER:
//*****************************************************************************
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

// DEFINE NAMES CYCLE COUNTER: *************************************************

enum counter {
  longtime_counter, //
  shorttime_counter, //
  upper_strap_feed, // [mm]
  lower_strap_feed, // [mm]
  end_of_counter_enum // keep this entry
};
int counter_no_of_values = end_of_counter_enum;

// GENERATE INSTANCES OF CLASSES:
//*****************************************************************************
EEPROM_Counter eeprom_counter;
State_controller state_controller;

Cylinder zylinder_schlitten(CONTROLLINO_D3);
Cylinder zylinder_entlueften(CONTROLLINO_D4);
Cylinder zylinder_messer(CONTROLLINO_D8);
Cylinder zylinder_visier(CONTROLLINO_D10);
Cylinder motor_band_oben(CONTROLLINO_D5);
Cylinder motor_band_unten(CONTROLLINO_D6);
Cylinder motor_bremse_oben(CONTROLLINO_D7);
Cylinder motor_bremse_unten(CONTROLLINO_D9);

const byte TEST_SWITCH_PIN = 2; // needed for the temporary pullup
Debounce test_switch(TEST_SWITCH_PIN);
Debounce sensor_upper_strap(CONTROLLINO_A0);
Debounce sensor_lower_strap(CONTROLLINO_A1);

Insomnia nex_reset_button_timeout(3000); // pushtime to reset counter
Insomnia brake_timeout(5000); // to prevent overheating
Insomnia print_interval_timeout(500);

// NEXTION DISPLAY - OBJECTS ***************************************************

// PAGE 0:
NexPage nex_page_0 = NexPage(0, 0, "page0");
// PAGE 1 - LEFT SIDE:
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

// NEXTION DISPLAY - TOUCH EVENT LIST ******************************************

NexTouch *nex_listen_list[] = { //
    // PAGE 0:
    &nex_page_0,
    // PAGE 1 LEFT:
    &nex_page_1, &button_previous_step, &button_next_step, &button_reset_cycle,
    &button_play_pause_ds, &button_modeswitch_ds,
    // PAGE 1 RIGHT:
    &button_schneiden, &button_klemmen_ds, &button_entlueften_ds, &button_schlitten,
    &button_motor_oben, &button_motor_unten,
    // PAGE 2 LEFT:
    &nex_page_2, &button_slider_1_left, &button_slider_1_right, &nex_page_2, &button_slider_2_left,
    &button_slider_2_right,
    // PAGE 2 RIGHT:
    &button_reset_shorttime_counter,
    // END OF LISTEN LIST:
    NULL};

// VARIABLES TO MONITOR NEXTION DISPLAY STATES *********************************

bool nex_state_play_pause_button;
bool nex_state_entlueftung;
bool nex_state_motorbremse;
bool nex_state_motor_oben;
bool nex_state_schlitten;
bool nex_state_messer;
bool nex_state_machine_running;
bool nex_state_motor_unten;
bool nex_state_step_mode = true;
byte nex_prev_cycle_step;
byte nex_current_page = 0;
long nex_upper_strap_feed;
long nex_lower_strap_feed;
long nex_prev_shorttime_counter;
long nex_prev_longtime_counter;

// CREATE VECTOR CONTAINER FOR THE CYCLE STEPS OBJECTS
//*****************************************************************************
int Cycle_step::object_count = 0; // enable object counting
std::vector<Cycle_step *> cycle_steps;

// NON NEXTION FUNCTIONS: ******************************************************

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
  //state_controller.set_machine_awake();
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

void monitor_motor_brake_and_set_sleep() {
  if (brake_timeout.has_timed_out()) {
    motor_brake_disable();
    //state_controller.set_machine_asleep();
  }
}

// NEXTION GENERAL DISPLAY FUNCTIONS: ******************************************

void send_to_nextion() {
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

void update_display_counter() {
  long new_value = eeprom_counter.get_value(longtime_counter);
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

void print_cylinder_states() {
  Serial.println("ZYLINDER_STATES: " + String(zylinder_entlueften.get_state()) +
                 zylinder_messer.get_state() + zylinder_visier.get_state() +
                 zylinder_schlitten.get_state() + motor_band_oben.get_state() +
                 motor_band_unten.get_state() + motor_bremse_oben.get_state() +
                 motor_bremse_unten.get_state());
}

// NEXTION TOUCH EVENT FUNCTIONS *******************************************

// TOUCH EVENT FUNCTIONS PAGE 1 - LEFT SIDE:
void button_play_pause_ds_push(void *ptr) {
  state_controller.toggle_machine_running_state();
  nex_state_machine_running = !nex_state_machine_running;
}

void button_modeswitch_ds_push(void *ptr) {
  if (state_controller.is_in_auto_mode()) {
    state_controller.set_step_mode();
  } else {
    state_controller.set_auto_mode();
  }
  nex_state_step_mode = state_controller.is_in_step_mode();
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

// TOUCH EVENT FUNCTIONS PAGE 1 - RIGHT SIDE:
void button_klemmen_ds_pop(void *ptr) {
  motor_brake_toggle();
  nex_state_motorbremse = !nex_state_motorbremse;
}
void button_motor_oben_push(void *ptr) { motor_band_oben.set(1); }
void button_motor_oben_pop(void *ptr) { motor_band_oben.set(0); }
void button_motor_unten_push(void *ptr) { motor_band_unten.set(1); }
void button_motor_unten_pop(void *ptr) { motor_band_unten.set(0); }
void button_entlueften_ds_push(void *ptr) {
  zylinder_entlueften.toggle();
  nex_state_entlueftung = !nex_state_entlueftung;
}
void button_schneiden_push(void *ptr) {
  zylinder_messer.set(1);
  zylinder_visier.set(0);
}
void button_schneiden_pop(void *ptr) {
  zylinder_messer.set(0);
  zylinder_visier.set(1);
}
void button_schlitten_push(void *ptr) { zylinder_schlitten.set(1); }
void button_schlitten_pop(void *ptr) { zylinder_schlitten.set(0); }

// TOUCH EVENT FUNCTIONS PAGE 2 - LEFT SIDE:
void button_upper_slider_left_push(void *ptr) { decrease_slider_value(upper_strap_feed); }
void button_upper_slider_right_push(void *ptr) { increase_slider_value(upper_strap_feed); }
void button_lower_slider_left_push(void *ptr) { decrease_slider_value(lower_strap_feed); }
void button_lower_slider_right_push(void *ptr) { increase_slider_value(lower_strap_feed); }
void increase_slider_value(int eeprom_value_number) {
  long max_value = 200; // [mm]
  long interval = 5;
  long current_value = eeprom_counter.get_value(eeprom_value_number);

  if (current_value <= (max_value - interval)) {
    eeprom_counter.set_value(eeprom_value_number, (current_value + interval));
  } else {
    eeprom_counter.set_value(eeprom_value_number, max_value);
  }
}
void decrease_slider_value(int eeprom_value_number) {
  long min_value = 0; // [mm]
  long interval = 5;
  long current_value = eeprom_counter.get_value(eeprom_value_number);

  if (current_value >= (min_value + interval)) {
    eeprom_counter.set_value(eeprom_value_number, (current_value - interval));
  } else {
    eeprom_counter.set_value(eeprom_value_number, min_value);
  }
}

// TOUCH EVENT FUNCTIONS PAGE 2 - RIGHT SIDE:
void button_reset_shorttime_counter_push(void *ptr) {
  eeprom_counter.set_value(shorttime_counter, 0);

  // ACTIVATE TIMEOUT TO RESET LONGTIME COUNTER:
  nex_reset_button_timeout.reset_time();
  nex_reset_button_timeout.set_flag_activated(1);
}
void button_reset_shorttime_counter_pop(void *ptr) {
  nex_reset_button_timeout.set_flag_activated(0);
}

// PAGE CHANGING EVENTS (TRIGGER UPDATE OF ALL DISPLAY ELEMENTS):
void page_0_push(void *ptr) { nex_current_page = 0; }
void page_1_push(void *ptr) {
  nex_current_page = 1;
  hide_info_field();

  // REFRESH BUTTON STATES:
  nex_prev_cycle_step = !state_controller.get_current_step();
  nex_state_step_mode = true;
  nex_state_entlueftung = 0;
  nex_state_motorbremse = 0;
  nex_state_motor_oben = 0;
  nex_state_schlitten = 0;
  nex_state_messer = 0;
  nex_state_motor_unten = 0;
  nex_state_machine_running = 0;
}
void page_2_push(void *ptr) {
  nex_current_page = 2;
  update_field_values_page_2();
}
void update_field_values_page_2() {
  nex_upper_strap_feed = eeprom_counter.get_value(nex_upper_strap_feed) - 1;
  nex_lower_strap_feed = eeprom_counter.get_value(nex_lower_strap_feed) - 1;
  nex_prev_shorttime_counter = eeprom_counter.get_value(nex_upper_strap_feed) - 1;
  nex_prev_longtime_counter = eeprom_counter.get_value(nex_upper_strap_feed) - 1;
}

//******************************************************************************
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
  nex_page_2.attachPush(page_2_push);
  button_slider_1_left.attachPush(button_upper_slider_left_push);
  button_slider_1_right.attachPush(button_upper_slider_right_push);
  button_slider_2_left.attachPush(button_lower_slider_left_push);
  button_slider_2_right.attachPush(button_lower_slider_right_push);
  // PAGE 2 PUSH AND POP:
  button_reset_shorttime_counter.attachPush(button_reset_shorttime_counter_push);
  button_reset_shorttime_counter.attachPop(button_reset_shorttime_counter_pop);
}
//******************************************************************************
void nextion_display_setup() {
  //*****************************************************************************
  Serial2.begin(9600);

  // RESET NEXTION DISPLAY: (refresh display after PLC restart)
  send_to_nextion(); // needed to start communication
  Serial2.print("rest"); // Reset
  send_to_nextion();

  setup_display_event_callback_functions();

  delay(2000);
  sendCommand("page 1"); // switch display to page x
  send_to_nextion();
}
//******************************************************************************
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
//------------------------------------------------------------------------------

// DIPLAY LOOP FUNCTIONS PAGE 1: -----------------------------------------------
void display_loop_page_1_left_side() {

  update_cycle_name();
  //update_traffic_light_field();

  // UPDATE SWITCHSTATE "STEP"/"AUTO"-MODE:
  if (nex_state_step_mode != state_controller.is_in_step_mode()) {
    Serial2.print("click bt1,1");
    send_to_nextion();
    nex_state_step_mode = state_controller.is_in_step_mode();
  }
}

void update_cycle_name() {
  if (nex_prev_cycle_step != state_controller.get_current_step()) {
    String number = String(state_controller.get_current_step() + 1);
    String name = get_display_string();
    Serial.println(number + " " + name);
    display_text_in_field(number + " " + name, "t0");
    //(state_controller.currentCycleStep() + 1) + (" " +
    // cycleName[state_controller.currentCycleStep()]), "t0");
    nex_prev_cycle_step = state_controller.get_current_step();
  }
}

void update_traffic_light_field() {
  String green = "2016";
  String blue = "500";
  String red = "63488";

  if (1) {
    set_traffic_light_field_text("SLEEP");
    set_traffic_light_field_color(blue);
  } else {
    set_traffic_light_field_text("CRIMP");
    set_traffic_light_field_color(green);
  }
}

void set_traffic_light_field_text(String text) { display_text_in_field(text, "bt0"); }

void set_traffic_light_field_color(String color) {
  Serial2.print("bt0.bco2=" + color);
  send_to_nextion();
  Serial2.print("bt0.bco=" + color);
  send_to_nextion();
}

String get_display_string() {
  int current_step = state_controller.get_current_step();
  char *display_text_cycle_name = cycle_steps[current_step]->get_display_text();
  return display_text_cycle_name;
}
//------------------------------------------------------------------------------
void display_loop_page_1_right_side() {
  // UPDATE SWITCHBUTTON (dual state):
  if (zylinder_entlueften.get_state() != nex_state_entlueftung) {
    Serial2.print("click bt3,1");
    send_to_nextion();
    nex_state_entlueftung = !nex_state_entlueftung;
  }

  // UPDATE SWITCHBUTTON (dual state):
  if (motor_bremse_oben.get_state() != nex_state_motorbremse) {
    Serial2.print("click bt5,1");
    send_to_nextion();
    nex_state_motorbremse = !nex_state_motorbremse;
  }

  // UPDATE BUTTON (momentary):
  if (zylinder_schlitten.get_state() != nex_state_schlitten) {
    if (zylinder_schlitten.get_state()) {
      Serial2.print("click b6,1");
    } else {
      Serial2.print("click b6,0");
    }
    send_to_nextion();
    nex_state_schlitten = zylinder_schlitten.get_state();
  }

  // UPDATE BUTTON (momentary):
  if (motor_band_oben.get_state() != nex_state_motor_oben) {
    if (motor_band_oben.get_state()) {
      Serial2.print("click b4,1");
    } else {
      Serial2.print("click b4,0");
    }
    send_to_nextion();
    nex_state_motor_oben = motor_band_oben.get_state();
  }

  // UPDATE BUTTON (momentary):
  if (zylinder_messer.get_state() != nex_state_messer) {
    if (zylinder_messer.get_state()) {
      Serial2.print("click b5,1");
    } else {
      Serial2.print("click b5,0");
    }
    send_to_nextion();
    nex_state_messer = zylinder_messer.get_state();
  }

  // UPDATE BUTTON (momentary):
  if (motor_band_unten.get_state() != nex_state_motor_unten) {
    if (motor_band_unten.get_state()) {
      Serial2.print("click b3,1");
    } else {
      Serial2.print("click b3,0");
    }
    send_to_nextion();
    nex_state_motor_unten = motor_band_unten.get_state();
  }
}
// DIPLAY LOOP FUNCTIONS PAGE 2: -----------------------------------------------
void display_loop_page_2_left_side() {
  update_upper_slider_value();
  update_lower_slider_value();
}
void update_upper_slider_value() {
  if (eeprom_counter.get_value(upper_strap_feed) != nex_upper_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(upper_strap_feed, "mm"), "t4");
    nex_upper_strap_feed = eeprom_counter.get_value(upper_strap_feed);
  }
}
void update_lower_slider_value() {
  if (eeprom_counter.get_value(lower_strap_feed) != nex_lower_strap_feed) {
    display_text_in_field(add_suffix_to_eeprom_value(lower_strap_feed, "mm"), "t2");
    nex_lower_strap_feed = eeprom_counter.get_value(lower_strap_feed);
  }
}
String add_suffix_to_eeprom_value(int eeprom_value_number, String suffix) {
  String value = String(eeprom_counter.get_value(eeprom_value_number));
  String space = " ";
  String suffixed_string = value + space + suffix;
  return suffixed_string;
}

void display_loop_page_2_right_side() {
  update_upper_counter_value();
  update_lower_counter_value();
  reset_lower_counter_value();
}
void update_upper_counter_value() {
  if (nex_prev_longtime_counter != eeprom_counter.get_value(longtime_counter)) {
    display_text_in_field(String(eeprom_counter.get_value(longtime_counter)), "t10");
    nex_prev_longtime_counter = eeprom_counter.get_value(longtime_counter);
  }
}
void update_lower_counter_value() {
  // UPDATE LOWER COUNTER:
  if (nex_prev_shorttime_counter != eeprom_counter.get_value(shorttime_counter)) {
    display_text_in_field(String(eeprom_counter.get_value(shorttime_counter)), "t12");
    nex_prev_shorttime_counter = eeprom_counter.get_value(shorttime_counter);
  }
}
void reset_lower_counter_value() {
  if (nex_reset_button_timeout.is_marked_activated()) {
    Serial.println("HAUDI");
    if (nex_reset_button_timeout.has_timed_out()) {
      Serial.println("GAUDI");
      eeprom_counter.set_value(longtime_counter, 0);
    }
  }
}
//******************************************************************************
// CLASSES FOR THE MAIN CYCLE STEPS ********************************************
//******************************************************************************
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
    if (test_switch.switchedLow()) {
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
    if (test_switch.switchedLow()) {
      std::cout << "STEP COMPLETED\n";
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
    if (test_switch.switchedLow()) {
      std::cout << "STEP COMPLETED\n";
      set_completed();
      eeprom_counter.count_one_up(longtime_counter);
      eeprom_counter.count_one_up(shorttime_counter);
    }
  }
};
//------------------------------------------------------------------------------
/* class Step_wippenhebel : public Cycle_step
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
    //motor_band_oben.stroke(1500, 1000);
    //if (motor_band_oben.stroke_completed())
    if(test_switch.switchedLow())
    {
      std::cout << "Class I bytes ya tooth\n";
      set_completed();
    }
  }

private:
}; */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//******************************************************************************
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
  eeprom_counter.setup(0, 1023, counter_no_of_values);
  //------------------------------------------------
  Serial.begin(115200);
  state_controller.set_auto_mode();
  state_controller.set_machine_running();
  pinMode(TEST_SWITCH_PIN, INPUT_PULLUP); // !!! DEACTIVATE FOR CONTROLLINO !!!
  Serial.println("EXIT SETUP");
  //------------------------------------------------
  nextion_display_setup();
}
//******************************************************************************
void loop() {

  // UPDDATE DISPLAY:
  nextion_display_loop();

  // MONITOR MOTOR BRAKE TO PREVENT FROM OVERHEATING
  monitor_motor_brake_and_set_sleep();

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
    // std::cout << "THIS IS WHAT IT DOES: ";
    cycle_steps[state_controller.get_current_step()]->do_stuff();
    // std::cout << "---------------\n\n";
  }
  // int current_step_no=state_controller.get_current_step();
  //...WESHALB GEHT DAS NICHT?: Cycle_step
  // current_cycle_step=cycle_steps[current_step_no];

  // DISPLAY DEBUG INFOMATION:
  if (print_interval_timeout.has_timed_out()) {
    // print_cylinder_states();
    print_interval_timeout.reset_time();
  }
}
//******************************************************************************
