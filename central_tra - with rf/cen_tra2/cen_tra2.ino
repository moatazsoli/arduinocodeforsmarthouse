/*
  sketch communicating over Serial using JSON and using Ethernet services
  By Moataz Soliman
  This Code is for the Central Transmitter Board
 */
#include <aJSON.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>

// MAC Address of your Arduino Ethernet shield
byte mac[] = { 0x90,0xA2,0xDA,0x00,0x55,0x8D};
// The IP address of your LAN used by your Ethernet shield
EthernetClient client;
char *myID = "2";
SoftwareSerial mySerial(2, 3); // RX, TX
SoftwareSerial RFSerial(4, 5); // RX, TX
unsigned long last_print = 0;
unsigned long last_time = 0;
aJsonStream serial_stream(&mySerial);
int i =0;
byte val = 0;
byte code[6];
byte checksum = 0;
byte bytesread = 0;
byte tempbyte = 0;
String RFdata = String();
String data = String();
void setup()
{
  Serial.begin(9600);
  mySerial.begin(9600);
  RFSerial.begin(9600);
  Ethernet.begin(mac);
  Serial.println("ready");
  
}

/* Process message like: { "pwm": { "8": 0, "9": 128 } } */
void processMessage(aJsonObject *received_msg) {
  Serial.println("working");
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
    aJson.deleteItem(ack);                // deallocate the ack object , this might solve the ack method that gets stuck there
    delay(100);
    char *rec_source = aJson.getObjectItem(received_msg, "source")->valuestring;
    char *rec_dest = aJson.getObjectItem(received_msg, "destination")->valuestring;
    int token = aJson.getObjectItem(received_msg, "token")->valueint;
    int temp = aJson.getObjectItem(received_msg, "temp")->valueint;
    int prox = aJson.getObjectItem(received_msg, "prox")->valueint;
    int amb = aJson.getObjectItem(received_msg, "amb")->valueint;
    int hum = aJson.getObjectItem(received_msg, "hum")->valueint;
    Serial.println("Printing");
    Serial.println((String)rec_source);
    Serial.println((String)rec_dest);
//    Serial.println(rec_dest3);
//    Serial.println(rec_dest4);
//    Serial.println(rec_dest5);
//    Serial.println(rec_dest6);
//    Serial.println(rec_dest7);
    data="";
    data.concat("{");
    data.concat("\"token"); data.concat("\":"); data.concat(token); data.concat(",");
    data.concat("\"temperature"); data.concat("\":"); data.concat(temp); data.concat(",");
    data.concat("\"proximity"); data.concat("\":"); data.concat(prox); data.concat(",");
    data.concat("\"ambient"); data.concat("\":"); data.concat(amb); data.concat(",");
    data.concat("\"humidity"); data.concat("\":"); data.concat(hum); data.concat("}");
 
    
    Serial.println(data);
    
if (client.connect("smarthouse.appspot.com",80) && data.length() > 0) {
    Serial.println("connected");
    client.println("POST /rest HTTP/1.1");
    client.println("Host: smarthouseiot.appspot.com");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    client.println();

    //Prints your post request out for debugging
    Serial.println("POST /rest HTTP/1.1");
    Serial.println("Host: smarthouseiot.appspot.com");
    Serial.println("Content-Type: application/json");
    Serial.println("Connection: close");
    Serial.print("Content-Length: ");
    Serial.println(data.length());
    Serial.println();
    Serial.print(data);
    Serial.println();
  }

  if (client.connected() && data.length() > 0) {
  Serial.println();
  Serial.println("disconnecting.");
  client.stop();
  }
    
//    Serial.print("{");
//    Serial.print("\"token"); Serial.print("\":"); Serial.print(rec_dest3); Serial.print(",");
//    Serial.print("\"temperature"); Serial.print("\":"); Serial.print(rec_dest4); Serial.print(",");
//    Serial.print("\"proximity"); Serial.print("\":"); Serial.print(rec_dest5); Serial.print(",");
//    Serial.print("\"ambient"); Serial.print("\":"); Serial.print(rec_dest6); Serial.print(",");
//    Serial.print("\"humidity"); Serial.print("\":"); Serial.print(rec_dest7); Serial.println("}");
  }
    
  
}

void RFID(){
    if((val = RFSerial.read()) == 2) {                  // check for header 
      bytesread = 0; 
      while (bytesread < 12) {                        // read 10 digit code + 2 digit checksum
        if( RFSerial.available() > 0) { 
          val = RFSerial.read();
          if((val == 0x0D)||(val == 0x0A)||(val == 0x03)||(val == 0x02)) { // if header or stop bytes before the 10 digit reading 
            break;                                    // stop reading
          }

          // Do Ascii/Hex conversion:
          if ((val >= '0') && (val <= '9')) {
            val = val - '0';
          } else if ((val >= 'A') && (val <= 'F')) {
            val = 10 + val - 'A';
          }

          // Every two hex-digits, add byte to code:
          if (bytesread & 1 == 1) {
            // make some space for this hex-digit by
            // shifting the previous hex-digit with 4 bits to the left:
            code[bytesread >> 1] = (val | (tempbyte << 4));

            if (bytesread >> 1 != 5) {                // If we're at the checksum byte,
              checksum ^= code[bytesread >> 1];       // Calculate the checksum... (XOR)
            };
          } else {
            tempbyte = val;                           // Store the first hex digit first...
          };

          bytesread++;                                // ready to read next digit
        } 
      } 

      // Output to Serial:

      if (bytesread == 12) {        // if 12 digit read is complete
        RFdata = "";
        
      RFdata.concat("{\"user\":");
      //String data = String();
        Serial.print("5-byte code: ");
        for (i=0; i<5; i++) {
          if (code[i] < 16) Serial.print("0");
          Serial.print(code[i],HEX);
          Serial.print(" ");
          RFdata.concat(code[i]);
        }
        RFdata.concat("}");
        Serial.println("data:");
        Serial.println(RFdata);
        Serial.print("Checksum: ");
        Serial.print(code[5],HEX);
        Serial.println(code[5] == checksum ? " -- passed." : " -- error.");
        Serial.println();
        if (client.connect("smarthouse.appspot.com",80) && RFdata.length() > 0) {
          Serial.println("connected");
          client.println("POST /updateRFID HTTP/1.1");
          client.println("Host: smarthouseiot.appspot.com");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.print("Content-Length: ");
          client.println(RFdata.length());
          client.println();
          client.print(RFdata);
          client.println();
      
          //Prints your post request out for debugging
          Serial.println("POST /updateRFID HTTP/1.1");
          Serial.println("Host: smarthouseiot.appspot.com");
          Serial.println("Content-Type: application/json");
          Serial.println("Connection: close");
          Serial.print("Content-Length: ");
          Serial.println(RFdata.length());
          Serial.println();
          Serial.print(RFdata);
          Serial.println();
        }
      
        if (client.connected() && RFdata.length() > 0) {
        Serial.println();
        Serial.println("disconnecting.");
        client.stop();
        }
      }

    }
}


void loop()
{
  if (serial_stream.available()) {
    /* First, skip any accidental whitespace like newlines. */
    serial_stream.skip();
  }

  if (serial_stream.available()) {
    /* Something real on input, let's take a look. */
    aJsonObject *msg = aJson.parse(&serial_stream);
    processMessage(msg);
    aJson.deleteItem(msg);
  }
  
   if(RFSerial.available() > 0) {
    i = 0;
    val = 0;
    code[6];
    checksum = 0;
    bytesread = 0;
    tempbyte = 0;
    RFID();
    i = 0;
  }
  
  
}
