//////////////////////////////////////////////////////////////////////////////////
///
///   Include - Define - globalVariables
///   Class Button (for the physical button)
///               SimpleClic - DoubleClic - LongClic
///   Setup (Settings - Start Web Page - Wifi Connection)
///   Loop (Update - Check booleans)
///   HandleClient(Check arguments - Update - WebPage)
///   Others Fct : UpdateHour - delayMilisec - delayMicrosec - BroadcastOnOff
///             ShortOnOff - GroupOnOff - ShortDAPC - GroupDAPC
///             SetMaxLevel - SetPowerOnLevel - SearchAndCompare
///             DaliTransmitCMD - UpdateSunElevation - SetPlaceDayCycle
///             DayCycle
///
//////////////////////////////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <Hash.h>
 

//physical button
#include <Arduino.h>
#include <OneButton.h>

//hour
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include <RTClib.h>
#include "time.h"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
RTC_DS3231 rtc;
int utc_delay;
time_t currentTime;
int currentYear, currentMonth, currentDay, currentHour, currentMinute, currentSecond;
bool setHourManually=false;

//Sun elevation
#include <SolarPosition.h>
SolarPosition_t savedPosition;// A solar position structure
float Latitude;
float Longitude;
int sunElevation = 0;

//Dali ctrl
#define DALI_TX_PIN 14    //D5 --> 14
#define DALI_RX_PIN 16    //D0 --> 16
#define LED_PIN 2         //D4 --> 2
#define ON 1
#define OFF 0
#define BROADCAST_DP 0b11111110
#define BROADCAST_C 0b11111111
#define ON_DP 0b11111110
#define OFF_DP 0b00000000
#define ON_C 0b00000101
#define OFF_C 0b00000000
#define QUERY_STATUS 0b10010000
#define RESET 0b00100000
#define INITIALISE          0xA5
#define RANDOMISE           0xA7
#define SEARCHADDRH         0xB1
#define SEARCHADDRM         0xB3
#define SEARCHADDRL         0xB5
#define PRG_SHORT_ADDR      0xB7
#define COMPARE             0xA9
#define WITHDRAW            0xAB
#define TERMINATE           0xA1
#define START_SHORT_ADDR    0
#define DOWN                0b00000010
#define UP                  0b00000001
#define DALI_HALF_BIT_TIME      416 //microseconds
#define DALI_TWO_PACKET_DELAY   10 //miliseconds
#define DALI_RESPONSE_DELAY_COUNT  15 //maximal number of halfbites
#define DALI_ANALOG_LEVEL   650

int pDay=0,pEvening=0,pNight=0;
int dayLightGroup = 00;
int eveningLight = 01;
int nightLight = 00;
int addDelay=0;//Add
int minuteDelay=0;//for setting new day period
unsigned long lastExecutionTime = 0;//loop 1sec
bool dayState=false, eveningState=false, nightState=false;
bool refreshState = true;
bool dayRunning = false;
bool addTimeForEvening = false;
bool broadcast = false;
String dayrunningState = "off";
String Argument_Name,current_day,current_month,current_year;

WiFiClient client;
ESP8266WebServer server(80);
WebSocketsServer    webSocket = WebSocketsServer(81);
String old_value, value;

class Button{
private:
  OneButton button;
  int value;
public:
  explicit Button(uint8_t pin):button(pin) {
    button.attachClick([](void *scope) { ((Button *) scope)->Clicked();}, this);
    button.attachDoubleClick([](void *scope) { ((Button *) scope)->DoubleClicked();}, this);
    button.attachLongPressStart([](void *scope) { ((Button *) scope)->LongPressed();}, this);
  }

  void Clicked(){ // DayCycle
    Serial.print("DayCycle : ");
        if (dayrunningState=="off") {
            dayrunningState="on";
            dayRunning=true;
          } else {
            dayrunningState="off";
            dayRunning=false;
            dayState=false;
            eveningState=false;
            nightState=false;
            addDelay=0; //to stop setNight
          }
        Serial.println(dayrunningState);
  }

  void DoubleClicked(){
    Serial.println("DoubleClick : Not assigned");
  }

  void LongPressed(){ //If (21:45<hour<23:30) --> Add 15min before SetNight

        if((currentHour<21) || (currentHour==21 && currentMinute<30)){Serial.println(" Too early ! ");}
        else if(currentHour==23&&currentMinute>=30){Serial.println(" Too late ! ");}
        else{//add 15 min
              addTimeForEvening = true;
              nightState = false; //leave night mode
              Serial.println(" add 15min ");
              addDelay=0;
              minuteDelay=0;
            }
  }

  void handle(){
    button.tick();
  }
};

Button button(0);
String webpage;

