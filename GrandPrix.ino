// nsocwx 2022
// @nsocwx
// Digital pressure and RPM counter
// Pressure transducer output conncets to A0
// RPM signal (falling) connects to digital 2
/*****************************/
// includes
#include <SPI.h>
#include <util/atomic.h>
#include <RunningMedian.h>

RunningMedian samples = RunningMedian(40);
/*****************************/
// definitions
#define pressurePin 3//analog
#define rpmPin 2
#define tempPin 0//analog
/*****************************/
// variables
volatile unsigned int rpm,RPM;//varaible to store rpm
unsigned int rpmImage;//variable to store which rpm image to display
volatile unsigned long delta= 15000000; //holds the delta time
volatile unsigned long timeNew; //variable to store current check time
volatile unsigned long timeOld;  //variable to store previous check time
unsigned int analogVolts; //variable to store the voltage reading from pressurePin
unsigned int oilPressure; //varible to store readable oil pressure
unsigned int tempRaw; //variable to store the raw temp reading
unsigned int waterTemp; //variable to store readable water temp
unsigned int waterTempBar; //variable to store the water temp bar graph %
unsigned int cylinders = 8; //stores number of cylinders
// Calibration for smoothing RPM:
const int numReadings = 20;     // number of samples for smoothing. The higher, the more smoothing, but slower to react. Default: 20
// Variables for smoothing tachometer:
int readings[numReadings];  // the input
int readIndex = 0;  // the index of the current reading
long total = 0;  // the running total
int average = 0;  // the average
// steinhart variables
// https://forum.arduino.cc/t/automotive-temperature-sensors-interface/1050838
/*****************************/
// Sensor parameters. Sensors are resistive, ground connected, and pulled
// up to (in this case) 5V with 470 ohms.
//
const float ADCFULLSCALE =          1024;     // analogRead() returns 0..1023
const float SENSORPUVOLTS =            5.0;   // volts
const float SENSORPUOHMS =           550;     // pullup resistor

// These are the Steinhart-Hart coefficients for the two sensor types.
const double GMCoeffA = 0.001454437010;
const double GMCoeffB = 0.0002335550484;
const double GMCoeffC = 0.00000008717999782;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);           //  setup serial
  //vss
  attachInterrupt(digitalPinToInterrupt(rpmPin),rpmCount,FALLING);//create interrupt on rpmPin to run rpmCount() on each pulse
  timeOld = micros();  
  delay(2000);
}

void loop() {
  delay(10);
  // put your main code here, to run repeatedly:
  // rpm read
  // calculate rpm, copies delta variable using atomic block

  //https://imgur.com/gallery/zIYda
  rpm = samples.getAverage(5);
  rpm = constrain(rpm, 0, 8000);  // Constrain the value so it doesn't go below or above the limits  
  rpmImage = map(rpm, 0, 8000, 0, 208);//maps smoothed rpm to the correct image to display
  rpmImage = constrain(rpmImage, 0, 208);//ensure image is 0 to 208 only   
  rpm = rpm - 5;// tick down rpm to handle no input

  //oil pressure read
  oilPressure = readPSISensor(pressurePin);
  oilPressure = constrain(oilPressure, 0, 100);
  
  //water temp read
  waterTemp = readNTCSensor (tempPin, "GM coolant temp", GMCoeffA, GMCoeffB, GMCoeffC);
  waterTempBar = readNTCSensor (tempPin, "GM coolant temp", GMCoeffA, GMCoeffB, GMCoeffC);
  map(waterTempBar, 100, 250, 0, 100); //map 100 deg to 0% and 250 deg to 100%
  waterTempBar = constrain(waterTempBar, 0, 100);
  
  //output
  // Send tachometer value:
  nextionWrite("tach.pic",rpmImage);
  //Send oil press value:
  nextionWrite("oil.val",oilPressure);
  nextionWrite("oiln.val",oilPressure);
  //Send water temp value:
  nextionWrite("temp.val",waterTempBar);
  nextionWrite("tempn.val",waterTemp);
   
}
/*****************************/
void rpmCount() //runs when rpmPin sees an interrupt signal, measures delta time between sparks
{
  timeNew = micros();
  if((timeNew-timeOld) > 2500){
    delta = timeNew-timeOld;
    rpm = 15.00 / ((float)delta / 1000000.00);
    samples.add(rpm);
  }  
  timeOld = timeNew;
}


int readPSISensor (int pin) //runs when called to read the 5v analog sensor
{
    // MEASUREMENT
    // make x readings and average them (reduces some noise) you can tune the number 16 of course
    int count = 16;
    int raw = 0;
    int psi = 0;
    for (int i=0; i< count; i++) raw += analogRead(pin);  // return 0..1023 representing 0..5V
    raw = raw / count;

    // INTERPRET READINGS
    psi = map(raw, 100, 920, 0, 110); // between 0.5 and 4.5v now...
    psi = constrain(psi, 0, 100);

    return psi;
}

//see https://forum.arduino.cc/t/automotive-temperature-sensors-interface/1050838
int readNTCSensor (int pin, const char *text, double kA, double kB, double kC) {
double Rs;
double logR2, T;

  Rs= readSensor (pin);

  // Steinhart-Hart NTC thermistor resistance to degrees F.
  // Steinhart - Hart Equation = 1/T = A+B(LnR)+C(LnR)3
  //
  logR2= log (Rs);
  T= 1.0 / (kA + (kB * logR2) + (kC * logR2 * logR2 * logR2));
  T= T - 273.15;                   // K to C
  T= (T * 9.0) / 5.0 + 32.0;       // C to F

  return T + 0.5;
}


/* read a resistive sensor pulled up with a resistor, return it's resistance, in ohms. */

int readSensor (int pin) {
int a;
float adcV, I, Rs;

  // voltage at the resistor divider. if the sensor is open
  // circuit, this should be the same as the reference
  // voltage.
  //  
  a= analogRead (pin);
  adcV= (float)a * (SENSORPUVOLTS / ADCFULLSCALE);

  // current flowing through the sensor and the pullup.
  // the battery voltage is in millivolts. this won't work out
  // well if the sensor is infinite ohms, hence the max (i, .0001)
  //
  I= fmax ((SENSORPUVOLTS - adcV) / SENSORPUOHMS, 0.0001);

  // sensor R now derives from I.
  //
  Rs= adcV / I;

  return Rs + 0.5;
}


void nextionWrite(const char* item, int val)
{
  Serial.print(item); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
  Serial.print("=");
  Serial.print(val);
  Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);      
}
