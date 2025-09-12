
#include <WiFi.h>       // standard library 
#include <WebServer.h>  // standard library
#include "html-webpage.h"   // .h file that stores your html page code

#include "watchdogs.h"
#include "common.h"


#define PIN_LED 2

bool LED0 = false, SomeOutput = false;

uint32_t lastSensorTime = 0;

int sliderPosition = 0;
int textToWeb = 0;

// the XML array size needs to be bigger that your maximum expected size. 2048 is way too big for this example
char XML[2048];

// just some buffer holder for char operations
char xml_tbuf[320];

// variable for the IP reported when you connect to your homes intranet (during debug mode)
IPAddress webPageIp;

// gotta create a server
WebServer server(80);


uint32_t oldTime = 0;

extern void vPrintTaskStats( void *pvParameters );

void web_loop() 
{


  // you main loop that measures, processes, runs code, etc.
  // note that handling the "on" strings from the web page are NOT in the loop
  // that processing is in individual functions all managed by the wifi lib

  // in my example here every 50 ms, i measure some analog sensor data (my finger dragging over the pins
  // and process accordingly
  // analog input can be from temperature sensors, light sensors, digital pin sensors, etc.


  // no matter what you must call this handleClient repeatidly--otherwise the web page
  // will not get instructions to do something
  server.handleClient();
  	
}

//-----------------------------------------------------

// function managed by an .on method to handle slider actions on the web page

// this example will get the passed string called VALUE and conver to a pwm value
// and control the fan speed

void UpdateSlider() 
{

  // man I hate strings, but wifi lib uses them...
  String t_state = server.arg("VALUE");

  // conver the string sent from the web page to an int
  sliderPosition = t_state.toInt();
  
  Serial.print("UpdateSlider"); Serial.println(sliderPosition);

  //setBrightness(sliderPosition);

  // YOU MUST SEND SOMETHING BACK TO THE WEB PAGE--BASICALLY TO KEEP IT LIVE

  // option 1: send no information back, but at least keep the page live
  // just send nothing back
  // server.send(200, "text/plain", ""); //Send web page

  // option 2: send something back immediately, maybe a pass/fail indication, maybe a measured value
  // here is how you send data back immediately and NOT through the general XML page update code

  // my simple example guesses at fan speed--ideally measure it and send back real data
  // i avoid strings at all caost, hence all the code to start with "" in the buffer and build a
  // simple piece of data

  // slider goes from 0 - 255, rpm goes from 0 - 2400
  
  textToWeb = map(sliderPosition, 0, 255, 0, 255);
  
  // disable but keep old pwm mapping
  //textToWeb = map(sliderPosition, 0, 255, 0, 2400);
  
  strcpy(xml_tbuf, "");
  sprintf(xml_tbuf, "%d", textToWeb);
  
  // now send rpm  back to webpage for display
  server.send(200, "text/plain", xml_tbuf); //Send web page

}

// now process LED on/off press from the web site. Typical applications are the used on the web client can
// turn on / off a light, a fan, disable something etc

void UserPressLEDbutton() 
{

  LED0 = !LED0;
  
  digitalWrite(PIN_LED, LED0);
  Serial.print("Button 0 "); Serial.println(LED0);
  
  // regardless if you want to send stuff back to client or not
  // you must have the send line--as it keeps the page running
  // if you don't want feedback from the MCU--or let the XML manage
  // sending feeback

  // option 1 -- keep page live but dont send any thing
  // here i don't need to send and immediate status, any status
  // like the illumination status will be send in the main XML page update
  // code
  server.send(200, "text/plain", ""); //Send web page

  // option 2 -- keep page live AND send a status
  // if you want to send feed back immediataly
  // note you must have reading code in the java script
  /*
    if (LED0) {
    server.send(200, "text/plain", "1"); //Send web page
    }
    else {
    server.send(200, "text/plain", "0"); //Send web page
    }
  */

}


// same notion for processing button_1
void UserPressClearNV() 
{
  TRACE ("do something \n");
  // not coming back. Going to do a reset
}


// code to send the main web page
// PAGE_MAIN is a large char defined in html-webpage.h
//
// first time send of webpage to remote pc.

void SendFullWebpage() {

  Serial.println("sending web page");

  server.send(200, "text/html", PAGE_MAIN);

}


// THIS IS WHERE TAGS are SENT.
// TAGS ARE DEFINED the "html file" in a function "response()";
//
// Tags in this project are things like BATVOLTAGE1, BATCURRENT2
//
// things like u1,b0, etc are not tags. They seem to be position placeholders
// for phsyical position in a 'excel' formatted table. That has
// nothing to do with tags.

#include "../include/s3km1110.h"