//------------------------Setup-------------------Begin
void setup() {
  Serial.begin(115200);
  //hour
  timeClient.begin();
  Wire.begin();
  rtc.begin();

  //Dali settings
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(DALI_TX_PIN, OUTPUT);
  digitalWrite(DALI_TX_PIN, HIGH);
  pinMode(DALI_RX_PIN, INPUT);
  SetPowerOnLevel(0);
  SetMaxLevel(255);

  //WiFiManager intialisation
  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect("Connect the Lamp")) {
    Serial.println("failed to connect and timeout occurred");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  timeClient.update();
  Serial.println("WiFi connected..");
  server.begin(); Serial.println("Webserver started..."); // Start the webserver
  Serial.print("Use this URL to connect: http://");// Print the IP address
  Serial.print(WiFi.localIP());Serial.println("/");
  // NOTE: You must use the IP address assigned to YOUR Board when printed/displayed here on your serial port
  // that's the address you must use, not the one I used !
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  // Next define what the server should do when a client connects
  server.on("/", HandleClient); 
}
//------------------------Setup-------------------End

//------------------------Loop-------------------Begin
void loop() {

  webSocket.loop();
  server.handleClient();
  button.handle();

  //Sun elevation
  UpdateSunElevation();
  //Update again the hour
  UpdateHour();

  //check booleans
  unsigned long currentTime = millis();
  if(dayRunning==true){
    if(refreshState==true){SetPlaceDayCycle();}
    if (currentTime - lastExecutionTime >= 1000) {
      lastExecutionTime = currentTime;
      DayCycle();
      server.send(200, "text/html", webpage);
    }
    refreshState=false;
  }
  else if(dayRunning==false){BroadcastOnOff(OFF);refreshState=true;}

  value = String(pDay);
  webSocket.broadcastTXT(value);
}
//------------------------Loop-------------------End

