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
#define READING_SENSOR_INTERVAL 5000     // Interval to read ALL sensors. 1s interval.
// ------------------------------------------
//  Software Configuration (END)
// ------------------------------------------

// ------------------------------------------
//  PIN Configuration (BEGIN)
// ------------------------------------------

// TDS Meter PIN:
//  TDS Meter    Arduino
//    1           5V
//    2           Pin 15 (RX3)  --> so using Serial3
//    3           Pin 14 (TX3)
//    4           GND

// ------------------------------------------
//  PIN Configuration (END)
// ------------------------------------------


// TDS Meter Configuration (BEGIN)
#define TDS_METER_SERIAL_BAUD 9600
// TDS Meter Configuration (END)

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                    FUNCTIONS
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{

  initTdsMeter();

  unsigned int highbit = 0x01;
  unsigned int lowbit = 0xF4;
  int value =  highbit << 8 | lowbit;
  Serial.println(String("Value:") + value);

  
}



void loop(){


  while (Serial3.available()){ // clear all the Serial buffer first.
    Serial3.read();
  }
  // A5 03 00 00 00 09 9C E8 
  //byte message[] = {0xA5, 0x04, 0x00, 0x03, 0x00, 0x04, 0x18, 0xED};

  byte message[] = {0xA5, 0x03, 0x00, 0x00, 0x00, 0x09, 0x9C, 0xE8}; // read all registers  (can use simply modbus to find CRC code)
    // -> Receive Packet[count]23     Default value: A5 3 12 0 3 0 0 1 F4 3F 80 0 0 0 0 0 0 0 0 0 0 4C 7E  
  
  //byte message[] = {0xA5, 0x03, 0x00, 0x02, 0x00, 0x01, 0x3C, 0xEE}; // read tds 轉換系數
    // ->  Receive Packet[count]7     Default value: A5 3 2 1 F4 C9 8A  [0x01 0xF4 == 500]


  
  Serial3.write(message, sizeof(message));
  Serial3.flush();
  
  delay(1000); 
  
  int count=0;
  unsigned char frame[128];
  while (Serial3.available()){
    frame[count++]=Serial3.read();
  }
  Serial.println(String("Receive Packet[count]") + count);
  for (int i=0; i<count;i++){
   Serial.print(frame[i],HEX);
   Serial.print(" ");
  }
Serial.println(" ");

  delay(READING_SENSOR_INTERVAL);
}



void initTdsMeter(){
  //pinMode(2, OUTPUT);
  Serial3.begin(TDS_METER_SERIAL_BAUD, SERIAL_8E1);
  Serial.begin(9600);
}





// put it to global variables (tds, temperature)
void readFromTdsMeter(){
  while (Serial3.available()){ // clear all the Serial buffer first.
    Serial3.read();
  }
  // digitalWrite(2, HIGH);
  
  //Request (This)
  //A5 04 00 03 00 04 18 ED
  //  Starting Address: 03
  //Response
  //A5 04 08 02 10 00 00 0A 21 00 00 F0 F5
  //02 10 -> TDS = 528 / 10 = 52.8 ppm
  //0A 21 -> Temperature = 2593 / 100 = 25.93 oC
  
  byte message[] = {0xA5, 0x04, 0x00, 0x03, 0x00, 0x04, 0x18, 0xED};
  Serial3.write(message, sizeof(message));
  Serial3.flush();
  
  delay(100); 
  
  int count=0;

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
    //tds=(highBitTds<<8 | lowBitTds)/10.0;
    //temperature =  (highBitTemp<<8 | lowBitTemp)/100.0;
    //Serial.println(String("[tds]") + tds + "[temperature]" + temperature +".");
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






