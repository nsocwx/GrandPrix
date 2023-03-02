// nsocwx 2022
// @nsocwx
// Digital pressure and RPM counter
// Pressure transducer output conncets to A0
// RPM signal (falling) connects to digital 2
/*****************************/
// includes
#include <SPI.h>
#include <util/atomic.h>
/*****************************/
// definitions
#define pressurePin A3
#define rpmPin 2
/*****************************/
// variables
unsigned int rpm,RPM;//varaible to store rpm
unsigned int rpmImage;//variable to store which rpm image to display
volatile unsigned int PULSE;//counter for pulses 
unsigned int delta; //holds the delta time
unsigned long timeNew; //variable to store current check time
unsigned long timeOld;  //variable to store previous check time
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


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);           //  setup serial
  //vss
  attachInterrupt(digitalPinToInterrupt(rpmPin),rpmCount,FALLING);//create interrupt on rpmPin to run rpmCount() on each pulse
  timeOld = millis();  
  delay(2000);
}

void loop() {
  delay(20);
  // put your main code here, to run repeatedly:
  //rpm read
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){  
    rpm = readRpm();
  }
  rpm = constrain(rpm, 0, 8000);  // Constrain the value so it doesn't go below or above the limits
  
  // Smoothing RPM:
  // subtract the last reading:
  total = total - readings[readIndex];
  // read speed:
  readings[readIndex] = rpm;  // takes the value we are going to smooth
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  
  // calculate the average:
  average = total / numReadings;  // the average value it's the smoothed result

  //rpmImage = map(rpm, 0, 8000, 0, 208);//maps rpm to the correct image to display
  rpmImage = map(average, 0, 8000, 0, 208);//maps smoothed rpm to the correct image to display
  rpmImage = constrain(rpmImage, 0, 208);//ensure image is 0 to 208 only 

  

  //oil pressure read
  analogVolts = analogRead(pressurePin); //read in the analog voltage
  if(analogVolts < 102){
    oilPressure = 0;
    //Serial.println("Oil pressure low input");
  }
  else if(analogVolts > 922){
    oilPressure = 0;
    //Serial.println("Oil pressure high input");
  }
  else{
    oilPressure = map(analogVolts, 102, 922, 0, 100); //map .5v to 0 PSI/% and 4.5v to 100 PSI/%
  }
  oilPressure = constrain(oilPressure, 0, 100);
  
  //water temp read
  tempRaw = analogread(0);
  waterTemp = Thermister(tempRaw);
  waterTempBar = waterTemp;
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
void rpmCount() //runs when rpmPin sees a falling signal
{
  PULSE++;//increment pulse counter by 1;
}
/*****************************/
unsigned int readRpm()//runs when called
{
  unsigned int result;
  timeNew = millis();
  delta = (timeNew-timeOld);
  result = (PULSE*60*1000)/delta/cylinders;
  timeOld = timeNew;
  PULSE = 0;
  return result;
}
/*****************************/
void nextionWrite(const char* item, int val)
{
  Serial.print(item); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
  Serial.print("=");
  Serial.print(val);
  Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);      
}

/*****************************/ 
//https://forum.arduino.cc/t/gm-coolant-sensor-12146312-interface/410131
double Thermister(int RawADC) {  //Function to perform the fancy math of the Steinhart-Hart equation
 double Temp;
 Temp = log(((10240000/RawADC) - 10000));
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;              // Convert Kelvin to Celsius
 Temp = (Temp * 9.0)/ 5.0 + 32.0; // Celsius to Fahrenheit - comment out this line if you need Celsius
 return Temp;
}
