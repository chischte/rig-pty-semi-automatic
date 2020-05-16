/*
 * StateController.h
 *
 *  Created on: Oct 20, 2019
 *      Author: realslimshady
 */

#ifndef StateController_H_
#define StateController_H_

class State_controller {

public:
  // FUNCTIONS:
  State_controller(int number_of_steps);
  State_controller();

  void set_no_of_steps(int number_of_steps);

  void set_step_mode();
  bool is_in_step_mode();

  void set_auto_mode();
  bool is_in_auto_mode();

  void set_machine_running(bool machine_state);
  void set_machine_running();
  void set_machine_stop();
  void toggle_machine_running_state();
  bool machine_is_running();

  void set_machine_asleep();
  void set_machine_awake();
  bool is_asleep();

  void switch_to_next_step();
  void set_current_step_to(int cycle_step);
  int get_current_step();
  bool step_switch_has_happend();

  void set_reset_mode(bool reset_state);
  bool reset_mode_is_active();

  void set_run_after_reset(bool run_after_reset);
  bool run_after_reset_is_active();

  // VARIABLES:
  // n.a.

private:
  // FUNCTIONS:
  // n.a.

  // VARIABLES:
  int _number_of_steps;
  int _current_cycle_step;
  int _previous_cycle_step;
  bool _machine_running;
  bool _machine_asleep;
  bool _step_mode;
  bool _auto_mode;
  bool _reset_mode;
  bool _run_after_reset;
};
#endif /* StateController_H_ */
