// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Arduino Software Version v1.6.12
// Arduino Board Mega2560  
//
// Sensors:
//  1. LCD (I2C 4x20)
//  2. Water TDS Meter
//  3. Water Flow Sensor (FS300A)
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------

// ------------------------------------------
//  Software Configuration (BEGIN)
// ------------------------------------------
#define WELCOME_MSG_FIRST_LINE "Amber Caffe Lab."
#define WELCOME_MSG_SECOND_LINE "v1.0 Loading......"
#define READING_SENSOR_INTERVAL 1000     // Interval to read ALL sensors. 1s interval.
// ------------------------------------------
//  Software Configuration (END)
// ------------------------------------------

// ------------------------------------------
//  PIN Configuration (BEGIN)
// ------------------------------------------
// WATER_FLOW_SENSOR_PIN PIN:  (have resistor)
//  WATER_FLOW_SENSOR     Arduino
//      YELLOW               2
//      BLACK               GND
//      RED                 5V
#define WATER_FLOW_SENSOR_PIN 2   

// LCD Display PIN:
//  LCD     Arduino
//  VCC     5V
//  GND     GND
//  SDA     SDA(Pin A4)     (Reference:Arduino Mega2560:Pin 21)
//  SCL     SCL(Pin A5)     (Reference:Arduino Mega2560:Pin 20)

// TDS Meter PIN:
//  TDS Meter    Arduino
//    1           5V
//    2           Pin 15 (RX3)  --> so using Serial3
//    3           Pin 14 (TX3)
//    4           GND

// ------------------------------------------
//  PIN Configuration (END)
// ------------------------------------------

// LCD Display Configuration (BEGIN)
#include <LiquidCrystal_I2C.h>
// Set the LCD address to 0x27 for a 20 chars and 4 line display
//LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x20 for a 16 chars and 2 line display
//LiquidCrystal_I2C lcd(0x27);  // Set the LCD I2C address  (This LCD address: 0x3F)
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address  (This LCD address: 0x3F)
#define LCD_DIGITS_PER_LINE 20  // No. of Characters per LINE
#define LCD_LINES 4  // LCD Lines
// LCD Display Configuration (END)

// TDS Meter Configuration (BEGIN)
#define TDS_METER_SERIAL_BAUD 9600
// TDS Meter Configuration (END)

// FS300A Water Flow sensor Configuration (BEGIN)
// 
// http://wiki.seeedstudio.com/wiki/G3/4_Water_Flow_sensor
// reading liquid flow rate using Seeeduino and Water Flow Sensor from Seeedstudio.com
// Code adapted by Charles Gantt from PC Fan RPM code written by Crenn @thebestcasescenario.com
// http:/themakersworkbench.com http://thebestcasescenario.com http://seeedstudio.com
volatile int waterFlowNbTopsFan; //measuring the rising edges of the signal
// FS300A Water Flow sensor Configuration (END)

// Software variables(BEGIN)
float tds = 0;          // Water TDSL : ppm
float temperature = 0;  // Temperature: degree
int   waterFlow;        // Water Flow : L/hour

#define TEMPERATURE_STR_SIZE 10
#define TDS_STR_SIZE 10
#define FLOW_STR_SIZE 10
char temperatureStr[TEMPERATURE_STR_SIZE];
char tdsStr[TDS_STR_SIZE];
char waterFlowStr[FLOW_STR_SIZE];
// Software variables (BEGIN)

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                    FUNCTIONS
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  initLCD(); // initialize LCD
  displayLCD(WELCOME_MSG_FIRST_LINE, WELCOME_MSG_SECOND_LINE, "","");
  
  initTdsMeter();
  initWaterFlowSensor();
}



void loop(){
  readFromTdsMeter();
  delay(100);
  readFromWaterFlowSensor();

  displayDataOnLCD();
  
  delay(READING_SENSOR_INTERVAL);
}


void initLCD(){
  lcd.begin(LCD_DIGITS_PER_LINE, LCD_LINES); // initialize the LCD
  lcd.backlight(); // Turn on the blacklight and print a message.
  //Serial.println("LCD OK");
}

void initTdsMeter(){
  //pinMode(2, OUTPUT);
  Serial3.begin(TDS_METER_SERIAL_BAUD, SERIAL_8E1);
  Serial.begin(9600);
}



void waterFlowInterrupt () //This is the function that the interupt calls 
{ 
  waterFlowNbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
} 
void initWaterFlowSensor(){
  pinMode(WATER_FLOW_SENSOR_PIN, INPUT); 
  attachInterrupt(0, waterFlowInterrupt, RISING); //and the interrupt is attached
}