void HandleClient() {

  if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      //Serial.print(server.argName(i)); // Display the argument
      Argument_Name = server.argName(i);
      if (server.argName(i) == "utc") {
        Serial.print("UTC delay = ");
        Serial.println(server.arg(i));
        utc_delay = server.arg(i).toInt();
      }
      if (server.argName(i) == "latitude") {
        Serial.print("latitude = ");
        Serial.println(server.arg(i));
        Latitude = server.arg(i).toFloat();
      }
      if (server.argName(i) == "longitude") {
        Serial.print("longitude = ");
        Serial.println(server.arg(i));
        Longitude = server.arg(i).toFloat();
      }
      if (server.argName(i) == "onoff") {
        Serial.print("DayCycle : ");
        if (dayrunningState=="off") {
            dayrunningState="on";
            dayRunning=true;
          } else {
            dayrunningState="off";
            dayRunning=false;
            addDelay=0; //to stop setNight
          }
        Serial.println(dayrunningState);
      }
      if (server.argName(i) == "add") {
        if((currentHour<21) || (currentHour==21 && currentMinute<30)){Serial.println(" Too early ! ");}
        else if(currentHour==23&&currentMinute>30){Serial.println(" Too late ! ");}
        else{//add 15 min
              addTimeForEvening = true;
              nightState = false; //leave night mode
              Serial.println(" add 15min ");
              addDelay=0;
              minuteDelay=0;
            }
      }
      if (server.argName(i) == "setSettings") {
        setHourManually=true;
      }
    }
  }
  
  //Get current hour
  UpdateHour();

  //String webpage;
  webpage +=  "<!DOCTYPE html>";
  webpage +=  "<html>";
   webpage += "<head>";
   webpage +=  "<title>Lamp Controler</title>";
    webpage +=  "<style>";
                  webpage +=  "html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;background-color:#dce7f7;}";
                  webpage +=  "body {border-width:2px;border:1rem solid;border-color:#141b6b;margin:5% 25% 5% 25%;border-radius: 25px;background-color:#ffffff;} ";
                  webpage +=  "button {background-color: #6976ff; border: none; color: white; padding: 10px 30px;border-width:1px;border:solid;border-color:#141b6b;";
                  webpage +=  "text-decoration: none; font-size: 20px; margin:2% 2% 2% 2%; cursor: pointer;border-radius: 25px;}";
                  webpage +=  ".button2 {background-color:#949494;border-width:1px;border:solid;border-color:#141b6b;}";
                  webpage +=  ".button:hover {color: #141b6b;}";
                  webpage +=  ".button:active{background-color: #141b6b;}";
                  webpage +=  "h1{display:grid;background-color:#ffffff;}";
                  webpage +=  "h2{border-width:1px;border:solid;border-color:#141b6b;z-index: 1;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#dce7f7;}";
                  webpage +=  "d1{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#ffffff;}";
                  webpage +=  "d2{display:flex;justify-content: center;}";
                  webpage +=  "d3{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#ffffff;width:50%}";
                  webpage +=  "d4{margin:1% 1% 1% 1%;display:flex;justify-content: center;}";
                  webpage +=  "c1{display:flex;margin:2% 2% 2% 2%;display: inline-block;}";
                  webpage +=  "c2{display:flex;margin:2% 2% 2% 2%;display: inline-block;}";
                  webpage +=  "e1{display:flex;justify-content: space-between;margin:2% 2% 2% 2%;}";
                  webpage +=  "txttitle{color:#141b6b;}";
                  webpage +=  "subtitle{color:#6976ff;}";
                  webpage +=  "p{margin:25px;}";
                  webpage +=  "text-decoration: none; font-size: 30px; margin: 0px auto; cursor: pointer;border-radius: 25px;}";

                  webpage +=  "input,label {margin: 0.4rem 0;}";
                  webpage +=  "</style>";
   webpage += "</head>";
   webpage += "<body>"; 
    String IPaddress = WiFi.localIP().toString();

    webpage +=  "<h1>";
                      webpage +=  "<txttitle>";
                        webpage +=  "Lamp Controler";
                      webpage +=  "</txttitle>";
                    webpage +=  "</h1>";

                    webpage +=  "<h2>";

                      webpage +=  "<d1>";
                        webpage += "<form action='http://"+IPaddress+"' method='POST'>";
                            if (dayrunningState=="off") {
                                webpage +=  "<button class=\"button\" name=\"onoff\" input type='submit'>ON</button>";
                            } else {
                                webpage +=  "<a href=\"/broadcast/off\"><button name=\"onoff\" class=\"button button2\">OFF</button></a>";
                            }
                            webpage +=  "<button class=\"button\" name=\"add\" input type='submit'>Add</button>";
                        webpage += "</form>";     
                	    webpage +=  "</d1>";
                    
                      webpage += "<form action='http://"+IPaddress+"' method='POST'>";
                      webpage +=  "<d2>";
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Time Zone</subtitle>";
                            webpage +=  "<c1>";
								                webpage +=  "<select name=\"utc\" required>";
                                  webpage +=  "<option value=\"\">---UTC---</option>";
                                  webpage += "<option value=\"0\"" + String(utc_delay == 0 ? " selected" : "") + ">UTC+0</option>";
                                  webpage += "<option value=\"1\"" + String(utc_delay == 1 ? " selected" : "") + ">UTC+1</option>";
                                  webpage += "<option value=\"2\"" + String(utc_delay == 2 ? " selected" : "") + ">UTC+2</option>";
                                  webpage += "<option value=\"3\"" + String(utc_delay == 3 ? " selected" : "") + ">UTC+3</option>";
                                  webpage += "<option value=\"4\"" + String(utc_delay == 4 ? " selected" : "") + ">UTC+4</option>";
                                  webpage += "<option value=\"5\"" + String(utc_delay == 5 ? " selected" : "") + ">UTC+5</option>";
                                  webpage += "<option value=\"6\"" + String(utc_delay == 6 ? " selected" : "") + ">UTC+6</option>";
                                  webpage += "<option value=\"7\"" + String(utc_delay == 7 ? " selected" : "") + ">UTC+7</option>";
                                  webpage += "<option value=\"8\"" + String(utc_delay == 8 ? " selected" : "") + ">UTC+8</option>";
                                  webpage += "<option value=\"9\"" + String(utc_delay == 9 ? " selected" : "") + ">UTC+9</option>";
                                  webpage += "<option value=\"10\"" + String(utc_delay == 10 ? " selected" : "") + ">UTC+10</option>";
                                  webpage += "<option value=\"11\"" + String(utc_delay == 11 ? " selected" : "") + ">UTC+11</option>";
                                  webpage += "<option value=\"12\"" + String(utc_delay == 12 ? " selected" : "") + ">UTC+12</option>";
                                  webpage += "<option value=\"13\"" + String(utc_delay == 13 ? " selected" : "") + ">UTC+13</option>";
                                  webpage += "<option value=\"14\"" + String(utc_delay == 14 ? " selected" : "") + ">UTC+14</option>";
                                  webpage += "<option value=\"15\"" + String(utc_delay == 15 ? " selected" : "") + ">UTC+15</option>";
                                  webpage += "<option value=\"16\"" + String(utc_delay == 16 ? " selected" : "") + ">UTC+16</option>";
                                  webpage += "<option value=\"17\"" + String(utc_delay == 17 ? " selected" : "") + ">UTC+17</option>";
                                  webpage += "<option value=\"18\"" + String(utc_delay == 18 ? " selected" : "") + ">UTC+18</option>";
                                  webpage += "<option value=\"19\"" + String(utc_delay == 19 ? " selected" : "") + ">UTC+19</option>";
                                  webpage += "<option value=\"20\"" + String(utc_delay == 20 ? " selected" : "") + ">UTC+20</option>";
                                  webpage += "<option value=\"21\"" + String(utc_delay == 21 ? " selected" : "") + ">UTC+21</option>";
                                  webpage += "<option value=\"22\"" + String(utc_delay == 22 ? " selected" : "") + ">UTC+22</option>";
                                  webpage += "<option value=\"23\"" + String(utc_delay == 23 ? " selected" : "") + ">UTC+23</option>";
                                  webpage +=  "</select>";
                    	      webpage +=  "</c1>";
                            webpage +=  "<c1>";
                            webpage +=  "</c1>";
                          
                	      webpage +=  "</d3>";
                    
                        webpage +=  "<d3>";
                	        webpage +=  "<subtitle>Location Settings</subtitle>";
                            webpage +=  "<c1>";
                              webpage +=  "<txt>Latitude</txt>";
								              webpage +=  "<input type=\"number\" step=0.0001 placeholder=\"Latitude\" value='" + String(Latitude, 4) + "' name=\"latitude\"min=\"-90\" max=\"90\" required>";
                    	      webpage +=  "</c1>";
                            webpage +=  "<c1>";
                              webpage +=  "<txt>Longitude</txt>";
								              webpage +=  "<input type=\"number\" step=0.0001 placeholder=\"Longitude\" value='" + String(Longitude, 4) + "' name=\"longitude\"min=\"-180\" max=\"180\" required>";
                    	      webpage +=  "</c1>";
                        webpage +=  "</d3>";
                        webpage +=  "<button class=\"button\" name=\"setSettings\" input type='submit'>Set</button>";
                	    webpage +=  "</d2>";
                      webpage +=  "<d1 style=\"background-color:#a4acfc;\">";
                        webpage += "<subtitle style=\"color:#141b6b;\">If you don't have an internet connection, select the time and date manually</subtitle>";
                          webpage +=  "<d4>";
                      
                            webpage += "<input type=\"number\" placeholder=\"---Day---\" value=\"";
                            webpage += (currentDay != 0) ? String(currentDay) : "";
                            webpage += "\" name=\"day\" min=\"1\" max=\"31\">";
                                  webpage +=  "<select name=\"month\" >";
                                  webpage +=  "<option value=\"\">---Month---</option>";
                                  webpage +=  "<option value=\"1\"" + String(currentMonth == 1 ? " selected" : "") + ">January</option>";
                                  webpage +=  "<option value=\"2\"" + String(currentMonth == 2 ? " selected" : "") + ">February</option>";
                                  webpage +=  "<option value=\"3\"" + String(currentMonth == 3 ? " selected" : "") + ">March</option>";
                                  webpage +=  "<option value=\"4\"" + String(currentMonth == 4 ? " selected" : "") + ">April</option>";
                                  webpage +=  "<option value=\"5\"" + String(currentMonth == 5 ? " selected" : "") + ">May</option>";
                                  webpage +=  "<option value=\"6\"" + String(currentMonth == 6 ? " selected" : "") + ">June</option>";
                                  webpage +=  "<option value=\"7\"" + String(currentMonth == 7 ? " selected" : "") + ">July</option>";
                                  webpage +=  "<option value=\"8\"" + String(currentMonth == 8 ? " selected" : "") + ">August</option>";
                                  webpage +=  "<option value=\"9\"" + String(currentMonth == 9 ? " selected" : "") + ">September</option>";
                                  webpage +=  "<option value=\"10\"" + String(currentMonth == 10 ? " selected" : "") + ">October</option>";
                                  webpage +=  "<option value=\"11\"" + String(currentMonth == 11 ? " selected" : "") + ">November</option>";
                                  webpage +=  "<option value=\"12\"" + String(currentMonth == 12 ? " selected" : "") + ">December</option>";
                                  webpage +=  "</select>";
                                webpage +=  "<input type=\"number\" placeholder=\"---Year---\" value=\"";
                                webpage += (currentYear != 0) ? String(currentYear) : "";
                                webpage += "\"name=\"year\"min=\"2022\" max=\"2500\" >";
                                webpage +=  "<input type=\"number\" placeholder=\"Hour\" value=\"";
                                webpage += String(currentHour);
                                webpage += "\"name=\"hour\"min=\"0\" max=\"23\" style=\"width: 60px;margin: 0px 0px 0px 70px;\" >";
                                webpage +=  " : ";
                                webpage +=  "<input type=\"number\" placeholder=\"Min\" value=\"";
                                webpage += String(currentMinute);
                                webpage += "\"name=\"minute\"min=\"0\" max=\"59\" style=\"width: 60px;\" >";
                          webpage +=  "</d4>";   
                	      webpage +=  "</d1>";
                      webpage += "</form>";

        webpage += "<script>";
        webpage += "socket = new WebSocket(\"ws:/\" + \"/\" + location.host + \":81\");";
        webpage += "socket.onopen = function(e) {  console.log(\"[socket] socket.onopen \"); };";
        webpage += "socket.onerror = function(e) {  console.log(\"[socket] socket.onerror \"); };";
        webpage += "socket.onmessage = function(e) {console.log(\"[socket] \" + e.data);";
        webpage += "document.getElementById(\"mrdiy_value\").innerHTML = e.data;};";
        webpage += "</script>";
                      webpage +=  "<d1>";
                          webpage +=  "<p><span id=\"textslider_value\">Power for (Day - Evening - Night) Lights</span></p>";
                          webpage += "<p><input type=\"range\" id=\"mrdiy_value\" min=\"0\" max=\"255\" value=\"";
                          webpage += String(pDay);
                          webpage += "\" step=\"1\" class=\"slider\"></p>";
                          webpage += "<p><input type=\"range\" id=\"powerEvening\" min=\"0\" max=\"255\" value=\"";
                          webpage += String(pEvening);
                          webpage += "\" step=\"1\" class=\"slider\"></p>";
                          webpage += "<p><input type=\"range\" id=\"powerNight\" min=\"0\" max=\"255\" value=\"";
                          webpage += String(pNight);
                          webpage += "\" step=\"1\" class=\"slider\"></p>";
                	    webpage +=  "</d1>";
                    webpage +=  "</h2>";
   webpage += "</body>";
  webpage += "</html>";

  server.send(200, "text/html", webpage); // Send a response to the client asking for input
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        webSocket.sendTXT(num, "0");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      // send message to client
      // webSocket.sendTXT(num, "message here");
      // send data to all connected clients
      // webSocket.broadcastTXT("message here");
      break;
      
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);
      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }

}

