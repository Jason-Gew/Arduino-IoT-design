/*****************************************************************************
 *  GPS Data Logger for UART GPS Module + Arduino Uno/Mega + SD Card + LCD1602
 *  Module Will Create .cvs log file into SD Card with GPS information.
 *	Real-Time GPS data will be displayed on LCD1602.
 *  Sketch requires TInyGPS++ and Grove rgb_lcd Library 
 *
 *  By Jason-Gew 
 *  On Dec/31/2016
 ****************************************************************************/
#include <TinyGPS++.h>
#include "rgb_lcd.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

/************ Definitions ************/
static const int GPS_BAUD = 9600;
static const int sd_select = 4; // SPI CS Pin

//#define Arduino_UNO
#define Arduino_MEGA

#define log_file "gpslog"
#define file_suffix "csv"
#define max_files 199		// Max Record File Number	
#define log_rate 28000		// ms/1000 = second
char file_name[14];			// Limit the Length of File Name

// If column number or name is changed, please also change corresponding settings in each function.
const int parameter_num = 2;
//char * column_names[parameter_num] = {"Latitude", "Longitude", "UTC Date", "UTC Time", "Altitude", "Speed"};
char * column_names[parameter_num] = {"Latitude", "Longitude"};
unsigned long last_log = 0;
bool sd_status = false;

/*************************************/
// The TinyGPS++ Object
TinyGPSPlus gps;

#ifdef Arduino_UNO
#include <SoftwareSerial.h>
	static const int SRX = 5, STX = 6;
	SoftwareSerial ss(SRX, STX);
#endif

// LCD OBject
rgb_lcd lcd;

void setup() 
{
	lcd.begin(16, 2);
	Serial.begin(9600);

#ifdef Arduino_MEGA
	Serial1.begin(GPS_BAUD);
#endif

#ifdef Arduino_UNO
	ss.begin(GPS_BAUD);
#endif

	lcd.setCursor(0, 0);
	lcd.print(" GPS Log System ");
	lcd.setCursor(0, 1);
	lcd.print(" -- Jason Wu -- ");
	delay(2500);
	lcd.clear();
  
	Serial.println(F("\n-> GPS Data (SD) Log System Initializing..."));
	lcd.setCursor(0, 0);
	lcd.print("GPS System ");
	lcd.setCursor(0, 1);
	lcd.blink();
	lcd.print("   Initializing");
  
  
	if (!SD.begin(sd_select))
	{
		Serial.println(F("-> Initializing SD Card Failed!"));
		Serial.println(F("***** Please Check the SD Card and Restart the Program *****"));
		sd_status = false;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("SD Card Failed!");
		delay(50);
		lcd.setRGB(250, 100, 10);
	}
	else
	{
		Serial.println(F("-> SD Card Initialized!\n"));
		sd_status = true;

		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("SD Card Loaded!");
		delay(50);
		lcd.setRGB(150, 255, 0);
	}
	if (sd_status)
	{
		start_new_file();

	}

	Serial.println(F("-> Waiting for GPS Signal ..."));
	Serial.println(F("---------------------------------------------------------------------"));
	Serial.println(F("   Date      Time     Latitude   Longitude   Fix  Sats   Alt   Speed"));
	Serial.println(F("(MM/DD/Year) (24)      (deg)       (deg)     Age  (No)   (m)   (km/h)"));
	Serial.println(F("---------------------------------------------------------------------"));

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Waiting For GPS");
	lcd.setCursor(0, 1);
	lcd.print(file_name);	
}

