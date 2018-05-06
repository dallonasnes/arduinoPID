//Dallon Asnes, da002014@mymail.pomona.edu, Apr 15 2018
//PID library used as described in documentation by Brett Beauregard (http://brettbeauregard.com/)
//assisted by Prof. Scott Medling and Tony Grigsby

//Include necessary libraries
#include <PID_v1.h>
#include <Adafruit_MAX31856.h>
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

// These #defines make it easy to set the backlight color of LCD shield corresponding to action
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7
#define ROOMTEMP 25
#define RelayPin 2

//initialize variables to be used internally
double Setpoint, Input, Output;
double idealOpTemp1, idealOpTemp2, minutes1, minutes2;
unsigned long startMillis, intMillis, endMillis, rateA, rateB;
int fraction;
int hasItRunA, hasItRunB;

//////////////////////////////////////////
//MUST SELECT WHICH HEATING ROUTINE TO RUN
//////////////////////////////////////////
int selectRoutine = 1; //set =1 to correspond to Routine 1; set =2 to correspond to Routine 2, etc.

//Specify initial tuning parameters linking to PID library
PID myPID(&Input, &Output, &Setpoint, 3, 2, 1, DIRECT);

//initialize peripherals
Adafruit_MAX31856 max = Adafruit_MAX31856(10, 11, 12, 13);
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

//set window size value
int WindowSize = 5000;

void setup()
{
  //initialize variables for timing parameters
  startMillis = millis();
  intMillis = startMillis;

  //////////////////////////////////////////////////
  //MUST SELECT WHICH ROUTINE TO RUN
  /////////////////////////////////////////////////
  if (selectRoutine == 1){
      Setpoint = 75; // initialize to the halfway point between room temperature and OpTemp1;
      rateA = 10; //10 degrees per minute
      rateB = 6; //6 degrees per minute
      idealOpTemp1 = 650;
      idealOpTemp2 = 975;
      //set the hold period at which we maintain the target temperature
      minutes1 = .01; //close to 0 minute hold during idealOpTemp transition
      minutes2 = 600; //10 hours hold at idealOpTemp2
      hasItRunA = 0;
      hasItRunB = 0;
  }
  else if (selectRoutine == 2){
     Setpoint = 350; // initialize to the halfway point between room temperature and OpTemp1;
     rateA = 5; //10 degrees per minute
     rateB = 20;
     idealOpTemp1 = 700;
     idealOpTemp2 = 930;
     //set the hold period at which we maintain the target temperature
     minutes1 = 20;
     minutes2 = 90;
     hasItRunA = 0;
     hasItRunB = 0;
    }

    pinMode(RelayPin, OUTPUT); //set the output pin
    Serial.begin(115200);     //for debugging
    lcd.begin(16, 2);         //initialize screen
    max.begin();               //initialize thermocouple
    max.setThermocoupleType(MAX31856_TCTYPE_K); //Type K thermocouple used

    //////////////LCD Screen Control //////////////////////////////
    //Extra -> can set backlight colors to correspond to heating actions
    lcd.setBacklight(WHITE);
    pinMode(RelayPin, OUTPUT);

    //tell the PID to range between 0 and the full window size
    myPID.SetOutputLimits(0, WindowSize);

    //turn the PID on
    myPID.SetMode(AUTOMATIC);

    Serial.print("Thermocouple type: "); //to be used for debugging
    switch ( max.getThermocoupleType() ) {
      case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
      case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
      case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
      case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
      case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
      case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
      case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
      case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
      case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
      case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
      default: Serial.println("Unknown"); break;
    }
}

/////////////////////////////////////////////////////////////
//DEFINING FUNCTIONS TO BE CALLED FROM LOOP
/////////////////////////////////////////////////////////////

//This heat routine determines how long to write High/Low to solid state relay based on PID calculation
//fraction is defined in the loop to be Output of PID computation divided by a constant
void heat(int fraction) {
  if (fraction > 0) {
    digitalWrite(RelayPin, HIGH);
    delay(max(fraction,10)*100);
    digitalWrite(RelayPin, LOW);
    delay((10-fraction)*100);
  }
  //printing T=temperature to the LCD screen
  Input = max.readThermocoupleTemperature();
  lcd.setCursor(0,0);
  lcd.print("T=");
  lcd.print(Input);
  lcd.print("(");
  lcd.print(int(Setpoint));
  lcd.print(">");

  //for debugging transition from idealOpTemp1 to idealOpTemp2 by printing to LCD screen
  /*if (Setpoint <= idealOpTemp1){
    lcd.print(idealOpTemp1);
  }
  else {
    lcd.print(idealOpTemp2);
  }

  lcd.print(")");
  lcd.setCursor(0,1);
  lcd.print("Heating ");
  lcd.print(fraction);
  lcd.print("0%");    //to print heating as percentage of max intensity*/
}