void UpdateHour(){

  //hour with or without wifi
  if (false) //-------------With wifi -> timeClient.get
  {
    timeClient.update();
    time_t hourUTC0 = timeClient.getEpochTime();
    struct tm* timeinfo;
    timeinfo = localtime(&hourUTC0);
    // Modifier les composantes de l'heure
    timeinfo->tm_hour+=utc_delay;
    if (timeinfo->tm_hour > 23) {
      timeinfo->tm_hour %= 24; 
    }
    currentTime = mktime(timeinfo);
    timeinfo = localtime(&currentTime);
    currentHour = (utc_delay + timeClient.getHours())%24;
    currentMinute = timeClient.getMinutes();
    currentSecond = timeClient.getSeconds();
    currentDay = timeinfo->tm_mday;
    currentMonth = timeinfo->tm_mon + 1;
    currentYear = timeinfo->tm_year + 1900;
  } 
  else //-------------Without wifi -> rtc.adjust
  {
    if (setHourManually==true) 
    { 
      Serial.println("Date & Hour are set manually");
      currentHour=server.arg("hour").toInt();
      currentMinute=server.arg("minute").toInt();
      currentSecond=30;
      currentYear=server.arg("year").toInt();
      currentMonth=server.arg("month").toInt();
      currentDay=server.arg("day").toInt();
      rtc.adjust(DateTime(currentYear, currentMonth, currentDay, currentHour, currentMinute, currentSecond));
    }
    setHourManually=false;
    DateTime now = rtc.now();
    currentTime = now.unixtime();
    currentYear=(now.year());
    currentMonth=(now.month());
    currentDay=(now.day());
    currentHour=(now.hour());
    currentMinute=(now.minute());
    currentSecond=(now.second());
  }
}

