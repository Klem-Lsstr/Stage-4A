//////////////////////////////////////////////////////////////////////////////////
///
///   Include - Define - globalVariables
///   Class Button (for the physical button)
///               SimpleClic - DoubleClic - LongClic
///   HTML ( char html_template )
///   Setup (Settings - Start Web Page - Wifi Connection)
///   Loop (Update - Check booleans - sendWebSocket)
///   HandleClient(UpdateHhour - HandleMain)
///   HandleMain (check Input from Webpage & update)
///   Others Fct : webSocketEvent - handleNotFound - handleTrigger (clic btn & input)
///             UpdateHour - delayMilisec - delayMicrosec - BroadcastOnOff
///             ShortOnOff - GroupOnOff - ShortDAPC - GroupDAPC
///             SetMaxLevel - SetPowerOnLevel - SearchAndCompare
///             DaliTransmitCMD - UpdateSunElevation - SetPlaceDayCycle
///             DayCycle
///
//////////////////////////////////////////////////////////////////////////////////

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
int elevationThreshold=5,dayThreshold=10,eveningThreshold=-5;//Threshold for day cycles

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
String dayrunningState = "OFF";
String Argument_Name,current_day,current_month,current_year;

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
        if (dayrunningState=="OFF") {
            dayrunningState="ON";
            dayRunning=true;
          } else {
            dayrunningState="OFF";
            dayRunning=false;
            dayState=false;eveningState=false;nightState=false;
            pDay=0,pEvening=0,pNight=0;
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

ESP8266WiFiMulti    WiFiMulti;
ESP8266WebServer    server(80);
WebSocketsServer    webSocket = WebSocketsServer(81);


char html_template[] PROGMEM = R"=====(

  <!DOCTYPE html> 
   <html> 
    <head> 
      <title>Lamp Controler</title> 
        <style> 
            html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;background-color:#dce7f7;} 
            body {border-width:2px;border:1rem solid;border-color:#141b6b;margin:5% 25% 5% 25%;border-radius: 25px;background-color:#ffffff;}  
            button {background-color: #6976ff; border: none; color: white; padding: 10px 30px;border-width:1px;border:solid;border-color:#141b6b; 
            text-decoration: none; font-size: 20px; margin:2% 2% 2% 2%; cursor: pointer;border-radius: 25px;} 
            .button2 {background-color:#949494;border-width:1px;border:solid;border-color:#141b6b;} 
            .button:hover {color: #141b6b;} 
            .button:active{background-color: #141b6b;} 
            h1{display:grid;background-color:#ffffff;} 
            h2{border-width:1px;border:solid;border-color:#141b6b;z-index: 1;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#dce7f7;} 
            d1{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#ffffff;} 
            d2{display:flex;justify-content: center;} 
            d3{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;border-radius: 5px;display:grid;background-color:#ffffff;width:50%} 
            d4{margin:1% 1% 1% 1%;display:flex;justify-content: center;} 
            d5{border-width:1px;border:solid;border-color:#141b6b;margin:1% 1% 1% 1%;padding:1% 1% 1% 1%;border-radius: 5px;display:flex;background-color:#ffffff;} 
            d6{margin:1% 1% 1% 1%;display:grid;justify-content:center;width:20%;padding:2% 2% 2% 2%;}
            d7{margin:1% 1% 1% 1%;display:grid;justify-content:flex;width:80%;padding:2% 2% 2% 2%;}
            c1{display:flex;margin:2% 2% 2% 2%;display: inline-block;} 
            c2{display:flex;margin:2% 2% 2% 2%;display: inline-block;} 
            e1{display:flex;justify-content: space-between;margin:2% 2% 2% 2%;} 
            txttitle{color:#141b6b;} 
            subtitle{color:#6976ff;}
            txt{color:#141b6b;font-size: 80%;font-family: Helvetica, Arial, sans-serif;white-space: nowrap;}
            text-decoration: none; font-size: 30px; margin: 0px auto; cursor: pointer;border-radius: 25px;} 
            input,label {margin: 0.4rem 0;}
            .slider1,.slider2,.slider3 {width: 90%;height: 15px;opacity: 0.7;}
            .slider1::-moz-range-progress{width: 100%;height: 15px;opacity: 0.7;background: linear-gradient(to right, #0472d9, #7fbffa);border-radius:10px;box-shadow: 2px 2px #141b6b;}
            .slider2::-moz-range-progress{width: 100%;height: 15px;opacity: 0.7;background: linear-gradient(to right, #ff8400, #d69b5c);border-radius:10px;box-shadow: 2px 2px #141b6b;}
            .slider3::-moz-range-progress{width: 100%;height: 15px;opacity: 0.7;background: linear-gradient(to right, #d95700, #ff934a);border-radius:10px;box-shadow: 2px 2px #141b6b;}
            .slider1::-moz-range-thumb,.slider2::-moz-range-thumb,.slider3::-moz-range-thumb {width: 0px;height: 0px;} 
        </style> 
    </head>

    <body>  
        <script>
          socket = new WebSocket("ws:/" + "/" + location.host + ":81");
          socket.onopen = function(e) {  console.log("[socket] socket.onopen "); };
          socket.onerror = function(e) {  console.log("[socket] socket.onerror "); };
          socket.onmessage = function(e) 
          {  
          console.log("[socket] " + e.data);
          var values = e.data.split(";");
          var pDay = parseInt(values[0]);
          var pEvening = parseInt(values[1]);
          var pNight = parseInt(values[2]);
          document.getElementById("pDay").value = pDay;
          document.getElementById("pEvening").value = pEvening;
          document.getElementById("pNight").value = pNight;
          };
        </script>
      <h1> 
        <txttitle>Lamp Controler</txttitle> 
      </h1> 

      <h2> 
          <d1> 
              <form method='post' action='/trigger'>
                  <input type='hidden' name='onoff'>
                  <button class="%BUTTON_CLASS%" type='submit' id="toggleButton">%BUTTON_VALUE%</button>
              </form>          
              <form method='post' action='/trigger'>
                  <input type='hidden' name='add'>
                  <button class="button" type='submit'>+15min</button>
              </form>
              <txt>Between 21:30 and 23:30 you can add evening time using this button</txt>   
          </d1> 
                    
        <form action='/trigger' method='POST'> 
          <d2> 
              <d3> 
                	<subtitle>Time Zone</subtitle> 
                  <c1> 
								      <select name="utc" required> 
                        <option value="">---UTC---</option> 
                        %UTC_OPTIONS%
                      </select>
                  </c1> 
              </d3> 
              <d3> 
                	<subtitle>Location Settings</subtitle> 
                  <c1> 
                      <txt>Latitude</txt> 
								      <input type="number" step=0.0001 placeholder="Latitude" value="%LATITUDE%" name="latitude"min="-90" max="90" required> 
                  </c1> 
                  <c1> 
                      <txt>Longitude</txt> 
								      <input type="number" step=0.0001 placeholder="Longitude" value="%LONGITUDE%" name="longitude"min="-180" max="180" required> 
                  </c1> 
              </d3>
              <input type='hidden' name='setSettings'> 
              <button class="button" name="setSettings" input type='submit'>Set</button> 
          </d2> 
              <d1 style="background-color:#a4acfc;"> 
                <txt>If you don't have an internet connection, select the time and date manually</txt> 
                <d4> 
                    <input type="number" placeholder="---Day---" value="%DAY_VALUE%" name="day" min="1" max="31">

                    <select name="month"><option value="">---Month---</option>%MONTH_OPTIONS%</select>
 
                    <input type="number" placeholder="---Year---" value="%YEAR_VALUE%" name="year" min="2022" max="2500">

                    <input type="number" placeholder="Hour" value="%HOUR_VALUE%" name="hour" min="0" max="23" style="width: 60px;margin: 0px 0px 0px 70px;"> 

                    <input type="number" placeholder="Min" value="%MINUTE_VALUE%" name="minute" min="0" max="59" style="width: 60px;"> 
                </d4>    
              </d1> 
        </form> 
              <d5>
                <d6> 
                  <txt style="color:#0472d9;">Day Light</txt>
                  <txt style="color:#ff8400;">Evening Light</txt>
                  <txt style="color:#d95700;">Night Light</txt>
                </d6> 
                <d7>
                  <input type="range" id="pDay" min="0" max="255" step="1" value="0" class="slider1">
                  <input type="range" id="pEvening" min="0" max="255" step="1" value="0" class="slider2"> 
                  <input type="range" id="pNight" min="0" max="255" step="1" value="0" class="slider3">
                </d7> 
              </d5>
      </h2>
    </body> 
   </html>
)=====";



//-------------------------------------------------------------------------SETUP BEGIN------------------------------------------
void setup() {

  Serial.begin(115200);
  delay(1000);

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

  //Wifi id&password
  WiFiMulti.addAP("C4-460", "bez_dratu460");
  WiFiMulti.addAP("xxx", "xxx");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  timeClient.update();
  Serial.println("Web Interface -> Connexion at :");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", handleMain);
  server.on("/trigger", handleTrigger);
  server.onNotFound(handleNotFound);
  server.begin();

}
//-------------------------------------------------------------------------SETUP END

//-------------------------------------------------------------------------LOOP BEGIN------------------------------------------
void loop() {
  
  webSocket.loop();//update webpage
  server.handleClient();//check interaction with the webpage
  button.handle();
  UpdateSunElevation();//update Sun elevation
  UpdateHour();//Update again the hour

  //check booleans
  unsigned long currentTime = millis();
  if(dayRunning==true){
    if(refreshState==true){SetPlaceDayCycle();}
    if (currentTime - lastExecutionTime >= 1000) {
      lastExecutionTime = currentTime;
      DayCycle();
    }
    refreshState=false;
  }
  else if(dayRunning==false){BroadcastOnOff(OFF);refreshState=true;}

  //send webSocket data
  String msg = String(pDay)+";"+String(pEvening)+";"+String(pNight);
  webSocket.broadcastTXT(msg);
  delay(50);
}
//-------------------------------------------------------------------------LOOP END

void HandleClient() {

  //Get current hour
  UpdateHour();
  handleMain();
}

void handleMain() {

  String button_class = (dayrunningState == "ON") ? "button2" : "button";
  String button_value;
  if(dayrunningState=="ON"){button_value = "OFF";}
  else if (dayrunningState=="OFF"){button_value = "ON";}

  String utc_options;
  for (int i = 0; i <= 23; i++) {
    String option = "<option value='" + String(i) + "'" + (utc_delay == i ? " selected" : "") + ">UTC+" + String(i) + "</option>";
    utc_options += option;
  }

  String month_options;
  for (int i = 1; i <= 12; i++) {
    String month_name;
    switch (i) {
      case 1: month_name = "January"; break;
      case 2: month_name = "February"; break;
      case 3: month_name = "March"; break;
      case 4: month_name = "April"; break;
      case 5: month_name = "May"; break;
      case 6: month_name = "June"; break;
      case 7: month_name = "July"; break;
      case 8: month_name = "August"; break;
      case 9: month_name = "September"; break;
      case 10: month_name = "October"; break;
      case 11: month_name = "November"; break;
      case 12: month_name = "December"; break;
      default: month_name = ""; break;
    }
    String option = "<option value='" + String(i) + "'" + (currentMonth == i ? " selected" : "") + ">" + month_name + "</option>";
    month_options += option;
  }
  String day_value = (currentDay != 0) ? String(currentDay) : "";
  String year_value = (currentYear != 0) ? String(currentYear) : "";
  String hour_value = String(currentHour);
  String minute_value = String(currentMinute);

  String html_string(html_template);
  html_string.replace("%BUTTON_CLASS%", button_class);
  html_string.replace("%BUTTON_VALUE%", button_value);
  html_string.replace("%UTC_OPTIONS%", utc_options);
  html_string.replace("%LATITUDE%", String(Latitude, 4));
  html_string.replace("%LONGITUDE%", String(Longitude, 4));
  html_string.replace("%DAY_VALUE%", day_value);
  html_string.replace("%MONTH_OPTIONS%", month_options);
  html_string.replace("%YEAR_VALUE%", year_value);
  html_string.replace("%HOUR_VALUE%", hour_value);
  html_string.replace("%MINUTE_VALUE%", minute_value);
  server.send(200, "text/html", html_string.c_str()); 
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

void handleNotFound() {
  server.send(404,   "text/html", "<html><body><p>404 Error</p></body></html>" );
}

void handleTrigger() {
  
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
        if (dayrunningState=="OFF") {
            dayrunningState="ON";
            dayRunning=true;
          } else {
            dayrunningState="OFF";
            dayRunning=false;
            dayState=false;eveningState=false;nightState=false;
            pDay=0,pEvening=0,pNight=0;
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
  server.sendHeader("Location", "/");
  server.send(302);
}

void UpdateHour(){

  //hour with or without wifi
  if (WiFi.status() == WL_CONNECTED) //-------------With wifi -> timeClient.get
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
      if(sunElevation<dayThreshold)//====================================Night
      {
        dayState=false;
      }
      else if(sunElevation>=dayThreshold)//====================================Day
      {
        dayState=true;
      }
    } 
    if((currentHour>=18&&currentHour<21)||(currentHour==21&&currentMinute<45))//-----18:00 / 21:45
    {
      if(sunElevation>eveningThreshold)//====================================Day
      {
        eveningState=false;
      }
      if(sunElevation<eveningThreshold)//====================================Evening
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
          if(sunElevation<elevationThreshold)//====================================Night
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
          if(sunElevation>=elevationThreshold)
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
          if(sunElevation>elevationThreshold)
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
          if(sunElevation<=elevationThreshold)
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