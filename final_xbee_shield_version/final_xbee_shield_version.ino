/*
  created 8 March 2013
  by Moataz Soliman
  This Code is For the Client Transmitter Board
*/

#include "DHT.h"
#include <Wire.h>
#include <aJSON.h>
#include <SPI.h>
#include <math.h>
#include <SoftwareSerial.h>

unsigned long last_print = 0;
unsigned long last_time = 0;

char *myID = "4";
SoftwareSerial mySerial(10, 11); // RX, TX
aJsonStream serial_stream(&Serial);

#define DHTPIN 7     // what pin we're connected to

#define VCNL4000_ADDRESS 0x13  //I2C Address of the board

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

DHT dht(DHTPIN, DHTTYPE);

aJsonObject *root;

void setup()
{
  Serial.begin(9600);
  mySerial.begin(9600);
  dht.begin();
  Wire.begin();  // initialize I2C stuff
  initVCNL4000(); //initilize and setup the board
}

aJsonObject *createMessage()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  unsigned int ambientValue = readAmbient(); //can a tiny bit slow
  unsigned int proximityValue = readProximity();
  
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    mySerial.println("Failed to read from DHT");
  } else {
    
  aJsonObject *msg = aJson.createObject();
  aJson.addStringToObject(msg, "source", myID);
  aJson.addStringToObject(msg, "destination", "2");
  
  aJson.addNumberToObject(msg,"token",123);
  aJson.addNumberToObject(msg,"temp",(int)t);
  aJson.addNumberToObject (msg,"prox",(int)proximityValue);
  aJson.addNumberToObject(msg,"amb",(int)ambientValue);
  aJson.addNumberToObject(msg,"hum",(int)h);
  
  return msg;
  }
  
}

void wait_for_acknowledgement(aJsonObject *sent_msg){
  mySerial.println("ack method");
  //sent_msg is the message for which we are waiting for acknowledgement
  last_time = millis();
  while(true){
    if(millis() -last_time < 7000 ){
      if(serial_stream.available()){
        aJsonObject *received_msg = aJson.parse(&serial_stream);
        aJsonObject *ack = aJson.getObjectItem(received_msg, "ack");    // to check if the acknowledgement is received
        char *rec_dest = aJson.getObjectItem(received_msg, "destination")->valuestring;
        char *rec_source = aJson.getObjectItem(received_msg, "source")->valuestring;
        char *sen_dest = aJson.getObjectItem(sent_msg, "destination")->valuestring;

         // non of the boards can have the id =0
        if(ack && ((String)(myID)).equals((String)rec_dest) && ((String)(sen_dest)).equals((String)rec_source)){
          // acknowledgement received
          
          mySerial.println("acknowledgement received");
          aJson.deleteItem(received_msg);
          break;   
          
         
        }else{
         mySerial.println("Error in rec_dest or rec_source");
        }
      }
    }else{
      //send again because time ( 7 secs ) ellapsed before receiving the ack.
      aJson.print(sent_msg, &serial_stream);
      mySerial.println("sending again");
      last_time = millis();
    }

  }

}
/*
For sending synchronous messages and waiting for acknowledgement
*/
void sendSyncMessage(aJsonObject *msgToSend){
  aJson.print(msgToSend, &serial_stream);    // send the message
  wait_for_acknowledgement(msgToSend); // wait for acknowledgement
  //aJson.deleteItem(msgToSend);
}

void loop()
{
  if (millis() - last_print > 8000) {
    /* One second elapsed, send message. */
    mySerial.println("sending");
    aJsonObject *msg = createMessage();
    sendSyncMessage(msg);
    aJson.deleteItem(msg);
    last_print = millis();
  }
}


void initVCNL4000(){
  byte temp = readVCNLByte(0x81);

  if (temp != 0x11){  // Product ID Should be 0x11
    mySerial.print("initVCNL4000 failed to initialize");
    mySerial.println(temp, HEX);
  }else{
    mySerial.println("VNCL4000 Online...");
  } 

  /*VNCL400 init params
   Feel free to play with any of these values, but check the datasheet first!*/
  writeVCNLByte(0x84, 0x0F);  // Configures ambient light measures - Single conversion mode, 128 averages
  writeVCNLByte(0x83, 15);  // sets IR current in steps of 10mA 0-200mA --> 200mA
  writeVCNLByte(0x89, 2);  // Proximity IR test signal freq, 0-3 - 781.25 kHz
  writeVCNLByte(0x8A, 0x81);  // proximity modulator timing - 129, recommended by Vishay 
}


unsigned int readProximity(){
  // readProximity() returns a 16-bit value from the VCNL4000's proximity data registers
  byte temp = readVCNLByte(0x80);
  writeVCNLByte(0x80, temp | 0x08);  // command the sensor to perform a proximity measure

  while(!(readVCNLByte(0x80)&0x20));  // Wait for the proximity data ready bit to be set
  unsigned int data = readVCNLByte(0x87) << 8;
  data |= readVCNLByte(0x88);

  return data;
}


unsigned int readAmbient(){
  // readAmbient() returns a 16-bit value from the VCNL4000's ambient light data registers
  byte temp = readVCNLByte(0x80);
  writeVCNLByte(0x80, temp | 0x10);  // command the sensor to perform ambient measure

  while(!(readVCNLByte(0x80)&0x40));  // wait for the proximity data ready bit to be set
  unsigned int data = readVCNLByte(0x85) << 8;
  data |= readVCNLByte(0x86);

  return data;
}


void writeVCNLByte(byte address, byte data){
  // writeVCNLByte(address, data) writes a single byte of data to address
  Wire.beginTransmission(VCNL4000_ADDRESS);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
}


byte readVCNLByte(byte address){
  // readByte(address) reads a single byte of data from address
  Wire.beginTransmission(VCNL4000_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(VCNL4000_ADDRESS, 1);
  while(!Wire.available());
  byte data = Wire.read();

  return data;
}
