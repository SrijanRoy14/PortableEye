#include <SPI.h>
#include <LoRa.h>
#include "GravityTDS.h"
#include <OneWire.h>
#include <DallasTemperature.h>



float tdssensor();
float turbidity();
float ph();
float Temp();
float round_to_dp(float,int);
float Cond();

float Tempv = 0.0,tdsValue = 0;
float calibration_value = 24.25;
int phval = 0; 
unsigned long int avgval; 
int buffer_arr[10],temp;
int counter = 0;
String Mymessage;              // outgoing message
byte MasterNode = 0xFF;     
byte Node1 = 0xBB;

#define ONE_WIRE_BUS 4
#define TdsSensorPin A4
#define turbpin A0
#define phpin A1

GravityTDS gravityTds;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  
  sensors.begin();

  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization
}

void loop() {
  
  Serial.println("\n\n");
  Serial.print("Sending packet: ");
  Serial.println(counter);
  
  float tds=tdssensor();
  delay(1000);
  float tur=turbidity();
  delay(1000);
  float phv=ph();
  delay(1000);
  Tempv=Temp();
  delay(1000);
  float ec=Cond();
  delay(1000);
  
  Mymessage = Mymessage+ tds + ","  + tur + "," + phv+","+Tempv+","+ec;
  sendMessage(Mymessage,MasterNode,Node1);
  delay(100);    Mymessage = "";
  delay(10000);
  
}
void sendMessage(String outgoing, byte MasterNode, byte otherNode) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(MasterNode);              // add destination address
  LoRa.write(Node1);             // add sender address
  LoRa.write(counter);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  counter++;                           // increment message ID
}

float tdssensor()
  {
    //temperature = readTemperature();  //add your temperature sensor and read it
    gravityTds.setTemperature(Tempv);  // set the temperature and execute temperature compensation
    gravityTds.update();  //sample and calculate
    tdsValue = gravityTds.getTdsValue();  // then get the value
    
    Serial.print(tdsValue);
    Serial.println(" ppm");
    
    return tdsValue;
  }

float Cond()
{
  float Ecvalue=gravityTds.getEcValue();
  Serial.println();
  Serial.print(Ecvalue);
  Serial.println(" uS/cm");
  return Ecvalue;
}

float turbidity()
  {
    float volt;
    int raw=analogRead(turbpin);
    /*Serial.println();
    Serial.println(raw);
    //Serial.println(raw);
    for(int i=0; i<800; i++)
    {
        volt += ((float)analogRead(turbpin)/1023)*5;
    }*/
    float turb=0.0;
    volt = ((float)analogRead(turbpin)/1023)*5*2.2;
    Serial.println(volt);
    //volt = round_to_dp(volt,2);
    //Serial.println();
    
    //Serial.println(volt);
    if(volt < 2.5){
      turb = 3000;
    }
    else if(volt>4.2)
    {
      turb=5.2;
    }

    
    else{
      turb = -1120.4*square(volt)+5742.3*volt-4353.8; 
    }

    float turb1=map(turb,5,3000,12,650);
    turb1=turb1/10;
    Serial.print(turb1);
    Serial.println(" NTU");
return turb1;
}
  
float ph()
  {
    /*for(int i=0;i<10;i++) 
    { 
      buffer_arr[i]=analogRead(phpin);
      delay(30);
    }
      for(int i=0;i<9;i++)
      {
        for(int j=i+1;j<10;j++)
        {
          if(buffer_arr[i]>buffer_arr[j])
          {
            int temp2=buffer_arr[i];
            buffer_arr[i]=buffer_arr[j];
            buffer_arr[j]=temp2;
          }
        }
      }
      avgval=0;
      for(int i=2;i<8;i++)
      avgval+=buffer_arr[i];*/
      int raw=analogRead(phpin);
      float volt=(float)analogRead(phpin)*5/1024*0.5081300813;
      //float volt2=(float)(avgval/6)*5/1024;
      //Serial.println();
      Serial.print("Raw analog reading of Ph: ");
      //Serial.println(avgval/6);
      Serial.println(raw);
      //Serial.println(volt2);
      //Serial.println();

      float ph_act = -5.70 * volt + calibration_value;
      
      Serial.print("Voltage pH: ");
      Serial.println(volt);
      Serial.print("pH Val: ");
      Serial.println(ph_act);
 //Serial.println("\n\n");

 return ph_act;
 
}

float Temp()
{
  // Send the command to get temperatures
  sensors.requestTemperatures(); 

  //print the temperature in Celsius
  Serial.print("Temperature: ");
  Serial.print(sensors.getTempCByIndex(0));
  //Serial.print((char)176);//shows degrees character
  Serial.print(" C ");
  return sensors.getTempCByIndex(0);
}

float round_to_dp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}
  
