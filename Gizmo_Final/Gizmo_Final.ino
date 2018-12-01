/*
  Gizmo Curio: Shylamp
  Lea Marolt Sonnenschein

  Shylamp is a lamp that is shy. When you leave it alone it's animated, moves about and stays lit up (like a proper lamp),
  but when you approach it, it gets shy, stopes moving, and slowly turns off.

  This code controls a 5-level rotating tower/lamp.
  // Levels 1 and 5 rotate the fastest @ 6 rotations/ 15 motor rotations
  // Levels 2 and 4 rotate at 1/3 of the speed of levels 1 & 5, @ 3 rotations/ 15 motor rotations
  // Level 3 is the slowest and rotates at 1/2 of the speed of levels 1 & 5 @ 2 rotations/ 15 motor rotations

  When a person is more than 90cm away, Shylamp will happily do its thing.
  But when you cross the threshold, it starts to hide. 
  First, at < 90cm, Shylamp will rotate all its towers into alignment, to hide the fact that it's "alive",
  Then every 15cm closer, Shylamp will turn off the lights on each of the level from bottom to top.
  The opposite also holds true.
*/

#include <Adafruit_NeoPixel.h>
#include <Stepper.h>

/* 
  NEOPIXELS: 
  Component: Soldered NeoPixel LED strip, 4 LEDs per tower level, 20 LEDs in total
  They're individually addressable based on RGB values

  NeoPixel strip has 3 wires; ground, voltage and data. Data has to be connected to a
  microcontroller pin through a resistor (300-500 Ohm)

  Tutorial: https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices
*/

const int NUM_PIXELS = 20;
const int PIXELS_PER_LEVEL = 4;
const int PIXEL_PIN = 8;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

/* 
  DISTANCE SENSOR: 
  Component: Ultrasonic Sensor HC-SR04
  
  Tutorial: https://howtomechatronics.com/tutorials/arduino/ultrasonic-sensor-hc-sr04/
  
  How it works: Emits an ultrasound at 40 000 Hz that travels through the air and bounces back to the module
  if it detects an obstacle.
*/

// Constants
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

// Variables
long duration; // How long it takes for the sound to travel back
int distance; // How far, in cm, the object the component sensed is

/* Since the proximity sensor is a little temperamental every so often, and produces values that 
 are outrageously different from the ones read before and after, this variable keeps track of the last
 9 readings to check for outliers and ignore them to insure a smooth transition between turning the
 tower levels on and off. */
 
int lastNineReadings [9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 }; 
int proximityReadingsCount = 0;

// The max distance at which the tower is both lit up and moving, 90 cm, 
//once the threshold is closed, the tower will slowly turn off
const int maxProximityDistance = 90; 
const int proximityStep = 15; // Turn off each level in 15cm incrememnts

/* 
 *  STEPPER:
 *  
 *  Component: Kysan 1124090 Nema 17 Stepper Motor + TB6612 1.2 A Driver
 *  
 *  Tutorial: https://learn.adafruit.com/adafruit-tb6612-h-bridge-dc-stepper-motor-driver-breakout/overview
 *  
 */

const int STEPS_PER_REVOLUTION = 200;  

// How many motor rotations it takes to align the tower into its original position?

// Based on the fact that the fastest tower level rotates at 6/15 motor rotations
// The medium tower level at 1/3 of the speed
// The bottom tower level at 1/2 of the speed

// Since each level is a square, that means that it each has 4 times the opportunity for perfect alignment, due to its 4 sides

// This means that for every 1.5 rotations of the fastest level, 
// the middle one will do 1 rotation,
// and the slowest 0.5

// So, they will all align at 1.5 rotations of the fastest level.
// Since the fastest level is operating at a speed of 6/15 motor rotations, that means that X is our alignment_revolutions number 

// 6/1.5 = 15/x
// x = 1.5*15/6
// x = 3.75

const float ALIGNMENT_REVOLUTIONS = 3.75; 

// initialize the stepper library on pins 5 through 7:
Stepper myStepper(STEPS_PER_REVOLUTION, 4, 5, 6, 7);

// number of steps the motor has taken, important to know to determine when a full revolution happens, 
//because stepper motors don't have a sense of positioning
int stepCount = 0;

/* 
  TOWER/LAMP
  
  Variables for controlling the tower structure. 
*/

