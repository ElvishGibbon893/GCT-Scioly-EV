#include <Arduino.h>

const int motorLeftPWM   = 25;
const int motorLeftDir   = 26;
const int motorRightPWM  = 27;
const int motorRightDir  = 14;

const int encoderLeftA   = 32;
const int encoderLeftB   = 33;
const int encoderRightA  = 34;
const int encoderRightB  = 35;

float targetDistance     = 200.0;
unsigned long runTime    = 10000;
float curveWidth         = 30.0;

volatile long leftTicks  = 0;
volatile long rightTicks = 0;

float ticksPerRev        = 600.0;
float wheelCircumference = 20.0;

float baseSpeed          = 180;
unsigned long startTime;

void IRAM_ATTR leftEncoderISR() {
  if (digitalRead(encoderLeftA) == digitalRead(encoderLeftB)) {
    leftTicks++;
  } else {
    leftTicks--;
  }
}

void IRAM_ATTR rightEncoderISR() {
  if (digitalRead(encoderRightA) == digitalRead(encoderRightB)) {
    rightTicks++;
  } else {
    rightTicks--;
  }
}

void setMotor(int pwmChannel, int dirPin, float speed) {
  if (speed >= 0) {
    digitalWrite(dirPin, HIGH);
    ledcWrite(pwmChannel, (int)fabs(speed));
  } else {
    digitalWrite(dirPin, LOW);
    ledcWrite(pwmChannel, (int)fabs(speed));
  }
}

float distanceTraveled() {
  float leftDist  = (leftTicks  / ticksPerRev) * wheelCircumference;
  float rightDist = (rightTicks / ticksPerRev) * wheelCircumference;
  return (leftDist + rightDist) / 2.0;
}

void setup() {
  Serial.begin(115200);

  pinMode(motorLeftDir, OUTPUT);
  pinMode(motorRightDir, OUTPUT);

  ledcSetup(0, 5000, 8);
  ledcAttachPin(motorLeftPWM, 0);

  ledcSetup(1, 5000, 8);
  ledcAttachPin(motorRightPWM, 1);

  pinMode(encoderLeftA, INPUT_PULLUP);
  pinMode(encoderLeftB, INPUT_PULLUP);
  pinMode(encoderRightA, INPUT_PULLUP);
  pinMode(encoderRightB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encoderLeftA),  leftEncoderISR,  CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderRightA), rightEncoderISR, CHANGE);

  Serial.println("Enter A<distance cm>, B<runtime ms>, C<curve cm>");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("A")) {
      targetDistance = input.substring(1).toFloat();
    } else if (input.startsWith("B")) {
      runTime = input.substring(1).toInt();
    } else if (input.startsWith("C")) {
      curveWidth = input.substring(1).toFloat();
    }

    startTime = millis();
    leftTicks = 0;
    rightTicks = 0;
  }

  unsigned long elapsed = millis() - startTime;
  float dist = distanceTraveled();

  if (elapsed >= runTime || dist >= targetDistance) {
    setMotor(0, motorLeftDir, 0);
    setMotor(1, motorRightDir, 0);
    return;
  }

  float halfDist = targetDistance / 2.0;
  float offset   = 0;

  if (dist < halfDist) {
    offset = (curveWidth / halfDist) * dist;
  } else {
    offset = curveWidth - (curveWidth / halfDist) * (dist - halfDist);
  }

  float leftSpeed  = baseSpeed + offset;
  float rightSpeed = baseSpeed - offset;

  setMotor(0, motorLeftDir, leftSpeed);
  setMotor(1, motorRightDir, rightSpeed);
}
