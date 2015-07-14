/*******************************************************/
/*       Arduino UNO with Arduino WiFi Shield          */
/*
This program helps you to update data from the sensors 
to C2M Cloud based on MQTT protocol. 
Design for Internet of Things edge device!
Change ssid, password, topic everytime for your
unique parameters.
Message format should be
apikey="apikey",feedID="feedid",feed=para1,value|para2,value|para3,value
Compiled Successful on Feb 2 2015
By Jason/Ge Wu
/******************************************************/
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "dht11.h"         //Library of the Humidity Sensor DHT11
#include "PubSubClient.h"  //Library of the MQTT Protocol
#include <SPI.h>

// Update these with values for your network.
byte mac[] = { 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F };    //Change for your unique MAC address
byte server[] = { 54, 191, 2, 249 };  //C2M MQTT Server
char ssid[] = "";                     // Replace for The Wireless Network SSID 
char pass[] = "";                     // Replace for The Network password
char topic[] = "";                    //Replace for the Topic of your specific project! Like "Product_1"
int status = WL_IDLE_STATUS;          // Status of WiFi network

char apikey[]=""; //Replace for your own C2M API (Find from your account)
char feedid[]=""; //Replace for your own C2M FeedID (Find from your account)

char str [201];
char buffer[5];

void callback(char* topic, byte* payload, unsigned int length) 
{
  // handle message arrived
}

WiFiClient wfClient;
PubSubClient client(server, 5001, callback, wfClient); 

// set pin numbers:
#define DHT11PIN 2         //DHT11 Sensor connected to PIN 2
dht11 DHT11;

//convert an unsigned int to a four character string
char* getInt(unsigned int num)
{
    memset(buffer,'\0',2);   
    sprintf (buffer, "%02i", num); 
    return buffer;                 
}

void setup()
{
    Serial.begin(9600);                 //Serial Communication Baud Rate
    if (WiFi.status() == WL_NO_SHIELD)  // Check for the presence of the shield:
    {
        Serial.println("WiFi shield does not present");
        while(true);
     }
      while ( status != WL_CONNECTED) 
      {
          Serial.print("Attempting to connect to WiFi SSID: ");
          Serial.println(ssid);
          status = WiFi.begin(ssid, pass);
          delay(10000);
       }
      Serial.println("You've connected to the network");
      printWifiData();
      printCurrentNet();
}

void loop()
{
  Serial.println("Connecting to C2M_MQTT Server...");
  if(!client.connected())
  {
    client.connect(topic);
    Serial.println("Connected to the Server.");
    delay(5000);
  }
  else
  {
     Serial.println("Connection Fail.");
  }
   memset(str,'\0',200);
   strcat(str,"apikey:");
   strcat(str,apikey);
   strcat(str,",feedid:");
   strcat(str,feedid);
   strcat(str,",feed=Light,");
   strcat(str,getInt(readlight(analogRead(A0))));
   strcat(str,"|Proximity,0|Humidity,");
   int Humi_value = Sensor_DHT11(1);  //Read the Humidity
   strcat(str,getInt(Humi_value));
   strcat(str,"|Temperature,");
   int Temp_value1 = Sensor_DHT11(2); //Read the Temp
   strcat(str,getInt(Temp_value1));
    
   Serial.println(str);  //Check the Message 
       
    if(client.publish(topic,str)) 
    {
        Serial.println("Publish Success.");
    }
    else 
    {
         Serial.println("Publish Fail."); 
    }
    delay(2000);   //Reduce or increase the delay time for your own project
    client.loop();
}

/***************** WiFi Status *******************/
void printWifiData() 
{
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC address: ");
    Serial.print(mac[5],HEX);
    Serial.print(":");
    Serial.print(mac[4],HEX);
    Serial.print(":");
    Serial.print(mac[3],HEX);
    Serial.print(":");
    Serial.print(mac[2],HEX);
    Serial.print(":");
    Serial.print(mac[1],HEX);
    Serial.print(":");
    Serial.println(mac[0],HEX);
}

void printCurrentNet() 
{
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(rssi);
    byte encryption = WiFi.encryptionType();
    Serial.print("Encryption Type:");
    Serial.println(encryption,HEX);
}


/*********************** Sensor Code ***************************/
// convert raw light data from sensor to percentage
// To calculate, we multiply the number by 1.1/1024, and
//take the percentage value out of 0.99 volts. 
int readlight(unsigned int lt)
{     
  float tmpflt = (float)lt*1.1/1024;
  int percent;
  
  if(tmpflt > 1.10)  //error handling
      return 0;
  if(tmpflt < 0.00)
      return 0;  
  
  percent = 100*(0.99-tmpflt); 
  if(percent > 100)
     percent = 100;
  if(percent < 0 )
     percent = 0;  
       
  return percent;
}

/************** DHT11 Humidity Sensor Function ****************/
int Sensor_DHT11(int Mode)  // Mode 1: Calculate Humidity, Mode 2: Calculate Temperature(F), Mode 3: Calculate Temperature(C).
{                           // Incorrect selection will return 999
    int temp,humi,fahrenheit; 
    DHT11.read(DHT11PIN);
    humi = DHT11.humidity;       
    temp = DHT11.temperature;
    fahrenheit = temp * 9/5 +32; 
    switch (Mode)
    {
        case 1:
          Serial.print(" Humidity: ");        
          Serial.print(humi);
          Serial.println("% ");
          return humi;
          break;      
        case 2:
          Serial.print(" Temperature: ");
          Serial.print(fahrenheit);
          Serial.println(" F");
          return fahrenheit;
          break;    
        case 3:
          Serial.print(" Temperature: ");
          Serial.print(temp);
          Serial.println(" C");
          return temp;
          break;   
        default: 
          return 999;        
    }         
}
