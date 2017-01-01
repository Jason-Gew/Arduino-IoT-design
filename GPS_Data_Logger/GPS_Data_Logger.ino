/*****************************************************************************
 *  GPS Data Logger for UART GPS Module + Arduino Mega + SD Card
 *  Sketch requires TInyGPS++ Library 
 *  By Jason-Gew 
 *  On Dec/30/2016
 ****************************************************************************/
//#include <SoftwareSerial.h>	# Use Software Serial when no additional Serial Port
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>

/************ Definitions ************/
//static const int SRX = 6;
//static const int STX = 7;

static const int GPS_BAUD = 9600;
static const int sd_select = 4; // SPI CS Pin

#define log_file "gpslog"
#define file_suffix "csv"
#define max_files 199		// Max Record File Number
#define log_rate 30000		// ms/1000 = second
char file_name[14];			// Limit the Length of File Name

// If column number or name is changed, please also change corresponding settings in each function.
const int parameter_num = 6;
char * column_names[parameter_num] = {"Latitude", "Longitude", "UTC Date", "UTC Time", "Altitude", "Speed"};
unsigned long last_log = 0;
bool sd_status = false;

/*************************************/
// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
//SoftwareSerial ss(SRX, STX);

/************** Functions **************/
void setup() 
{
	Serial.begin(9600);
	Serial1.begin(GPS_BAUD);

	Serial.println(F("\n-> GPS SD Log System Initializing..."));
	if (!SD.begin(sd_select))
	{
		Serial.println(F("-> Initializing SD Card Failed!"));
		Serial.println(F("***** Please check the SD Card and Restart the Program *****"));
		sd_status = false;		
	}
	else
	{
		Serial.println(F("-> SD Card Initialized!\n"));
		sd_status = true;
	}
	if (sd_status)
	{
		start_new_file();
		printHeader();
	}

	Serial.println(F("-> Waiting for GPS Signal ..."));
	Serial.println(F("---------------------------------------------------------------------"));
	Serial.println(F("   Date      Time     Latitude   Longitude   Fix  Sats   Alt   Speed"));
	Serial.println(F("(MM/DD/Year) (24)      (deg)       (deg)     Age  (No)   (m)   (km/h)"));
	Serial.println(F("---------------------------------------------------------------------"));
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

			int store = logGPSData();
			switch (store)
			{
				case 0:
					Serial.println(F("-> Failed to store GPS data to SD log..."));
					break;

				case 1:
					Serial.println(F("-> GPS Data stored in SD log!"));
					last_log = millis();
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
			delay(50);
		}
	}
	while (Serial1.available())
	{
		gps.encode(Serial1.read());
	}

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
			logFile.print(',');
		}
		else
		{
			logFile.print(",,");
		}
		char date[10];
		if (gps.date.isValid())
		{
			sprintf(date, "%02d/%02d/%02d", gps.date.month(), gps.date.day(),gps.date.year());
		}
		else
		{
			sprintf(date, "");
		}
		logFile.print(date);
		logFile.print(',');
		char time[8];
		if (gps.time.isValid())
		{
			sprintf(time, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());	
		}
		else 
		{
			sprintf(time, "");
		}
		logFile.print(time);
		logFile.print(',');
		if (gps.altitude.isValid())
		{
			logFile.print(gps.altitude.meters(), 1);
		}
		else
		{
			logFile.print(0, 1);
		}
		logFile.print(',');
		logFile.print(gps.speed.kmph(), 1);
		logFile.println();
		logFile.close();

		return 1; // Return success
	}
	else
	{
		return 0; // If we failed to open the file, return fail
	}
	
}

bool printHeader()
{
	File logFile = SD.open(file_name, FILE_WRITE);
	if (logFile)
	{
		for(int i = 0; i < parameter_num; i++)
		{
			logFile.print(column_names[i]);
			if (i < parameter_num - 1)
			{
				logFile.print(',');
			}
			else
			{
				logFile.println();
			}
		}
		logFile.close();
		return true;
	}
	else
	{
		return false;
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
			Serial.println(F(" create success."));
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
		Serial.print(F("****/**/** "));
	}
	else
	{
		char sz[32];
		sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
		Serial.print(sz);
  	}
  
	if (!t.isValid())
	{
		Serial.print(F("**:**:** "));
	}
	else
	{
		char sz[32];
		sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
		Serial.print(sz);
	}

//	smartDelay(0);
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
