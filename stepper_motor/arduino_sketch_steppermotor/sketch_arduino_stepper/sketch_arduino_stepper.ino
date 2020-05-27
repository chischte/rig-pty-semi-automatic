#include <Controllino.h>
#include <AccelStepper.h> // https://www.airspayce.com/mikem/arduino/AccelStepper
#include <Debounce.h>        // https://github.com/chischte/debounce-library

// PINS:
const byte STEP_PIN = CONTROLLINO_D0;
const byte DIRECTION_PIN = CONTROLLINO_D1;
const byte ENABLE_PIN = CONTROLLINO_D2;
//const byte GND1 = 8;
//const byte GND2 = 9;
//const byte GND3 = 10;
//const byte GND4 = 7;
const byte SWITCH_PIN = CONTROLLINO_A0;
const byte CHANGE_DIRECTION_PIN = 2;



// MAKE INSTANCE OF STEPPER
AccelStepper StepperOben(1, STEP_PIN, DIRECTION_PIN ); //1=DRIVER MODE
// CONFIGURE MICRO STEPPING:
// ********
long microStepFactor = 2; // adjust to selected microstep mode
// ********
Debounce ChangeDirectionSwitch(CHANGE_DIRECTION_PIN);
Debounce FeedPin(SWITCH_PIN);


void setup() {
  Serial.begin(115200);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  //pinMode(GND1, OUTPUT);
  //pinMode(GND2, OUTPUT);
  //pinMode(GND3, OUTPUT);
  //pinMode(GND4, OUTPUT);
  //pinMode(SWITCH_PIN, INPUT_PULLUP);
  //pinMode(CHANGE_DIRECTION_PIN, INPUT_PULLUP);

  //digitalWrite(GND1, LOW);
  //digitalWrite(GND2, LOW);
  //digitalWrite(GND3, LOW);
  //digitalWrite(GND4, LOW);

  // ENABLE MOTOR:
  digitalWrite(ENABLE_PIN, HIGH);
  Serial.println("EXIT SETUP");
  //SWITCH DIRECTION
  //digitalWrite(DIRECTION_PIN, HIGH);

  // SETUP STEPPER

  StepperOben.setMaxSpeed((3000 * microStepFactor)); // [steps/s]
  // 1/2 step  StepperOben.setAcceleration((100000 * microStepFactor)); // [steps/s^2)
  StepperOben.setAcceleration((10000 * microStepFactor)); // [steps/s^2)
}
//*****************************************************************************
//********************#*********#####***#####***######*************************
//********************#********#*****#*#*****#**#*****#************************
//********************#********#*****#*#*****#**######*************************
//********************#********#*****#*#*****#**#******************************
//********************#######***#####***#####***#******************************
//*****************************************************************************
void loop() {

  // FLIP DIRECTION
  static int8_t directionFactor = 1;
  //if (ChangeDirectionSwitch.switchedLow()) {
  //  directionFactor *= -1;
  //  Serial.println(directionFactor);
  // }

  // WORKING:
  ///*
  static bool motorRunning = false;
  // PRESS FEED BUTTON:
  if (FeedPin.switchedHigh()) {
    StepperOben.move(99999999 * microStepFactor * directionFactor);
    motorRunning = true;
  }
  // RELEASE AGAIN:
  if (motorRunning == true && FeedPin.switchedLow()) {
    StepperOben.move(300 * microStepFactor * directionFactor);
  }
  // */


  /*
    if (!digitalRead(SWITCH_PIN)) {
          StepperOben.move(4000 * directionFactor);
        }
  */
  StepperOben.run();
}