void loop() 
{
	if ((last_log + log_rate) <= millis())
	{
		if (gps.location.isUpdated())
		{
			printDateTime(gps.date, gps.time);
			Serial.print(F("  "));
			printFloat(gps.location.lat(), true, 11, 6);
			printFloat(gps.location.lng(), true, 12, 6);
			printInt(gps.location.age(), true, 5);
			Serial.print(F(" "));
			printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
			printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
			Serial.print(F(" "));
			printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
			Serial.println();

			lcd.clear();
			lcd.noBlink();
			lcd.setCursor(3, 0);
			char date[11];
			sprintf(date, "%02d-%02d-%02d ", gps.date.year(), gps.date.month(), gps.date.day());
			lcd.print(date);
			lcd.setCursor(4, 1);
			char ts[9];
			sprintf(ts, "%02d:%02d:%02d ", gps.time.hour(), gps.time.minute(), gps.time.second());
			lcd.print(ts);
			delay(3000);

			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Lat: ");
			lcd.print(gps.location.lat(),6);
			lcd.setCursor(0, 1);
			lcd.print("Lng: ");
			lcd.print(gps.location.lng(),6);
			delay(3000);

			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Speed: ");
			lcd.print(gps.speed.kmph(),1);
			lcd.print(" km/h");
			lcd.setCursor(0, 1);
			lcd.print("Satellites: ");
			lcd.print(gps.satellites.value());
			delay(500);	


			int store = logGPSData();
			switch (store)
			{
				case 0:
					Serial.println(F("-> Failed to store GPS data to SD log..."));
					lcd.setRGB(250, 100, 10);
					break;

				case 1:
					Serial.println(F("-> GPS Data stored in SD log!"));
					last_log = millis();
					break;

				case 2:
					Serial.println(F("-> No Invalid GPS Data..."));
					lcd.setRGB(250, 50, 10);
					break;

				default:
					Serial.println(F("-> SD Card Loading Error..."));

			}
		}
		else if (sd_status == false and gps.satellites.value() >= 3)
		{
			delay(50);
		}
		else
		{
			Serial.print(F("-> Waiting for better GPS data, Current Satellite Number: "));
			Serial.println(gps.satellites.value());

			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Satellite Number");
			lcd.setCursor(7, 1);
			lcd.print(gps.satellites.value());

			delay(50);
		}
	}

#ifdef Arduino_MEGA
	while (Serial1.available())
	{
		gps.encode(Serial1.read());
	}
#endif

#ifdef Arduino_UNO
	while (ss.available())
	{
		gps.encode(ss.read());
	}
#endif

}

int logGPSData()
{
	File logFile = SD.open(file_name, FILE_WRITE); // Open the log file

	if (logFile)
	{ // Print longitude, latitude, altitude (in feet), speed (in mph), course
	// in (degrees), date, time, and number of satellites.
		if (gps.location.isValid())
		{
			logFile.print(gps.location.lat(), 6);
			logFile.print(',');
			logFile.print(gps.location.lng(), 6);
			logFile.println();
			logFile.close();
			return 1; // Return success
		}
		else
		{
			return 2;
		}

	}
	else
	{
		return 0; // If we failed to open the file, return fail
	}
	
}

void start_new_file()
{
	for (int i = 1; i < max_files; i++)
	{
		memset(file_name, 0, strlen(file_name)); 
		sprintf(file_name, "%s%d.%s", log_file, i, file_suffix);
		if ( !SD.exists(file_name))
		{	
			Serial.print(file_name);
			Serial.println(F(" Create Success! "));
			break;
		}
		else if ( i == max_files-1 )
		{
			Serial.println(F("GPS Record Files Has Reach the MAX Number!\nPlease Clear the SD Log!"));

		}
		else
		{	
			File content = SD.open(file_name);

			Serial.print(F("Current GPS Data File: "));
			Serial.print(file_name);
			Serial.print(F(" | Size: "));
			Serial.print(content.size(), DEC);
			Serial.println(F(" Bytes"));
			content.close();
		}
	}
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
	if (!d.isValid())
	{
		Serial.print(F("****/**/**"));
	}
	else
	{
		char sz[32];
		sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
		Serial.print(sz);
  	}
  
	if (!t.isValid())
	{
		Serial.print(F("**:**:**"));
	}
	else
	{
		char sz[32];
		sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
		Serial.print(sz);
	}
}

static void printInt(unsigned long val, bool valid, int len)
{
	char sz[32] = "*****************";
	if (valid)
		sprintf(sz, "%ld", val);
	sz[len] = 0;
	for (int i=strlen(sz); i<len; ++i)
		sz[i] = ' ';
	if (len > 0) 
		sz[len-1] = ' ';
	Serial.print(sz);
}

static void printFloat(float val, bool valid, int len, int prec)
{
	if (!valid)
	{
		while (len-- > 1)
			Serial.print('*');
		Serial.print(' ');
	}
	else
	{
		Serial.print(val, prec);
		int vi = abs((int)val);
		int flen = prec + (val < 0.0 ? 2 : 1); // . and -
		flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
		for (int i=flen; i<len; ++i)
			Serial.print(' ');
	}
}
