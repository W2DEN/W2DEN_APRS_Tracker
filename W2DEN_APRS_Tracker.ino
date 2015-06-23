// Example program for the APRS code in this repository

/*
* This is W2DEN's attempt at a Teensy3.1 APRS Tracker.
* The bases for this sketch comes from Richard Nash (KC3ARY). Without his code I'd still be searching
* Also thanks to:
*
* The APRS libraries come from the Trackduino project with modes by many before it got here.
 *
 *
 */
#define thisver "1.0" ////////////////////////////////// VERSION
#include <WProgram.h>
// Note: this example uses my GPS library for the Adafruit Ultimate GPS
// Code located here: https://github.com/rvnash/ultimate_gps_teensy3
#include <GPS.h>
#include <aprs.h>

// for the display
#include "SPI.h"
#include "ILI9341_t3.h"
ILI9341_t3 tft = ILI9341_t3(10, 9, 8, 11, 14, 12);
static const int line=25; //# of lines on screen @ font size 3

// APRS Information
#define PTT_PIN 13 // Push to talk pin

// Set your callsign and SSID here. Common values for the SSID are
#define S_CALLSIGN      "W2DEN"
#define S_CALLSIGN_ID   5   // 11 is usually for balloons
// Destination callsign: APRS (with SSID=0) is usually okay.
#define D_CALLSIGN      "APRS"
#define D_CALLSIGN_ID   0
// Symbol Table: '/' is primary table '\' is secondary table
#define SYMBOL_TABLE '/'
// Primary Table Symbols: /O=balloon, /-=House, /v=Blue Van, />=Red Car
#define SYMBOL_CHAR '-'
int dTime = 1 * 60 * 1000; //Minutes *  seconds * milliseconds
struct PathAddress addresses[] = {
  {(char *)D_CALLSIGN, D_CALLSIGN_ID},  // Destination callsign
  {(char *)S_CALLSIGN, S_CALLSIGN_ID},  // Source callsign
  {(char *)NULL, 0}, // Digi1 (first digi in the chain)
  {(char *)NULL, 0}  // Digi2 (second digi in the chain)
};


HardwareSerial &gpsSerial = Serial1;
GPS gps(&gpsSerial, true);

// setup() method runs once, when the sketch starts
void setup()
{
  dTime = dTime + 1;
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(0);
  Serial.begin(38400); // For debugging output over the USB port
  gps.startSerial(9600);
  delay(1000);
  gps.setSentencesToReceive(OUTPUT_RMC_GGA);

  // Set up the APRS module
  aprs_setup(50, // number of preamble flags to send
             PTT_PIN, // Use PTT pin
             500, // ms to wait after PTT to transmit
             0, 0 // No VOX ton
            );

  while (!(gps.sentenceAvailable())) {
    delay(1000);
  }
  gps.parseSentence();
  // Serial.print("APRS Initial");
  gps.dataRead();
  broadcastLocation(gps, "Teensy APRS Tracker Testing Nokomis, Fl" );
  display();
  //Serial.printf("Location: %f, %f altitude %f\n\r", gps.latitude, gps.longitude, gps.altitude);
}

// Function to broadcast your location
void broadcastLocation(GPS &gps, const char *comment)
{
  // If above 5000 feet switch to a single hop path
  int nAddresses;
  if (gps.altitude > 1500) {
    // APRS recomendations for > 5000 feet is:
    // Path: WIDE2-1 is acceptable, but no path is preferred.
    nAddresses = 3;
    addresses[2].callsign = "WIDE2";
    addresses[2].ssid = 1;
  } else {
    // Below 1500 meters use a much more generous path (assuming a mobile station)
    // Path is "WIDE1-1,WIDE2-2"
    nAddresses = 4;
    addresses[2].callsign = "WIDE1";
    addresses[2].ssid = 1;
    addresses[3].callsign = "WIDE2";
    addresses[3].ssid = 2;
  }

  // For debugging print out the path
  /*Serial.print("APRS(");
  Serial.print(nAddresses);
  Serial.print("): ");
  for (int i=0; i < nAddresses; i++) {
    Serial.print(addresses[i].callsign);
    Serial.print('-');
    Serial.print(addresses[i].ssid);
    if (i < nAddresses-1)
      Serial.print(',');
  }
  Serial.print(' ');
  Serial.print(SYMBOL_TABLE);
  Serial.print(SYMBOL_CHAR);
  Serial.println();
  */

  // Send the packet
  aprs_send(addresses, nAddresses
            , gps.day, gps.hour, gps.minute
            , gps.latitude, gps.longitude // degrees
            , gps.altitude // meters
            , gps.heading
            , gps.speed
            , SYMBOL_TABLE
            , SYMBOL_CHAR
            , comment);
}

