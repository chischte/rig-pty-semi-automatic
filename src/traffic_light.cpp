#include "traffic_light.h"
#include "Arduino.h"

void Traffic_light::set_info_start() {
  _info_start_is_active = true;
  _info_user_do_stuff_is_active = false;
  _info_user_wait_is_active = false;
  _info_sleep_is_active = false;

  _info_color = _blue;
  _info_text = "START";
}
void Traffic_light::set_info_user_do_stuff() {}
void Traffic_light::set_info_user_wait() {}
void Traffic_light::set_info_sleep() {}

String Traffic_light::get_info_color() { return _info_color; }
String Traffic_light::get_info_text() { return _info_text; }

bool Traffic_light::info_has_changed() { return _info_has_changed; }