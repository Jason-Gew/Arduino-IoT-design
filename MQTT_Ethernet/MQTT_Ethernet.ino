/*
* Arduino Ethernet Monitor/Control Client. Environmental monitoring + Remote control
* Based on MQTT
* Produced By Jason/Ge Wu on Apr/9/2017
*/
#include <Ethernet.h>
#include <SPI.h>
#include "PubSubClient.h"
#include "dht11.h"


dht11 DHT;

#define DHT11_PIN 3
#define LED 5
#define BUZZER 2

const char* mqtt_server = "192.168.0.4";
//const char* mqtt_server = "199.108.99.17";
const int mqtt_port = 1883; 
unsigned long refresh_rate = 30000; // Default refresh rate: 30 second;
const int msg_size = 128;		 // Define your own max message size.

const char *sub_topic = "Jason_CMD";
const char *pub_topic = "Jason_Client";

byte mac[] = { 0xAB, 0xFF, 0xAA, 0x68, 0x66, 0x8A };
IPAddress ip(192, 168, 0, 10);	 // Define your own client IP address if necessary.

EthernetClient ethClient;
PubSubClient client(ethClient);


unsigned long lastMsg = 0;
unsigned int led_status = 0;
unsigned int buzzer_status = 0;
char received[msg_size];

int value = 0;

/* <Execute Cloud Command>: For on/off control, use NAME=ON or NAME=OFF
*	For exact value control, use NAME=VALUE'unit_bit' for example REFRESH=60s
*/
int execute_cmd(char received_cmd[])
{
	String string_cmd(received_cmd);
	if(string_cmd == "LED=ON")
	{
		Serial.println("-> Set Led On!");
		digitalWrite(LED, HIGH);
		led_status = 1;
		return 0;
	}
	else if(string_cmd == "LED=OFF")
	{
		Serial.println("-> Set Led Off!");
		digitalWrite(LED, LOW);
		led_status = 0;
		return 1;
	}
	else if(string_cmd == "BUZZER=ON")
	{
		Serial.println("-> Set Buzzer On!");
		digitalWrite(BUZZER, HIGH);
		buzzer_status = 1;
		return 2;
	}
	else if(string_cmd == "BUZZER=OFF")
	{
		Serial.println("-> Set Buzzer Off!");
		digitalWrite(BUZZER, LOW);
		buzzer_status = 0;
		return 3;
	}
	else if(string_cmd.indexOf("REFRESH=") != -1)
	{
		int unit_bit = string_cmd.lastIndexOf('s');
		if(unit_bit != -1)
		{
			String rate = string_cmd.substring(8, unit_bit);
			Serial.print("-> New Refresh Rate: ");
			Serial.println(rate);
			if(rate != "")
			{
				int temp_rate = rate.toInt();
				if(temp_rate > 15 && temp_rate < 7200)	// refresh rate must be more than 15 seconds and less than 2 hour.
				{
					refresh_rate = (unsigned long)(temp_rate * 1000);
					Serial.print(F("-> Set System New Refresh Rate: "));
					Serial.println(temp_rate);
					return temp_rate;
				}
				else
				{
					Serial.println("-> Invalid Refresh Rate!");
					return -1;
				}
			}
			else
			{
				Serial.println("-> Empty Refresh Rate!");
				return -1;
			}
		}
		else
		{
			Serial.println("-> Unable to find unit bit!");
			return -1;
		} 
	}
	else
	{
		Serial.println("-> Invalid Command!");
		return -1;
	}
}

void callback(char* topic, byte* payload, unsigned int length)
{
	Serial.print(F("-> Received Command: <"));
	Serial.print(topic);
	Serial.print(F("> ["));

	for (unsigned int i=0; i<length; i++) 
	{
		received[i] = (char)payload[i];
	}
	Serial.print(received);
	Serial.println(F("]"));
	int result = execute_cmd(received);
	memset(received, 0, sizeof(received));
} //end callback


void reconnect() 
{
	while (!client.connected()) 
	{
		Serial.println(F("-> Attempting MQTT Connection..."));
		// Create a random client ID
		String clientId = "Arduino-6888";
	//	clientId += String(random(0xffff), HEX);

		//if you MQTT broker has clientID,username and password
		//if(client.connect(clientId, Username, Password))
		if(client.connect(clientId.c_str()))
		{
			Serial.println(F("-> Broker Connected!"));

			client.subscribe(sub_topic);
		}
		else 
		{
			Serial.print(F("-> Failed, rc = "));
			Serial.print(client.state());
			Serial.println(F(" || -> Try to reconnect in 5 seconds"));
	
			delay(5000);
		}
	}
}

void setup() 
{
	Serial.begin(9600);
	pinMode(BUZZER, OUTPUT);
	delay(10);
	pinMode(LED, OUTPUT);
	delay(10);
	digitalWrite(BUZZER, LOW);
	delay(10);
	digitalWrite(LED, LOW);


	Ethernet.begin(mac, ip);
	client.setServer(mqtt_server, mqtt_port);
	client.setCallback(callback);
}

void loop() 
{

	if (!client.connected()) 
	{
		reconnect();
	}
	client.loop();

	unsigned long now = millis();
	
	if (now - lastMsg > refresh_rate) // read DHT11 sensor every 30 seconds
	{
    lastMsg = now;
		DHT.read(DHT11_PIN);
		String msg = "Temperature,";
		msg = msg + String(DHT.temperature);
		msg = msg + "|Humidity," ;
		msg = msg + String(DHT.humidity);
		msg = msg + "|Led,";
		msg = msg + String(led_status);
		msg = msg + "|Buzzer,";
		msg = msg + String(buzzer_status);
		msg = msg + " ";
		char message[msg.length()];
		msg.toCharArray(message, msg.length());
		Serial.println(message);

		bool flag = client.publish(pub_topic, message);
		if(flag)
		{
			Serial.println("-> MQTT Publish Success!");
			msg = "";
		}
		else
		{
			Serial.println("-> MQTT Publish Fail!");
		}
	}
}