//DECLARATION OF GLOBAL VARIABLES TO TRACK TIME CHANGE
int n = 1; //used to set rate of change of setpoint for idealOpTemp1
int na = 1; //used to set rate of change of setpoint for idealOpTemp2

void timeToTemp(){

   if (Setpoint < idealOpTemp1 && Input<idealOpTemp1){
     endMillis = millis();

      //Use n counter to standardize incremementation to one per minute
      if (endMillis>= (60000*n)){
        Setpoint = Setpoint + rateA;
        n+=1;
        myPID.Compute();
        fraction = Output / 50;
        heat(fraction);
        timeToTemp();
      }

     //Print calls to LCD screen for debugging
     //lcd.clear();
     //lcd.setCursor(0,0);
     //lcd.print(endMillis);
     //lcd.setCursor(0,1);
     //lcd.println(endMillis/6000);
    }

   //hasItRun variable signals if maintainTemp function has already been called
   else if (Input >= idealOpTemp1 && hasItRunA == 0){
      Setpoint = idealOpTemp1;
      maintainTemp(minutes1);
   }

   else if (Input >= idealOpTemp1 && hasItRunA == 1){
      //to signal transition between
      if (Setpoint < idealOpTemp2 && Input<idealOpTemp2){

         endMillis = millis();

         if (endMillis>= (60000*na)){
          Setpoint = Setpoint + rateB;
          na +=1;
          myPID.Compute();
          fraction = Output / 50;
          heat(fraction);
          timeToTemp();
         }

         //Printing to LCD screen for debugging purposes
         //lcd.clear();
         //lcd.setCursor(0,0);
         //lcd.print(endMillis);
         //lcd.setCursor(0,1);
         //lcd.println(endMillis/6000);
       }

      else if (Input >= idealOpTemp2 && hasItRunB == 0){
        hasItRunB = 1;
        Setpoint = idealOpTemp2;
        maintainTemp(minutes2);
      }

      else if (Input >= idealOpTemp2 && hasItRunB == 1){
        //after the maintenance hold period is completed, ramp the temperature back down to room temperature
        reduceToRoomTemp();
      }
  }
}

void maintainTemp(double minutes){
     //Print calls for debugging
     //lcd.clear();
     //lcd.print("maintain temp");
     //delay(1000);

     unsigned long timeHoldStart = millis(); //grab the time at the start of this loop
     myPID.Compute(); //function call to PID library
     fraction = Output / 50;
     heat(fraction);
     unsigned long timeHoldEnd = millis(); //grab the time after one heat routine

     //while the duration of this program is less than the specified time in milliseconds
     while ((timeHoldEnd-timeHoldStart) < 60000*minutes) //60000 factor to convert minute variable into milliseconds
     {
       myPID.Compute();
       fraction = Output / 50;
       heat(fraction);
       timeHoldEnd = millis(); //update the total run time at the end of each call

       if ((timeHoldEnd-timeHoldStart) >= 60000*minutes){ //set hasItRun to indicate completion of maintainTemp routine
          hasItRunA = 1;
          timeToTemp();  //return to timeToTemp function to ensure that conditions are met to proceed
       }
     }

}

void reduceToRoomTemp(){
  //temperature reduction implemented by writing LOW to solid state relay
   digitalWrite(2, LOW);
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Reducing Temp");

   while(1){
       //to print temperatures of ramp back to room temperature
       lcd.clear();
       lcd.print("T=");
       lcd.print(max.readThermocoupleTemperature());
       lcd.setCursor(0,1);
       lcd.print("NOT HEATING");
       delay(200);
   }
}

void loop()
{
  uint8_t buttons = lcd.readButtons(); //initialize lcd buttons -> for future use
  Input = max.readThermocoupleTemperature();

  //print to monitor for debugging
  Serial.print("Thermocouple Temp: ");
  Serial.println(Input);

  lcd.setCursor(0,0);
  lcd.print("T=");
  lcd.print(Input);
  lcd.print("(");
  lcd.print(int(Setpoint));
  lcd.print(">");

  //for debugging switch between idealOpTemp1 and idealOpTemp2
  /*if (Setpoint <= idealOpTemp1){
    lcd.print(idealOpTemp1);
  }
  else {
    lcd.print(idealOpTemp2);
  }
  lcd.print(")");*/

  //SAFETY FIRST
   if (Input > 975) {
    digitalWrite(RelayPin, LOW);
    lcd.setCursor(0,1);
    lcd.print("TOO HOT!!");
    delay(15000);
    }

  else {
    myPID.Compute();
    fraction = Output / 50;
    heat(fraction);
    timeToTemp();
  }
}
