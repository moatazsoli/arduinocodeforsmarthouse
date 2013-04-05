/*
  updated 12 March 2013
  by Moataz Soliman
  This Code is For the Client Receiver Code
*/

#include <aJSON.h>
#include <SoftwareSerial.h>
char *myID = "3";
SoftwareSerial mySerial(2, 3); // RX, TX
unsigned long last_print = 0;
unsigned long last_time = 0;
aJsonStream serial_stream(&mySerial);
int i;


/* 
Process messages like: { "pwm": { "8": 0, "9": 128 } } 
and send an acknowledgement
*/
void processMessage(aJsonObject *received_msg) {
  aJsonObject *rec_dest_obj =aJson.getObjectItem(received_msg, "destination");
  aJsonObject *rec_source_obj = aJson.getObjectItem(received_msg, "source");
  char *rec_dest = rec_dest_obj->valuestring;
  char *rec_source = rec_source_obj->valuestring;
  Serial.println("start processing");
  if( ((String)(myID)).equals((String)rec_dest) ) {
    delay(10);
    Serial.println("replaying");
    aJsonObject *ack = aJson.createObject();
    aJson.addStringToObject(ack, "source", myID);
    aJson.addStringToObject(ack, "destination", rec_source);
    aJson.addStringToObject(ack, "ack", "");
    
    aJson.print(ack, &serial_stream);     // sending acknowledgement
    delay(100);
    //aJson.deleteItem(ack);
    //aJson.deleteItem(rec_dest_obj);
    //aJson.deleteItem(rec_source_obj);
    // START PROCESSING 
    aJsonObject *commands = aJson.getObjectItem(received_msg, "commands");
    if (!commands) {
      Serial.println("no data");
      return;
    }else{
      // process commands
      Serial.println("msgs recieved");
      //Serial.println(commands);
      processCommands(commands);
      Serial.println("gottt");
      //aJson.deleteItem(commands);
      Serial.println("goooooott");
    }
    
  }
}

/*
To Process commands received. 
Input example : { "8": "H", "9": "L" }
*/
void processCommands(aJsonObject *commands) {
  const static int pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
  const static int pins_n = 14;
  for (int i = 0; i < pins_n; i++) {
    //Serial.println(i);
    char pinstr[3];
    snprintf(pinstr, sizeof(pinstr), "%d", pins[i]);

    aJsonObject *command = aJson.getObjectItem(commands, pinstr);
    if (!command) continue; /* Value not provided, ok. */
    if (command->type == aJson_Int) {
      /* analog value */
      Serial.println("Analog data type ");
      Serial.print("setting pin ");
      Serial.print(pins[i], DEC);
      Serial.print(" to value ");
      Serial.println(command->valueint, DEC);
      analogWrite(pins[i], command->valueint);
      continue;
    }else if(command->type == aJson_String) {
      /* digital value*/
      Serial.print("Digital data type ");
      Serial.print("setting pin ");
      Serial.print(pins[i], DEC);
      Serial.print(" to value ");
      Serial.println(command->valuestring);
      if(((String)command->valuestring).equals("H")) {
        digitalWrite(pins[i], HIGH);
      }else if(((String)command->valuestring).equals("L")) {
        digitalWrite(pins[i], LOW);
      } 
    }
    //aJson.deleteItem(command);
  }
  
  Serial.println("Got here");
}

void setup(){
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.println("Ready");
  for(i = 4; i < 10; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  for(i = 14; i < 20; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
}
void loop(){
  if (serial_stream.available()) {
    /* First, skip any accidental whitespace like newlines. */
    Serial.println("got an empty message");
    serial_stream.skip();
  }
  if (serial_stream.available()) {
    Serial.println("got a message");
    /* Something real on input, let's take a look. */
    aJsonObject *msg = aJson.parse(&serial_stream);
    processMessage(msg);
    Serial.println("got here 2");
    aJson.deleteItem(msg);
    Serial.println("got here 3");
  }
}
