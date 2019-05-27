#include <AccelStepper.h>
//#include <MultiStepper.h>


#define ARM_S_PIN 3
#define ARM_D_PIN 4
#define TURNTABLE_S_PIN 5
#define TURNTABLE_D_PIN 6
#define SHUTTER_PIN 7
#define LOWER_LIMIT_PIN -1
#define UPPER_LIMIT_PIN -1

AccelStepper arm(1, ARM_S_PIN, ARM_D_PIN);

void setup() {
  
  pinMode(LOWER_LIMIT_PIN, INPUT);
  pinMode(UPPER_LIMIT_PIN, INPUT);

  pinMode(ARM_S_PIN, OUTPUT);
  pinMode(ARM_D_PIN, OUTPUT);
  pinMode(TURNTABLE_S_PIN, OUTPUT);
  pinMode(TURNTABLE_D_PIN, OUTPUT);
  pinMode(SHUTTER_PIN, OUTPUT);

  const int ARM_SPEED = 4000;
  arm.setMaxSpeed(ARM_SPEED);
  arm.setSpeed(ARM_SPEED / 2);
  arm.setAcceleration(ARM_SPEED / 2);

  Serial.begin(9600);
}

void motorStepRoutine(int directionPin, int stepsPin, int waitMS, bool invert = false) {
  digitalWrite(directionPin, invert ? LOW : HIGH);//direction of rotation of turn table set LOW to reverse direction)
  digitalWrite(stepsPin, HIGH);// pulses for turn table  
  delayMicroseconds(waitMS);// pulses for turn table                        
  digitalWrite(stepsPin, LOW);// pulses for turn table     
  delayMicroseconds(waitMS);// pulses for turn table         
}

bool limitActive(int pin){
  int state = digitalRead(pin);
  return state == HIGH;
}

void turnArmAccel(int steps, int delayMS = 0, int endDelay = 0){
  arm.move(steps);
  arm.runToPosition();
  if(endDelay > 0) delay(endDelay);
}

void turnArm(int steps, int delayMS = 0, int endDelay = 0){
  bool invert = false;
  if(steps < 0){
    steps = -steps;
    invert = true;
  }

  for (int t = 0; t < steps; t++){
    motorStepRoutine(ARM_D_PIN, ARM_S_PIN, 500, invert);
    if(delayMS > 0) delay(delayMS);
  }

  if(endDelay > 0) delay(endDelay);
}

void capture(int preCaptureDelay, int captureDelay, int delayMS = 0) {
  if(preCaptureDelay > 0) delay (preCaptureDelay);
  digitalWrite(SHUTTER_PIN, HIGH);//chutter pin
  if(captureDelay > 0) delay (captureDelay);//wait time while shooting shooting milliseconds
  digitalWrite(SHUTTER_PIN, LOW);//chutter pin
  if(delayMS > 0) delay((int)(delayMS / 2.0));//wait time after shooting shooting milliseconds
}

void rotateAndCapture(int captures, int degrees = 12, int delayMS = 0) {

  // turn table rotates 12 degrees each time
  // 6400*12(you can change 12 to any degree you want ) / 360 = 213
  int steps = (int)(6400.0 * degrees / 360);
  int stepsToCapture = (int)((double)steps / captures);
  for (int t = 0; t < steps; t++){
    motorStepRoutine(TURNTABLE_D_PIN, TURNTABLE_S_PIN, 500);
    if(t % stepsToCapture == 0) capture(0, 10, 10);
    if(delayMS > 0) delay(delayMS);
  }
}

const int TURNTABLE_ROTATION_COUNT = 1;
const int TURNTABLE_ROTATION_DEGREES = 360;
const int TURNTABLE_ROTATION_DELAY = 500;

const int CAPTURES_PER_ROTATION = 30;

#define ARM_SHOULD_ROTATE true
const int ARM_ROTATION_POS_DELAY = 2000;
const int ARM_ROTATION_RANGE = 18000;
const int ARM_ROTATION_START_OFFSET = 4000;
const int ARM_ROTATION_POSITIONS = 3;

void capturePosition(){
  for (int f = 0; f < TURNTABLE_ROTATION_COUNT; f++){
    rotateAndCapture(CAPTURES_PER_ROTATION, TURNTABLE_ROTATION_DEGREES, 4); 
    delay(TURNTABLE_ROTATION_DELAY);//wait time in before shooting milliseconds
  }
}

void print(String value){
  Serial.println(value);
}

char readSerialChar(){
  return Serial.available() > 0 ? Serial.read() : '\0';
}

int reachLimit(int limitPin, int stepsPerTry){
  const int MAX_STEPS = 22000;
  int maxTrials = MAX_STEPS / stepsPerTry;
  int trials = 0;
  while(trials++ < maxTrials && !limitActive(limitPin)) turnArm(stepsPerTry);
  return trials * stepsPerTry;
}

void captureCycle() {

  const int STEPS_PER_TRY = 50;

  // Travel until top is reached
  if(ARM_SHOULD_ROTATE)
    turnArmAccel(ARM_ROTATION_RANGE, 0, ARM_ROTATION_POS_DELAY);

  print("Capture cycle started");

  // Capture and lower arm
  int stepsPerPosition = (ARM_ROTATION_RANGE - ARM_ROTATION_START_OFFSET) / (ARM_ROTATION_POSITIONS - 1);
  for(int i = 0; i < ARM_ROTATION_POSITIONS; i++){
    capturePosition();
    print("Position complete");

    if(ARM_SHOULD_ROTATE && i < (ARM_ROTATION_POSITIONS - 1))
      turnArmAccel(-stepsPerPosition, 0, ARM_ROTATION_POS_DELAY);
  }
  
  print("Capture cycle complete");
  
  // Travel until lower limit reached
  if(ARM_SHOULD_ROTATE)
    turnArmAccel(-ARM_ROTATION_START_OFFSET, 0, 0);
}

void loop() {
  //while(readSerialChar() != '\n');
  delay(500);
  captureCycle();
  while(true) delay(1000);
}
