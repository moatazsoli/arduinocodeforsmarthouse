/*
Moataz Soliman 
This code is for the Central Receiver Arduino Board
*/


#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>
#include <aJSON.h>
SoftwareSerial mySerial(2, 3); // RX, TX
unsigned long last_print = 0;
unsigned long last_time = 0;
aJsonStream serial_stream(&mySerial);


// MAC Address of your Arduino Ethernet shield
byte mac[6] = { 0x90, 0xA2, 0xDA, 0x0D, 0x1F, 0x62 };
int parser_counter =0;
char *myID = "1";  // 1 is the central receiver

IPAddress server( 74, 125, 132, 141 );
EthernetClient client;
int p;
int i;
String data = String();
char c;
boolean take;
int waiting;
int n;
int v;
int digits;
int jump;

void setup() {
  for(i = 4; i < 10; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  for(i = 14; i < 20; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  Serial.begin(9600);
  mySerial.begin(9600);
  	waiting = 0;
	while (Ethernet.begin(mac) == 0  && waiting < 800){
		delay(10);
		waiting++;
	}
	//  Serial.println(Ethernet.localIP());

  delay(1000);
  Serial.println("Ready");
}

void wait_for_acknowledgement(aJsonObject *sent_msg){
  Serial.println("ack method");
  //sent_msg is the message for which we are waiting for acknowledgement
  last_time = millis();
  while(true){
    if(millis() -last_time < 7000 ){
      if(serial_stream.available()){
        aJsonObject *received_msg = aJson.parse(&serial_stream);
        aJsonObject *ack = aJson.getObjectItem(received_msg, "ack");    // to check if the acknowledgement is received
        aJsonObject *rec_dest_obj = aJson.getObjectItem(received_msg, "destination");
        aJsonObject *rec_source_obj = aJson.getObjectItem(received_msg, "source");
        aJsonObject *sen_dest_obj = aJson.getObjectItem(sent_msg, "destination");
        char *rec_dest = rec_dest_obj->valuestring;
        char *rec_source = rec_source_obj->valuestring;
        char *sen_dest = sen_dest_obj->valuestring;
        Serial.println("testing values");
        Serial.println((String)rec_dest);
        Serial.println((String)rec_source);
        Serial.println((String)sen_dest);
        Serial.println("testing values");
         // non of the boards can have the id =0
        if(ack && ((String)(myID)).equals((String)rec_dest) && ((String)(sen_dest)).equals((String)rec_source)){
          // acknowledgement received
          
          Serial.println("acknowledgement received");
          aJson.deleteItem(received_msg);
//          aJson.deleteItem(ack);
//          aJson.deleteItem(rec_dest_obj);
//          aJson.deleteItem(rec_source_obj);
//          aJson.deleteItem(sen_dest_obj);
          break;   
            
         
        }else{
         Serial.println("Error in rec_dest or rec_source");
//         aJson.deleteItem(received_msg);
//         aJson.deleteItem(ack);
//         aJson.deleteItem(rec_dest_obj);
//         aJson.deleteItem(rec_source_obj);
//         aJson.deleteItem(sen_dest_obj);
         break;  
        }
      }
    }else{
      //send again because time ( 7 secs ) ellapsed before receiving the ack.
      aJson.print(sent_msg, &serial_stream);
      Serial.println("sending again");
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
  }
  //aJson.deleteItem(commands);
}

void processMessage(aJsonObject *received_msg){
  char *rec_dest = aJson.getObjectItem(received_msg, "destination")->valuestring;
  Serial.println("Verifying the destination");
  if( ((String)(myID)).equals((String)rec_dest) ) {
    Serial.println("received a message , now start processing the commands");
    // START PROCESSING 
    aJsonObject *commands = aJson.getObjectItem(received_msg, "commands");
    if (!commands) {
      Serial.println("no data");
      return;
    }else{
      // process commands
      //aJson.deleteItem(received_msg);
      Serial.println("commands recieved");
      processCommands(commands);
    }
  }else{
    Serial.print("destination=");
    Serial.println((String)rec_dest);
    // add the source as the central rc
    aJson.addStringToObject(received_msg, "source", myID);
    delay(10);
    Serial.println("forwarding to another board.. sending synch msg");
    sendSyncMessage(received_msg);
    
  }  
  
}
void loop() {
  if (client.connect(server, 80)) {
    //Serial.println("started");
    client.println("GET /get HTTP/1.1");
    client.println("Host: smarthouse.appspot.com");
    client.println();
    data = "";
    take = false;
    //Serial.println("           ");
    //Serial.println("reading ...");
    waiting = 0;
    while (!client.available() && waiting < 500){
        delay(10);
        waiting++;
    }
     while (client.available()){
        c = client.read();
        
        if (c == '}'){
          data.concat("}");
          parser_counter--;
          if(parser_counter==0){
            take = false;
            data.concat("}");
            char myString[data.length()];
            data.toCharArray(myString,data.length());
            Serial.println(myString);
            // convert to JSON ... etc
            aJsonObject* jsonObject = aJson.parse(myString);
            processMessage(jsonObject);
            aJson.deleteItem(jsonObject);
            break;
          }
        }
        else if(c == '{'){
          data.concat("{");
          parser_counter++;
          take = true;
        }
        else if(c == '\\' || c == ' '){}
        else {
          if (take){

          data.concat(String(c));
          }
        }
     }

  }
  client.stop();
  //delay(1000);
}

