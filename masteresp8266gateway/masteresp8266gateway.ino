#include <SPI.h>
#include <LoRa.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;

const char* ssid = "Hello";   // your network SSID (name) 
const char* password = "helloworld";   // your network password

//WiFiClient  client;

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "O8dze4KGjESasawgZJfFanvsK7x5WdkZ3CENEUgN9Ih2f_KdtaFCwihJ1U6Voi7yPDPPXcciXYSOKO-2tvquiw=="
#define INFLUXDB_ORG "6b815d5741bf3497"
#define INFLUXDB_BUCKET "test_water_monitoring"

// Time zone info
#define TZ_INFO "UTC5.5"

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
  
// Declare Data point
Point sensor("test_monitoring");

unsigned long myChannelNumber =2443484;
const char * myWriteAPIKey = "Z1378HFGRYBA7OWW";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// Variable to hold temperature readings
float fph,ftds,ftemp;
float ftur,fec;
String myStatus = "";


byte MasterNode = 0xFF;     
byte Node1 = 0xBB;
String SenderNode = "";
String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
String incoming = "";

void setup() {
  LoRa.setPins(15,16,2);
  Serial.begin(9600);
 // while (!Serial);
  
  Serial.println("LoRa Receiver");

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid,password);   
  //ThingSpeak.begin(client);  // Initialize ThingSpeak


  // Connecting to a WiFi network
  
 Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  //WiFi.begin(ssid, password);
  
 while (wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      //delay(100);
    }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  yield();
  
  // Check server connection
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    
    while (1);
  }

  sensor.addTag("device", "ESP8266");
  sensor.addTag("Node", "1");
}

void loop() {
  onReceive(LoRa.parsePacket());
  yield();
  }

  void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  if( sender == 0XBB )
  SenderNode = "Node1:";
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length


  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {  
     // check length for error
    Serial.println(incomingLength);
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != Node1 && recipient != MasterNode) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  //if message is for this device, or broadcast, print details:
  Serial.println("\nReceived from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();

  ftds=(getValue(incoming, ',', 0)).toFloat(); 
  ftur =(getValue(incoming, ',', 1)).toFloat(); 
  fph =(getValue(incoming, ',', 2)).toFloat(); 
  ftemp =(getValue(incoming, ',', 3)).toFloat(); 
  fec=(getValue(incoming, ',', 4)).toFloat();
  
  
  Serial.print(fph);
  Serial.println(" pH");
  Serial.print(ftemp);
  Serial.println(" C ");
  Serial.print(ftds);
  Serial.println(" ppm");
  Serial.print(ftur);
  Serial.println(" NTU");
  Serial.print(fec);
  Serial.print(" uS/cm");

  delay(100);
  //Serial.print(quality(ftur));
  
  //cloud();

  influxdb();
  
  incoming = "";
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
 
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String quality(int ftur)
{
  if (ftur <= 30) {
    return(" its CLEAR ");
  }
  if ((ftur > 30) && (ftur <= 60)) {
     return(" its CLOUDY ");
  }
  return(" its DIRTY ");
  
}

void cloud()
{
  if ((millis() - lastTime) > timerDelay) {
    
    ThingSpeak.setField(1, ftur);
    //ThingSpeak.setField(1, temperatureF);
    ThingSpeak.setField(2, ftds);
    ThingSpeak.setField(3,ftemp);
    ThingSpeak.setField(4,fph);
    ThingSpeak.setField(5,fec);
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    int x = ThingSpeak.writeFields(myChannelNumber,myWriteAPIKey);

    if(x == 200){
      Serial.println("\nChannel update successful.");
    }
    else{
      Serial.println("\nProblem updating channel. HTTP error code " + String(x));
    }
    lastTime=millis();
    }
}

void influxdb()
{
  // Clear fields for reusing the point. Tags will remain the same as set above.
    sensor.clearFields();
  
    // Store measured value into point
    
    sensor.addField("Turbidity (NTU)", ftur);
    sensor.addField("TDS (ppm)", ftds);
    sensor.addField("Temp (C)", ftemp);
    sensor.addField("pH", fph);
    sensor.addField("Conductivity (uS/cm)", fec);
  
  
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(sensor.toLineProtocol());
  
    // Check WiFi connection and reconnect if needed
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Wifi connection lost");
    }
  
    // Write point
    if (!client.writePoint(sensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }
  
    Serial.println("Waiting 1 second");
    delay(1000);
}