void delayMilisec(unsigned long t) {
  uint32_t s = millis();
  while(millis() - s < t) {
    yield();
  }
  return;
}

void delayMicrosec(unsigned int t) {
  uint32_t s = micros();
  while(micros() - s < t) {
    yield();
  }
  return;
}

void BroadcastOnOff(bool state) {
    if (state) {
        DaliTransmitCMD(BROADCAST_C, ON_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        //Serial.println("Broadcast On");
    } // broadcast, 100%
    else {
        DaliTransmitCMD(BROADCAST_C, OFF_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        //Serial.println("Broadcast Off");
    }; // broadcast, 0%}

}

void ShortOnOff(int shortAddress, bool state) {
    Serial.print("Short ");
    Serial.print(shortAddress);
    if (state) {
        DaliTransmitCMD(((shortAddress << 1) | 0x01), ON_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" on");
    } //short address on
    else {
        DaliTransmitCMD(((shortAddress << 1) | 0x01), OFF_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" off");
    }; //short address off
}

void GroupOnOff(int groupAddress, bool state) {
    Serial.print("Group ");
    Serial.print(groupAddress);
    if (state) {
        DaliTransmitCMD(((groupAddress << 1) | 0x81), ON_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" on");
    } //group address on
    else {
        DaliTransmitCMD(((groupAddress << 1) | 0x81), OFF_C);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.println(" off");
    }; //group address off
 }

void ShortDAPC(int shortAddress, int value) {
    Serial.print("Short ");
    Serial.print(shortAddress);
    DaliTransmitCMD(((shortAddress << 1) | 0x00), value);
    delayMilisec(DALI_TWO_PACKET_DELAY);
    Serial.print(" level is ");
    Serial.println(value);
}; //short address DAPC

void GroupDAPC(int groupAddress, int value) {
    Serial.print("Group ");
    Serial.print(groupAddress);
        DaliTransmitCMD(((groupAddress << 1) | 0x80), value);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        Serial.print(" level is ");
        Serial.println(value);
}; //group address DAPC

void SetMaxLevel(int maxLevel) {
    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, maxLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, maxLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2A); // StoreDTRasMaxLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2A); // StoreDTRasMaxLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);
} // broadcast set max level

void SetPowerOnLevel(int powerOnLevel) {
    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA5, 0x00); // Initialize
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, powerOnLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA3, powerOnLevel); // DataTransferRegister
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2D); // StoreDTRasPowerOnLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(BROADCAST_C, 0x2D); // StoreDTRasPowerOnLevel
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);

    DaliTransmitCMD(0xA1, 0x00); // Terminate
    delayMilisec(DALI_TWO_PACKET_DELAY);
} // broadcast Power On level

