#include "traffic_light.h"
#include "Arduino.h"

void Traffic_light::set_info_start() {
  _info_has_changed = true;
  _info_color = _blue;
  _info_text = "START";
}

void Traffic_light::set_info_user_do_stuff() {
  _info_has_changed = true;
  _info_color = _green;
  _info_text = "CRIMPEN";
}

void Traffic_light::set_info_user_wait() {
  _info_has_changed = true;
  _info_color = _blue;
  _info_text = "WARTEN";
}

void Traffic_light::set_info_sleep() {
  _info_has_changed = true;
  _info_color = _red;
  _info_text = "SLEEP";
}

String Traffic_light::get_info_color() { return _info_color; }
String Traffic_light::get_info_text() { return _info_text; }

bool Traffic_light::info_has_changed() {
  if (_info_has_changed) {
    _info_has_changed = false;
    return true;
  }
  return false;
}