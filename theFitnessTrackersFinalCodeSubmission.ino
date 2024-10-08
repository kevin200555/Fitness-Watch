#define BLYNK_TEMPLATE_ID "TMPL2nxHx7mLN"
#define BLYNK_TEMPLATE_NAME "The Fitness Trackers WiFi Test 2"
#define BLYNK_AUTH_TOKEN "anmxRKtcI7vIECPoEsVmn0Gv4Ypto9-S"

//these don't change (I think)
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
char auth[] = BLYNK_AUTH_TOKEN;
// Your WiFi credentials.
// Set password to "" for open networks.
//char ssid[] = "Arduino"; //WIFI NAME
//char pass[] = "fitnesstracker"; //WIFI PWD
//End of Blynk App initiaization
char ssid[] = "iPhone 13"; //WIFI NAME
char pass[] = "Houston123"; //WIFI PWD

#include <DFRobot_LIS2DH12.h>
#include <DFRobot_BMP3XX.h>

/*!
 * @brief Constructor 
 * @param pWire I2c controller
 * @param addr  I2C address(0x18/0x19)
 */
DFRobot_BMP388_I2C sensor(&Wire, sensor.eSDOVDD);
DFRobot_LIS2DH12 acce(&Wire,0x18);

//booleans
bool isOn = false;
bool flag = 0;
bool stepGoalMet = false;
bool startup = true;
//steps
int stepCount = 0;
int stepGoal = 20;
//electronics
int button = D6;
int buzzer = D7;
//accelerometer
float accelValue1 = 0;
float accelValue2 = 0;
//noise zones
float minZone = 0;
float maxZone = 0;
//barometer
float floorValue1 = 0;
float floorValue2 = 0;
float flights = 0;
void setup() {
  // put your setup code here, to run once:
  
 // pinMode(LS,INPUT);
  Serial.begin(115200);
  //Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  //Chip initialization
  while(!acce.begin()){
     Serial.println("Initialization failed, please check the connection and I2C address settings");
     delay(1000);
  }
  //Get chip id
  Serial.print("chip id : ");
  Serial.println(acce.getID(),HEX);

  /**
    set range:Range(g)
              eLIS2DH12_2g,/< ±2g>/
              eLIS2DH12_4g,/< ±4g>/
              eLIS2DH12_8g,/< ±8g>/
              eLIS2DH12_16g,/< ±16g>/
  */
  acce.setRange(/*Range = */DFRobot_LIS2DH12::eLIS2DH12_16g);

  /**
    Set data measurement rate：
      ePowerDown_0Hz 
      eLowPower_1Hz 
      eLowPower_10Hz 
      eLowPower_25Hz 
      eLowPower_50Hz 
      eLowPower_100Hz
      eLowPower_200Hz
      eLowPower_400Hz
  */
  acce.setAcquireRate(/*Rate = */DFRobot_LIS2DH12::eLowPower_10Hz);
  Serial.print("Acceleration:\n");
  delay(1000);

  int rslt;
  while( ERR_OK != (rslt = sensor.begin()) ){
    if(ERR_DATA_BUS == rslt){
      Serial.println("Data bus error!!!");
    }else if(ERR_IC_VERSION == rslt){
      Serial.println("Chip versions do not match!!!");
    }
    delay(3000);
  }
  Serial.println("Begin ok!");

  /**
   * 6 commonly used sampling modes that allows users to configure easily, mode:
   *      eUltraLowPrecision, Ultra-low precision, suitable for monitoring weather (lowest power consumption), the power is mandatory mode.
   *      eLowPrecision, Low precision, suitable for random detection, power is normal mode
   *      eNormalPrecision1, Normal precision 1, suitable for dynamic detection on handheld devices (e.g on mobile phones), power is normal mode.
   *      eNormalPrecision2, Normal precision 2, suitable for drones, power is normal mode.
   *      eHighPrecision, High precision, suitable for low-power handled devices (e.g mobile phones), power is in normal mode.
   *      eUltraPrecision, Ultra-high precision, suitable for indoor navigation, its acquisition rate will be extremely low, and the acquisition cycle is 1000 ms.
   */
  while( !sensor.setSamplingMode(sensor.eUltraPrecision) ){
    Serial.println("Set samping mode fail, retrying....");
    delay(3000);
  }

  delay(100);
  #ifdef CALIBRATE_ABSOLUTE_DIFFERENCE
  /**
   * Calibrate the sensor according to the current altitude
   * In this example, we use an altitude of 540 meters in Wenjiang District of Chengdu (China). 
   * Please change to the local altitude when using it.
   * If this interface is not called, the measurement data will not eliminate the absolute difference.
   * Notice: This interface is only valid for the first call.
   */
  if( sensor.calibratedAbsoluteDifference(540.0) ){
    Serial.println("Absolute difference base value set successfully!");
  }
  #endif

  float sampingPeriodus = sensor.getSamplingPeriodUS();
  Serial.print("samping period : ");
  Serial.print(sampingPeriodus);
  Serial.println(" us");

  float sampingFrequencyHz = 1000000 / sampingPeriodus;
  Serial.print("samping frequency : ");
  Serial.print(sampingFrequencyHz);
  Serial.println(" Hz");

  Serial.println();
  delay(1000);
  pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  
}
//counts steps
void stepCounter() {
  accelValue1 = accelValue2;
  //Reads the X value from the Accelerometer (can change this later if needed)
  accelValue2 = acce.readAccX();
  //if statements that check to see if the first value is less than the max noise zone and the second value is greater than the maxnoise zone
  //if it is, thne it counts as a step
  //I made a chart that explains this better
  if (accelValue1 < maxZone+300 && accelValue2 >= maxZone+300) {
    stepCount++;
  //same thing but if one value is less than the minimum noise zone and one value is greater than the min noise zone 
  } else if (accelValue1 > minZone-300 && accelValue2 <= minZone-300) {
    stepCount++;
  }
}
//counts flights of stairs 
void flightCounter()
{
  //reads in altitdue value from the barometer
	floorValue1 =  sensor.readAltitudeM();
  //checks if the read altitude value is 2 meters greater than the previous stored value
	if (floorValue1 >= floorValue2 + 3)
  {
		floorValue2 = floorValue1;
    //counts flights of steps
    flights++;
  }
  //checks if the read altitude value is 2 meters smaller than the previous stored value
	else if (floorValue1 <= floorValue2 - 3)
  {
    //doesn't count flights of steps
		floorValue2 = floorValue1;
  }
}
void getSteadyState() 
{
  int x;
  delay(100);
  //sets the x axis values from the accelerometer to min and max
  minZone = acce.readAccX();
  maxZone = acce.readAccX();
  //reads in 10000 values from the accelerometer and updates the lowest and highest values after each iteration
  
  for (int i = 0; i < 100; i++) 
  {
    //reads in values from the accelerometer's x axis
    x = acce.readAccX();
    // if the x value is less than the current min value, make the min value x
    if (x < minZone) 
    {
      minZone = x;
    }
    // if the x value is greater than the current max value, make the max value x
    if (x > maxZone) 
    {
      maxZone = x;
    }
  }
  //print the min and max values
  Serial.print("Min = ");
  Serial.println(minZone);
  Serial.print("Max = ");
  Serial.println(maxZone);
}
//buzzer function for when the step goal is met
//when testing this function only played two beeps
void goalMetSound(int bpm)
{
  int x = (1000*60)/bpm;
  // C C C F A (rest) C C C F A (rest) 
  for(int i = 0; i < 2; i++)
  {
    tone(buzzer,523,x/8);
    tone(buzzer,523,x/8);
    tone(buzzer,523,x/8);
    tone(buzzer,698,x/8);
    tone(buzzer,880,x/8);
    delay(x/8);
  } 
  delay((3*x)/8);
  //F F E E D D C
  tone(buzzer,698,x/8);
  tone(buzzer,698,x/8);
  tone(buzzer,659,x/8);
  tone(buzzer,659,x/8);
  tone(buzzer,587,x/8);
  tone(buzzer,587,x/8);
  tone(buzzer,523,x/2);
}
//takes in value from V1(the slider) and changes step goal
BLYNK_WRITE(V1) 
{
  //reads in value
  int pinValue = param.asInt();
  //sets value to stepGoal
  stepGoal = pinValue;
  stepGoalMet = false;
}