bool SearchAndCompare(long SearchAddr)
{
    bool Response = 0;

    uint8_t HighByte = SearchAddr >> 16;
    uint8_t MiddleByte = SearchAddr >> 8;
    uint8_t LowByte = SearchAddr;

    for (uint8_t i = 0; i < 3; i++)
    {
        DaliTransmitCMD(SEARCHADDRH, HighByte);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(SEARCHADDRM, MiddleByte);
        delayMilisec(DALI_TWO_PACKET_DELAY);
        DaliTransmitCMD(SEARCHADDRL, LowByte);
        delayMilisec(DALI_TWO_PACKET_DELAY);
    }
    DaliTransmitCMD(COMPARE, 0x00);
    delayMicrosec(7 * DALI_HALF_BIT_TIME);

    for (uint8_t i = 0; i < DALI_RESPONSE_DELAY_COUNT; i++)
    {
        if (analogRead(DALI_RX_PIN) < DALI_ANALOG_LEVEL)
        {
            Response = 1;
            digitalWrite(LED_PIN, HIGH);
            break;
        }

        delayMicrosec(DALI_HALF_BIT_TIME);
    }

    return Response;
}

void DaliTransmitCMD(uint8_t Part1, uint8_t Part2)
{
    uint8_t DALI_CMD[] = { Part1, Part2 };

    digitalWrite(DALI_TX_PIN, LOW);
    delayMicrosec(DALI_HALF_BIT_TIME);
    digitalWrite(DALI_TX_PIN, HIGH);
    delayMicrosec(DALI_HALF_BIT_TIME);
    
    for (uint8_t CmdPart = 0; CmdPart < 2; CmdPart++)
    {
        for (int i = 7; i >= 0; i--)
        {
            bool BitToSend = false;

            if ((DALI_CMD[CmdPart] >> i) & 1)
                BitToSend = true;

            if (BitToSend)
                digitalWrite(DALI_TX_PIN, LOW);
            else
                digitalWrite(DALI_TX_PIN, HIGH);

            delayMicrosec(DALI_HALF_BIT_TIME);

            if (BitToSend)
                digitalWrite(DALI_TX_PIN, HIGH);
            else
                digitalWrite(DALI_TX_PIN, LOW);

            delayMicrosec(DALI_HALF_BIT_TIME);
        }
    }

    digitalWrite(DALI_TX_PIN, HIGH);
}

void UpdateSunElevation(){
    savedPosition = calculateSolarPosition(currentTime, Latitude * DEG_TO_RAD, Longitude * DEG_TO_RAD);
    sunElevation = (int)(savedPosition.elevation * RAD_TO_DEG);
}

void SetPlaceDayCycle()
{
    if(currentHour>=6&&currentHour<12)//-----------------------------------------06:00 / 12:00
    {
      if(sunElevation<10)//====================================Night
      {
        dayState=false;
      }
      else if(sunElevation>=10)//====================================Day
      {
        dayState=true;
      }
    } 
    if((currentHour>=18&&currentHour<21)||(currentHour==21&&currentMinute<45))//-----18:00 / 21:45
    {
      if(sunElevation>0)//====================================Day
      {
        eveningState=false;
      }
      if(sunElevation<0)//====================================Evening
      {
        eveningState=true;
      }
    }  
    if(currentHour>=22)//----------------------------------------------------------------22:00
    {
      nightState=true;
    }  
}