void readFromWaterFlowSensor(){
  waterFlowNbTopsFan = 0;   //Set NbTops to 0 ready for calculations
  sei();      //Enables interrupts
  delay (1000);   //Wait 1 second
  cli();      //Disable interrupts
  waterFlow = (waterFlowNbTopsFan * 60 / 5.5); //(Pulse frequency x 60) / 5.5Q, = flow rate in L/hour 
  Serial.print (waterFlow, DEC); //Prints the number calculated above
  Serial.print (" L/hour\r\n"); //Prints "L/hour" and returns a  new line

  sei();      // Enables interrupts again for tds meter measurement.
}


// put it to global variables (tds, temperature)
void readFromTdsMeter(){
  while (Serial3.available()){ // clear all the Serial buffer first.
    Serial3.read();
  }
  // digitalWrite(2, HIGH);
  
  //Request (This)
  //A5 04 00 03 00 04 18 ED
  // Explain: 
  //  A5: Address
  //  04: Function Code
  //  00 03 : Starting Address
  //  00 04 : Numbers of Registers requested
  //  18 ED : CRC Code
  
  //Response
  //A5 04 08 02 10 00 00 0A 21 00 00 F0 F5
  //02 10 -> 進水口 TDS = 528 / 10 = 52.8 ppm
  //0A 21 -> 進水口 Temperature = 2593 / 100 = 25.93 oC
  
  byte message[] = {0xA5, 0x04, 0x00, 0x03, 0x00, 0x04, 0x18, 0xED};
  Serial3.write(message, sizeof(message));
  Serial3.flush();
  
  delay(500); // 0.5 sec : waiting time for the sensor register writing (dont less than 0.5s), otherwise it may cause invalid packet received.
  
  int count=0;
  tds=0;
  temperature = 0;   
  unsigned char frame[128];
  while (Serial3.available()){
    frame[count++]=Serial3.read();
  }
  Serial.println(String("Receive Packet[count]") + count);
  if ( count==13 && frame[0]==0xA5 ){
   for (int i=0; i<count;i++){
     Serial.print(frame[i],HEX);
     Serial.print(" ");
    }
    Serial.println("");
    unsigned int highBitTds = frame[3];
    unsigned int lowBitTds = frame[4];
    unsigned int highBitTemp = frame[7];
    unsigned int lowBitTemp = frame[8];
    Serial.println(String("[highBitTds]") + highBitTds + "[lowBitTds]" + lowBitTds  + "[highBitTemp]" + highBitTemp + "[lowBitTemp]" + lowBitTemp +".");
    tds=(highBitTds<<8 | lowBitTds)/10.0;     // e.g. 02 0B == 523  (52.3ppm)
    temperature =  (highBitTemp<<8 | lowBitTemp)/100.0; // e.g. 0A 41 == 2625 ( 26.25c )
    Serial.println(String("[tds]") + tds + "[temperature]" + temperature +".");
  } else {
    // packet error
    Serial.println("ERROR: invalid packet received");
    for (int i=0; i<count;i++){
      Serial.println(String("frame[i]") + i+":" + frame[i]);
      Serial.print (String("frame[i][HEX]") + i+":");
      Serial.println(frame[i],HEX);
    }
  }
}





void displayDataOnLCD(){
  String firstLine = WELCOME_MSG_FIRST_LINE;

  memset(temperatureStr, ' ', TEMPERATURE_STR_SIZE);
  memset(tdsStr, ' ', TDS_STR_SIZE);
  memset(waterFlowStr, ' ', FLOW_STR_SIZE);
  dtostrf(temperature, 2, 2, temperatureStr);  //format the display string (2 digits, 1 dp)
  dtostrf(tds, 2, 2, tdsStr); 
  dtostrf(waterFlow, 4, 2, waterFlowStr); 
  
  String secondLine =  String("TEMP:") +  temperatureStr + "c "; 
  String thirdLine = String("TDS :") + tdsStr + "ppm";
  String fourthLine = String("Flow:") + waterFlowStr + "L/hour";
 
  displayLCD(firstLine, secondLine, thirdLine, fourthLine);
}




// firstLine: LCD First Line
// secondLine: LCD Second Line
// thirdLine: LCD Third Line
// fourthLine: LCD Fourth Line
void displayLCD(String firstLine, String secondLine, String thirdLine, String fourthLine) {
 // Serial.println("LCD<BEGIN>");
//  Serial.println(firstLine);
//  Serial.println(secondLine);
//  Serial.println(thirdLine);
//  Serial.println(fourthLine);
//  Serial.println("LCD<END>");
  lcd.clear();
  if (firstLine.length()>0) {
    lcd.setCursor(0, 0);
    lcd.print(firstLine);
  }
  if ( secondLine.length()>0) {
    lcd.setCursor(0, 1);
    lcd.print(secondLine);
  }
  if ( thirdLine.length()>0) {
    lcd.setCursor(0, 2);
    lcd.print(thirdLine);
  }
  if ( fourthLine.length()>0) {
    lcd.setCursor(0, 3);
    lcd.print(fourthLine);
  }
}