//takes in value from V2(the reset button) and resets the number of steps
BLYNK_WRITE(V2)
{
  int pinValue = param.asInt();
  if (pinValue == 1){
    stepCount = 0;
    flights = 0;
  }
}
BLYNK_WRITE(V3)
{
  int pinValue = param.asInt();
  if (pinValue == 1){
    isOn = true;
    flag = 1;
  }
  if (pinValue == 0){
    isOn = false;
    flag = 0;
  }
}
void loop() {
  //starts the Blynk app
  Blynk.run();
  //gets steadystate
  if(startup == true){
    getSteadyState();
    startup = false;
  }
  int buttonVal = digitalRead(button);
  if (buttonVal == LOW && flag == 0)
  {
    //read in value from V1(the slider) and set to stepGoal (may need to add this to if(isOn == false))
    //BLYNK_WRITE(V1);
    //onSound(100);
    flag = 1;
    isOn = true;
    //reset stepgoal status
    stepGoalMet = false;
    Serial.println("hi");
  }
  //if button pressed again, plays offSound, program stops
  else if(buttonVal == LOW && flag == 1)
  {
   // offSound(100);
    flag = 0;
    isOn = false;
  }
  //works every .5 seconds
  if (isOn == true){
    if((millis() % 500) == 0){
    //calls step and flight counter
    stepCounter();
    flightCounter();
    //writes to blynk
    Blynk.virtualWrite(V0, stepCount);
    Blynk.virtualWrite(V4, flights);
    //checks if step goal met
    if((stepCount >= stepGoal)&&(stepGoalMet == false)){
      stepGoalMet = true;
      //plays sound if met
      goalMetSound(100);
    }
  }
  }
  if(isOn == true){
    if((millis() % 1000) == 0){
    //changes step goal from Blynk app slider
    BLYNK_WRITE(V1); 
    }
  }

  //functions for blynk app buttons
  BLYNK_WRITE(V2);
  BLYNK_WRITE(V3);
  

}