void DayCycle() {
  
    Serial.print("                             Current hour : ");
    Serial.print(currentHour);Serial.print(":");
    Serial.print(currentMinute);Serial.print(":");
    Serial.print(currentSecond);
    Serial.print("    ");Serial.print("sunElevation : ");Serial.println(sunElevation);

    if(currentHour>=6&&currentHour<12)//-----------------------------------------06:00 / 12:00
      {
        if(dayState==false)//-----already in day ?
        {
          if(sunElevation<5)//====================================Night
          {
            pDay=0;//-------------------------lights power
            pEvening=0;
            pNight=192;
            GroupOnOff(dayLightGroup,OFF);//---lights on/off
            ShortOnOff(eveningLight,OFF);
            //ShortOnOff(nightLight,OFF);
            //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
            //ShortDAPC(eveningLight,pEvening);
            ShortDAPC(nightLight,pNight);
          }
          if(sunElevation>=5)
          {
            minuteDelay++;
            Serial.println("SetDay running");
            Serial.println("");

            if(minuteDelay<=20)//====================================SetDay
            {
              pDay = map(minuteDelay, 0, 20, 0, 210);//-------------------------lights power
              pEvening = 0;
              pNight = map(minuteDelay, 0, 20, 192, 0);
              //GroupOnOff(dayLightGroup,OFF);//---lights on/off
              ShortOnOff(eveningLight,OFF);
              //ShortOnOff(nightLight,OFF);
              GroupDAPC(dayLightGroup,pDay);//----lights DAPC
              //ShortDAPC(eveningLight,pEvening);
              ShortDAPC(nightLight,pNight);
            }
            else if(minuteDelay>20)
            {
              pDay = map(minuteDelay, 21, 300, 210, 255);//-------------------------lights power
              pEvening = 0;
              pNight = 0;
              //GroupOnOff(dayLightGroup,OFF);//---lights on/off
              ShortOnOff(eveningLight,OFF);
              ShortOnOff(nightLight,OFF);
              GroupDAPC(dayLightGroup,pDay);//----lights DAPC
              //ShortDAPC(eveningLight,pEvening);
              //ShortDAPC(nightLight,pNight);
              if(minuteDelay==300)
              {
                minuteDelay=0;
                dayState=true;
              }
            }
          }
        }
        if(dayState==true)//====================================Day
        {
          pDay=255;//-------------------------lights power
          pEvening=0;
          pNight=0;
          //GroupOnOff(dayLightGroup,OFF);//---lights on/off
          ShortOnOff(eveningLight,OFF);
          ShortOnOff(nightLight,OFF);
          GroupDAPC(dayLightGroup,pDay);//----lights DAPC
          //ShortDAPC(eveningLight,pEvening);
          //ShortDAPC(nightLight,pNight);
        } 
      }

    if(currentHour>=12&&currentHour<18)//-----------------------------------------12:00 / 18:00
      {
        dayState=false;//We don't need them for now
        eveningState=false;
        nightState=false;
        pDay=255;//-------------------------lights power//====================================Day
        pEvening=0;
        pNight=0;
        //GroupOnOff(dayLightGroup,OFF);//---lights on/off
        ShortOnOff(eveningLight,OFF);
        ShortOnOff(nightLight,OFF);
        GroupDAPC(dayLightGroup,pDay);//----lights DAPC
        //ShortDAPC(eveningLight,pEvening);
        //ShortDAPC(nightLight,pNight);
      }

    if((currentHour>=18&&currentHour<21)||(currentHour==21&&currentMinute<45))//-----18:00 / 21:45
      {
        if(eveningState==false)//-----already in evening ?
        {
          if(sunElevation>5)
          {
            pDay=255;//-------------------------lights power//====================================Day
            pEvening=0;
            pNight=0;
            //GroupOnOff(dayLightGroup,OFF);//---lights on/off
            ShortOnOff(eveningLight,OFF);
            ShortOnOff(nightLight,OFF);
            GroupDAPC(dayLightGroup,pDay);//----lights DAPC
            //ShortDAPC(eveningLight,pEvening);
            //ShortDAPC(nightLight,pNight);
          }
          if(sunElevation<=5)
          {
            minuteDelay++;
            Serial.println("SetEvening running");
            Serial.println("");

            if(minuteDelay<=900)//====================================SetEvening
            {
              pDay = map(minuteDelay, 0, 900, 255, 210);//-------------------------lights power
              pEvening = map(minuteDelay, 0, 900, 0, 255);
              pNight = 0;
              //GroupOnOff(dayLightGroup,OFF);//---lights on/off
              //ShortOnOff(eveningLight,OFF);
              ShortOnOff(nightLight,OFF);
              GroupDAPC(dayLightGroup,pDay);//----lights DAPC
              ShortDAPC(eveningLight,pEvening);
              //ShortDAPC(nightLight,pNight);
            }
            else if(minuteDelay>900)
            {
              pDay = map(minuteDelay, 901, 1800, 210, 0);//-------------------------lights power
              pEvening = 255;
              pNight = 0;
              //GroupOnOff(dayLightGroup,OFF);//---lights on/off
              //ShortOnOff(eveningLight,OFF);
              ShortOnOff(nightLight,OFF);
              GroupDAPC(dayLightGroup,pDay);//----lights DAPC
              ShortDAPC(eveningLight,pEvening);
              //ShortDAPC(nightLight,pNight);
              if(minuteDelay==1800)
              {
                minuteDelay=0;
                eveningState=true;
              }
            }
          }
        }
        if(eveningState==true)//====================================Evening
        {
          pDay=0;//-------------------------lights power
          pEvening=255;
          pNight=0;
          GroupOnOff(dayLightGroup,OFF);//---lights on/off
          //ShortOnOff(eveningLight,OFF);
          ShortOnOff(nightLight,OFF);
          //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
          ShortDAPC(eveningLight,pEvening);
          //ShortDAPC(nightLight,pNight);
        } 
      }

    if((currentHour==21&&currentMinute>=45)||(currentHour==22)||(currentHour==23&&currentMinute<30))//-----21:45 / 23:30
      {
        if(nightState==false)//-----already in night ?
        {
          if(addTimeForEvening==true)//-----click add=======================Evening
          {
            pDay=0;//-------------------------lights power
            pEvening=255;
            pNight=0;
            GroupOnOff(dayLightGroup,OFF);//---lights on/off
            //ShortOnOff(eveningLight,OFF);
            ShortOnOff(nightLight,OFF);
            //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
            ShortDAPC(eveningLight,pEvening);
            //ShortDAPC(nightLight,pNight);
            addDelay++;
            Serial.print("Click Add Time for Evening since ");
            Serial.print(addDelay);
            Serial.println(" seconds");
            if(addDelay==900)//==============Wait 15min before SetNight
            {
              addTimeForEvening=false;
            }
          }
          if(addTimeForEvening==false)//-----click add
          {
            minuteDelay++;
            Serial.println("SetNight running");
            Serial.println("");

            pDay=0;//-------------------------lights power
            pEvening = map(minuteDelay, 0, 900, 255, 0);
            pNight = map(minuteDelay, 0, 900, 0, 192);
            GroupOnOff(dayLightGroup,OFF);//---lights on/off
            //ShortOnOff(eveningLight,OFF);
            //ShortOnOff(nightLight,OFF);
            //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
            ShortDAPC(eveningLight,pEvening);
            ShortDAPC(nightLight,pNight);
            if(minuteDelay==900)
            {
                minuteDelay=0;
                nightState=true;
            }
          }
        }
        else if(nightState==true)//====================================Night
        {
          if(addTimeForEvening==true)//-----click add=======================Evening
          {
            nightState=false;
          }
          else
          {
            pDay=0;//-------------------------lights power
            pEvening=0;
            pNight=192;
            GroupOnOff(dayLightGroup,OFF);//---lights on/off
            ShortOnOff(eveningLight,OFF);
            //ShortOnOff(nightLight,OFF);
            //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
            //ShortDAPC(eveningLight,pEvening);
            ShortDAPC(nightLight,pNight);
          }
        }
      }

    if(currentHour==23&&currentMinute>=30)//------------------------------------------------23:30 / 00:00
      {
        if(nightState==false)//-----already in night ?
        {
            minuteDelay++;
            Serial.println("SetNight running");
            Serial.println("");

            pDay=0;//-------------------------lights power
            pEvening = map(minuteDelay, 0, 900, 255, 0);
            pNight = map(minuteDelay, 0, 900, 0, 192);
            GroupOnOff(dayLightGroup,OFF);//---lights on/off
            //ShortOnOff(eveningLight,OFF);
            //ShortOnOff(nightLight,OFF);
            //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
            ShortDAPC(eveningLight,pEvening);
            ShortDAPC(nightLight,pNight);
            if(minuteDelay==900)
            {
              minuteDelay=0;
              nightState=true;
            }
        }
        else if(nightState==true)//====================================Night
        {
            pDay=0;//-------------------------lights power
            pEvening=0;
            pNight=192;
            GroupOnOff(dayLightGroup,OFF);//---lights on/off
            ShortOnOff(eveningLight,OFF);
            //ShortOnOff(nightLight,OFF);
            //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
            //ShortDAPC(eveningLight,pEvening);
            ShortDAPC(nightLight,pNight);
        }
      }

    if(currentHour>=0&&currentHour<6)//------------------------------------------------00:00 / 06:00
      {
          nightState=false;//we don't need it anymore===================Night
          eveningState=false;//we don't need it anymore
          pDay=0;//-------------------------lights power
          pEvening=0;
          pNight=192;
          GroupOnOff(dayLightGroup,OFF);//---lights on/off
          ShortOnOff(eveningLight,OFF);
          //ShortOnOff(nightLight,OFF);
          //GroupDAPC(dayLightGroup,pDay);//----lights DAPC
          //ShortDAPC(eveningLight,pEvening);
          ShortDAPC(nightLight,pNight);
       
      }

}