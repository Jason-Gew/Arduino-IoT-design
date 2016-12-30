/*****************************************************************************
 *  GPS Data Logger
 * 
 * 
 * 
 ****************************************************************************/
//#include <SoftwareSerial.h>	# Use Software Serial when no additional Serial Port
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>

/************ Definitions ************/
//static const int SRX = 6;
//static const int STX = 7;

static const int GPS_BAUD = 9600;
static const int sd_select = 4;

#define log_file "GPS.csv"
#define log_rate 5000
char file_name[14];
// If column number or name is changed, please also change corresponding settings in each function.
char * column_names[9] = {"Date", "Time", "Latitude", "Longitude", "Fix", "Satellite", "Altitude", "Speed", "Course"}
unsigned long last_log = 0;
bool sd_status = false;

/*************************************/
// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
//SoftwareSerial ss(SRX, STX);




void setup() 
{
	Serial.begin(9600);
	Serial1.begin(GPS_BAUD);

	Serial.println(F("-> Setting Up SD Card..."));
	if (!SD.begin(sd_select))
	{
		Serial.println(F("-> Initializing SD Card Failed!"));
		sd_status = false;		
	}
	else
	{
		Serial.println(F("-> SD Card Initialized!"));
		sd_status = true;
	}
	if (sd_status)
		printHeader();

	Serial.println(F("Date      Time    Latitude   Longitude   Fix    Satellites      Alt    Course   Speed"));
	Serial.println(F("                    (deg)      (deg)     Age      Number        (m)                  "));
	Serial.println(F("-------------------------------------------------------------------------------------"));
}

void loop() 
{
	if ((last_log + log_rate) <= millis())
	{
		if (tinyGPS.location.isUpdated())
		{
			store = logGPSData();
			switch (store)
			{
				case 0:
					Serial.println(F("-> Failed to store GPS data to log..."));
					break;

				case 1:
					Serial.println(F("-> GPS Data stored in SD log!"));
					last_log = millis();
					break;

				default:
					Serial.println(F("-> SD Card Loading Error..."));

			}
		}
		else
		{
			Serial.print(F("-> No GPS Data, Current Satellite Number: "));
			Serial.println(F(gps.satellites.value()));
		}
	}
	while (Serial1.available())
	{
		gps.encode(Serial1.read());
	}

}

int logGPSData()
{
	File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file

	if (logFile)
	{ // Print longitude, latitude, altitude (in feet), speed (in mph), course
	// in (degrees), date, time, and number of satellites.
		logFile.print(gps.date.value());
		logFile.print(',');
		logFile.print(gps.time.value());
		logFile.print(',');
		logFile.print(gps.location.lat(), 6);
		logFile.print(',');
		logFile.print(gps.location.lng(), 6);
		logFile.print(',');
		logFile.print(gps.location.age());
		logFile.print(',');
		logFile.print(gps.satellites.value());
		logFile.print(',');
		logFile.print(gps.altitude.meters(), 1);
		logFile.print(',');
		logFile.print(gps.speed.mph(), 1);
		logFile.print(',');
		logFile.print(gps.course.deg(), 1);
		logFile.println();
		logFile.close();

		return 1; // Return success
	}

	return 0; // If we failed to open the file, return fail
}

bool printHeader()
{
	File logFile = SD.open(log_file, FILE_WRITE);
	if (logFile)
	{
		for(int i = 0; i < 9; i++)
		{
			logFile.print(column_names[i]);
			if (i < 8)
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

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
	if (!d.isValid())
	{
		Serial.print(F("********** "));
	}
	else
	{
		char sz[32];
		sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
		Serial.print(sz);
  	}
  
	if (!t.isValid())
	{
		Serial.print(F("******** "));
	}
	else
	{
		har sz[32];
		sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
		Serial.print(sz);
	}

	printInt(d.age(), d.isValid(), 5);
	smartDelay(0);
}