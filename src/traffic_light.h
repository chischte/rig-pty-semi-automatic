#ifndef TRAFFIC_LIGHT_H_
#define TRAFFIC_LIGHT_H_

#include "Arduino.h"

class Traffic_light {

public:
  void set_info_start();
  void set_info_user_do_stuff();
  void set_info_user_wait();
  void set_info_sleep();

  bool info_has_changed();
  bool is_in_sleep_state();

  String get_info_color();
  String get_info_text();

private:
  bool _info_has_changed;
  bool _sleep_state_active;
  String _info_color;
  String _info_text;
  String _green = "2016";
  String _blue = "500";
  String _red = "63488";
};
#endif