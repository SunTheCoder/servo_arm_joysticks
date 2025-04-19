#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === OLED Setup ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === Servo Setup ===
Servo base;
Servo gripper;
Servo elbow;
Servo shoulder;

// === Pins ===
const int trigPin = 8;
const int echoPin = 7;
const int joy1X = A0;  // Joystick 1: Base
const int joy1Y = A1;  // Joystick 1: Elbow
const int joy1Btn = 2; // Joystick 1 button
const int joy2Y = A2;  // Joystick 2: Shoulder

// === State Tracking ===
bool gripperClosed = false;
bool lastButtonState = HIGH;

// === Angle Tracking ===
int baseAngle = 90;
static int elbowAngle = 120;
static int targetElbow = 120;

static int shoulderAngle = 0;
static int targetShoulder = 0;

const int threshold = 30;     // joystick dead zone
const int stepSize = 5;       // speed of movement (lower = slower)

void setup() {
  Serial.begin(9600);

  // Attach servos
  base.attach(5);
  gripper.attach(9);
  elbow.attach(6);
  shoulder.attach(3);

  // === Safe Origin Positions ===
  base.write(baseAngle);
  elbow.write(elbowAngle);
  shoulder.write(shoulderAngle);
  gripper.write(160);  // open

  // Sensor setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Joystick button setup
  pinMode(joy1Btn, INPUT_PULLUP);

  // OLED setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Dual Joystick Arm");
  display.display();
  delay(1000);
}

void loop() {
  // === Sensor Read (for display only) ===
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  // === Joystick Reads ===
  int x1 = analogRead(joy1X);
  int y1 = analogRead(joy1Y);
  int y2 = analogRead(joy2Y);
  bool buttonPressed = digitalRead(joy1Btn) == LOW;

  // === Base Control (direct write) ===
  baseAngle = map(x1, 0, 1023, 120, 60);
  base.write(baseAngle);

  // === Elbow Stepper Logic ===
  if (abs(y1 - 512) > threshold) {
    targetElbow = map(y1, 0, 1023, 180, 90);
  }
  if (elbowAngle < targetElbow) {
    elbowAngle += stepSize;
    if (elbowAngle > targetElbow) elbowAngle = targetElbow;
  } else if (elbowAngle > targetElbow) {
    elbowAngle -= stepSize;
    if (elbowAngle < targetElbow) elbowAngle = targetElbow;
  }
  elbow.write(elbowAngle);

  // === Shoulder Stepper Logic ===
  if (abs(y2 - 512) > threshold) {
    targetShoulder = map(y2, 0, 1023, 80, 0);
  }
  if (shoulderAngle < targetShoulder) {
    shoulderAngle += stepSize;
    if (shoulderAngle > targetShoulder) shoulderAngle = targetShoulder;
  } else if (shoulderAngle > targetShoulder) {
    shoulderAngle -= stepSize;
    if (shoulderAngle < targetShoulder) shoulderAngle = targetShoulder;
  }
  shoulder.write(shoulderAngle);

  // === Gripper Toggle ===
  if (buttonPressed && !lastButtonState) {
    gripperClosed = !gripperClosed;
    gripper.write(gripperClosed ? 90 : 160);
    delay(200); // debounce
  }
  lastButtonState = buttonPressed;

  // === OLED Display ===
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Distance: ");
  display.print(distance);
  display.println(" cm");

  display.print("Base: "); display.println(baseAngle);
  display.print("Elbow: "); display.println(elbowAngle);
  display.print("Shoulder: "); display.println(shoulderAngle);
  display.print("Gripper: "); display.println(gripperClosed ? "CLOSED" : "OPEN");

  display.display();
  delay(100);
}