// Variables
bool shouldAlign = false; // Should the tower align itself or keep moving in continuous rotationm
bool levelsOff [5] = { 0, 0, 0, 0, 0 }; // Keeps track of whether a tower level is off or on

void setup() {

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.begin(9600);
  pixels.begin(); // This initializes the NeoPixel library.
}

void loop() {
  
  handleMotorUpdate();
    
  // Clears the TRIG_PIN
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  // Sets the TRIG_PIN on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);
  // Calculating the distance
  distance= duration*0.034/2;

  storeDistanceReading(distance);
  
  int medDistance = medianDistance();
  updateTower(medDistance);
  
  // Increase proximity readings count by 1
  proximityReadingsCount++;
}

// Sorts the array of the last 9 proximity sensor readings, and returns the median value, to avoid outlier spikes
int medianDistance() {
  int lastNineReadings_length = 9;
  // qsort - last parameter is a function pointer to the sort function
  qsort(lastNineReadings, lastNineReadings_length, sizeof(lastNineReadings[0]), sort_desc);

  return lastNineReadings[5];
}

// Returns the number of revolutions the stepper motor has taken so far
float currentRevolutions(int steps) {
  return (float)steps / (float)STEPS_PER_REVOLUTION;
}

// Stores the distance readings in the correct position of the lastNineReadings array
void storeDistanceReading(int distance) {
  
  // Replace reading position with current distance reading
  int mod = proximityReadingsCount%10;  
  lastNineReadings[mod] = distance;
}

// Updates the alignment and light levels of the tower
void updateTower(int distance) {
  updateAlignment(distance);
  updateLampLevels(distance);
}

// Sets shouldAlign to true if the tower should try to align itself based on the distance being less than the maxProximityDistance
void updateAlignment(int distance) {
  shouldAlign = (distance < maxProximityDistance);
}

/* Handles updating the motor speed through the different stages of the tower's lifecycle
 *  
 1. When object is > maxProximityDistance away, the stepper continuously revolves at 60 RPM
 2. When object is <= maxProximityDistance && > (maxProximityDistance - proximityStep), 
 it speeds up to 90 RPM and calculates how long to go until it's aligned
 3. When tower is aligned && object is <= maxProximityDistance, the motor stops
 
*/
void handleMotorUpdate() {
  
  if (shouldAlign) {
    myStepper.setSpeed(90); // Speed up the motor, to align the tower quicker

    float revolutions = currentRevolutions(stepCount);
    Serial.println(revolutions);

    // If the motor has done 3.75 revolutions, the tower ~should~ be aligned
    // This will only work if the stepper steps have actually physically happened
    // Since there's no way of actually telling that the stepper has physically moved, this is theoretical
    if (fmod(revolutions, float(ALIGNMENT_REVOLUTIONS)) != 0) {
      myStepper.step(1);
      stepCount++;
    } 
  } else {
    myStepper.setSpeed(60);
    myStepper.step(1);
    stepCount++;
  }
}

// Updates which tower levels should be turned on/off in levelsOff array, and then calls updateLevel to apply the changes to the NeoPixels
void updateLampLevels(int distance) {
  for (int i = 0; i < 5; i++) {
    
    levelsOff[i] = (distance <= maxProximityDistance - ((i + 1) * proximityStep));
    updateLevel(levelsOff[i], i);
  }
}

// Updates the NeoPixels for each tower level; on/off based on object distance
void updateLevel(bool off, int levelPosition) {
  int startingPixelNum = levelPosition * PIXELS_PER_LEVEL;
  
  for (int i = startingPixelNum; i < startingPixelNum + PIXELS_PER_LEVEL; i++) {
    pixels.setPixelColor(i, off ? pixels.Color(0, 0, 0) : pixels.Color(184, 233, 100)); // Moderately bright green color.
    pixels.show();
  }
}

/* Quicksort
/ Code adapted from:  https://arduino.stackexchange.com/questions/38177/how-to-sort-elements-of-array-in-arduino-code
/ qsort requires you to create a sort function
*/

int sort_desc(const void *cmp1, const void *cmp2)
{
  // Need to cast the void * to int *
  int a = *((int *)cmp1);
  int b = *((int *)cmp2);
  // The comparison
  return a < b ? -1 : (a > b ? 1 : 0);
}
