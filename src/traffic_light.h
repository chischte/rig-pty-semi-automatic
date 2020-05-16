#ifndef TRAFFIC_LIGHT_H_
#define TRAFFIC_LIGHT_H_

#include "Arduino.h"

class Traffic_light {

public:
  void set_info_start();
  void set_info_user_do_stuff();
  void set_info_user_wait();
  void set_info_sleep();

  String get_info_color();
  String get_info_text();

  bool info_has_changed();

private:
  bool _info_start_is_active;
  bool _info_user_do_stuff_is_active;
  bool _info_user_wait_is_active;
  bool _info_sleep_is_active;
  String _info_color;
  String _info_text;
};
#endif