uint32_t timeOfAPRS = 0;
bool gotGPS = false;
// the loop() methor runs over and over again,
// as long as the board has power

void loop()
{
  if (gps.sentenceAvailable()) gps.parseSentence();

  if (gps.newValuesSinceDataRead()) {
    gotGPS = true; // @TODO: Should really check to see if the location data is still valid
    gps.dataRead();
    //Serial.printf("Location: %f, %f altitude %f\n\r", gps.latitude, gps.longitude, gps.altitude);
  }


  if (gotGPS && timeOfAPRS + dTime < millis()) {
    broadcastLocation(gps, "Teensy APRS Tracker Testing Nokomis, Fl" );
    timeOfAPRS = millis();
    display();
  }
}
void display()
{
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setCursor(190, 0);
  tft.print(dTime / 1000 );
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(0, 0);
  char sz[32];
  sprintf(sz, "%02d/%02d/%02d ", gps.month, gps.day, gps.year);
  tft.println(sz);
  //char sz[32];
  sprintf(sz, "%02d:%02d ", gps.hour, gps.minute);
  tft.println(sz);
  
  tft.setTextSize(3);
    tft.setCursor(0, line*3);
    tft.setTextColor(ILI9341_GREEN,ILI9341_BLACK);  
    displayLatLong(gps.latitude);
    
    tft.setTextSize(2);
    if (gps.latitude <0)
    {
      tft.println(" S");
    }
     else
    {
       tft.println(" N"); 
    }
    tft.setTextSize(3);
    tft.setCursor(0, line*5);
    tft.setTextColor(ILI9341_YELLOW,ILI9341_BLACK); 
    displayLatLong(gps.longitude);
    tft.setTextSize(2);
    if (gps.longitude <0)
    {
      tft.println(" W");
    }
     else
    {
       tft.println(" E"); 
    }
    tft.setTextSize(3);
 
  
  tft.setCursor(0, line*7);
  tft.setTextColor(ILI9341_MAGENTA,ILI9341_BLACK);
  tft.setTextSize(4);
  tft.print("          ");
  tft.setCursor(0, line*7);
  printStr(String(gps.speed ),5,false); 
  tft.setTextSize(3);
  printStr(" mph",4,true);
  tft.setCursor(0, line*9);
  tft.setTextSize(4);
  tft.print("          ");
  tft.setTextSize(3);
  tft.setCursor(0, line*9);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  printStr(gps.heading,4,false);
  printStr(" deg",4,true);
   tft.setCursor(0, 280);
  tft.setTextSize(2);
  printStr("Sats:", 6,false);
  printStr(String(gps.satellites), 2,false);
  tft.setCursor(150, 280);
  float x = analogRead(39);
  if (x<=1500) // approximately 3 vdc
  {
    tft.setTextColor(ILI9341_GREEN);
    tft.print("V+ OK");
  }
  else
  {
  tft.setTextColor(ILI9341_RED);
  tft.print("V+ ");
  tft.println(( (178*x*x + 2688757565 - 1184375 * x)  / 372346 )/1000);
  }
  tft.setCursor(170,300);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(1);
  printStr(thisver,6,false); // display version
  
}
static void displayLatLong(float val)
// converts decimal degrees to degrees and decimal minutes (APRS format)
{
  float wlong = val;
  char charVal[10];
  int deglong = wlong; //long;
  wlong -= deglong; // remove the degrees from the calculation
  wlong *= 60; // convert to minutes
  float mlong = wlong;
  String pDegrees = " " + String(abs(deglong));
  //return pDegrees;
  printStr(pDegrees, 4, false);
  tft.setTextSize(2);
  tft.print(char(247));
  tft.setTextSize(3);
  mlong = abs(mlong);
  dtostrf(mlong, 6, 4, charVal);
  pDegrees = "";

  for (unsigned int i = 0; i < sizeof(charVal); i++) // was sizeof
  {
    pDegrees += charVal[i];
  }
  if (mlong < 10)
  {
    printStr(" ", 2, false);
    printStr(pDegrees, 5, false);
  }
  else
  {
    printStr(" ", 1, false);
    printStr(pDegrees, 6, false);
  }
}


static void printStr(String istring, unsigned int len, boolean rtn)
{

  String sout = "";
  unsigned int slen = istring.length();// this how long it is
  istring = istring.trim();
  if (slen > len)
  {
    sout = istring.substring(0, len);
  }
  else
  {
    sout = istring;
    while (sout.length() < len)
    {
      sout = " " + sout;
    }
  }
  if (rtn)
  {
    tft.println(sout);
  }
  else
  {
    tft.print(sout);
  }
}