void UpdatePartsOfWebPage() 
{

  //------------------------
  // send rates are all over the place. Lets sync them
  // up to a one second rate
  
  static uint32_t last_time;
  uint32_t diff;
  uint32_t now = millis();
  
  diff = now - last_time;
  
  //Serial.printf("sending xml at %d\n", diff);

  if (diff < 1000) delay(1001 - diff); //send every 1sec
  last_time = millis();
  
  //------------------------
  int i;
  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");

  for (i = 0; i < S3KM1110_DISTANCE_GATE_COUNT; i++)
  {
  	sprintf(xml_tbuf, "<ENERGY%d>%d</ENERGY%d>\n", i+1, distanceGateEnergy[i], i+1);
  	strcat(XML, xml_tbuf);
  }
  
  // send USB voltage
  sprintf(xml_tbuf, "<USBVOLTAGE1>%5.2fv</USBVOLTAGE1>\n", 1.1);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<USBVOLTAGE2>%5.2fv</USBVOLTAGE2>\n", 2.2);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<USBVOLTAGE3>%5.2fv</USBVOLTAGE3>\n", 3.3);
  strcat(XML, xml_tbuf);

  
  sprintf(xml_tbuf, "<BATT_LO>%5.2fv</BATT_LO>\n", 4.4);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<BATT_NO>%5.2fv</BATT_NO>\n", 5.5);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<BATT_HI>%5.2fv</BATT_HI>\n", 6.6);
  strcat(XML, xml_tbuf);


  sprintf(xml_tbuf, "<BATCURRENT1>%d</BATCURRENT1>\n", 100);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<BATCURRENT2>%s</BATCURRENT2>\n", "HIMOM");
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<BATCURRENT3>%d%% </BATCURRENT3>\n", 33);
  strcat(XML, xml_tbuf);

  int32_t val =33;
  char *msg = "testing";
  
  sprintf(xml_tbuf, "<UDTIME1>%s</UDTIME1>\n", msg);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<NOTIME>%s</NOTIME>\n", msg);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<UDTIME2>%s</UDTIME2>\n", msg);
  strcat(XML, xml_tbuf);

  // design point 2
  // this is where element tag 'REBOOTSx' is moved across the wire
  sprintf(xml_tbuf, "<REBOOTS1>%d</REBOOTS1>\n", val );
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<REBOOTS2>%d</REBOOTS2>\n", val);
  strcat(XML, xml_tbuf);

  sprintf(xml_tbuf, "<REBOOTS3>%d</REBOOTS3>\n", val);
  strcat(XML, xml_tbuf);


  // show led0 status
  if (LED0) {
    strcat(XML, "<LED>1</LED>\n");
  }
  else {
    strcat(XML, "<LED>0</LED>\n");
  }

  if (SomeOutput) {
    strcat(XML, "<SWITCH>1</SWITCH>\n");
  }
  else {
    strcat(XML, "<SWITCH>0</SWITCH>\n");
  }

  strcat(XML, "</Data>\n");

  // wanna see what the XML code looks like?
  // actually print it to the serial monitor and use some text editor to get the size
  // then pad and adjust char XML[2048]; above

  #if 0 // dwade no print xml
  Serial.println(XML);
  #endif
  
  // you may have to play with this value, big pages need more porcessing time, and hence
  // a longer timeout that 200 ms

  server.send(500, "text/xml", XML);


}

// I think I got this code from the wifi example
void printWifiStatus() {

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  webPageIp = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(webPageIp);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  
  // print where to go in a browser:
  Serial.print("Open http://");
  Serial.println(webPageIp);
}




void web_setup() {

  Serial.printf("BUILT ON %s %s *** ESP-IDF VER = %s ***\n", __DATE__, __TIME__, esp_get_idf_version());                                                                          

  // turn off led
  LED0 = false;
  digitalWrite(PIN_LED, LED0);

  Serial.println("starting server");
  printWifiStatus();


  // these calls will handle data coming back from your web page
  // this one is a page request, upon ESP getting / string the web page will be sent
  server.on("/", SendFullWebpage);

  // upon esp getting /XML string, 
  // ESP will build and send the XML, 
  // this is how we refresh
  // just parts of the web page
  
  server.on("/xml", UpdatePartsOfWebPage);

  // upon ESP getting /UPDATE_SLIDER string,
  // ESP will execute the UpdateSlider function
  // same notion for the following .on calls
  // add as many as you need to process incoming strings from your web page
  // as you can imagine you will need to code some javascript in your web page to send such strings
  // this process will be documented in the html-webpage.h web page code
  
  server.on("/UPDATE_SLIDER", UpdateSlider);
  server.on("/BUTTON_0", UserPressLEDbutton);
  server.on("/BUTTON_1", UserPressClearNV);

  // finally begin the server
  server.begin();

  oldTime = millis();

}

