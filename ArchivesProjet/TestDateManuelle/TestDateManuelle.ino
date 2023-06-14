//truc déjà----
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "time.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
int utc_delay;
time_t currentTime;
String Argument_Name;
int currentHour,currentMinute,currentSecond,currentYear,currentMonth,currentDay;

WiFiClient client;
ESP8266WebServer server(80);
//truc déjà fin----

#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  rtc.begin();

  //truc déjà--------------------
  timeClient.begin();
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
  //truc déjà fin-------------------
  
  server.on("/", HandleClient);
}

void loop() {

  server.handleClient();

  //Get current hour
  UpdateHour();

  if (Serial.available()) {
    String dateTimeInput = Serial.readStringUntil('\n'); // Lire la chaîne de caractères saisie
    
    // Analyser la chaîne de caractères pour extraire les valeurs de la date et de l'heure
    int day, month, year, hour, minute, second;
    sscanf(dateTimeInput.c_str(), "%d,%d,%d,%d,%d,%d", &day, &month, &year, &hour, &minute, &second);
    
    // Mettre à jour la date et l'heure dans le RTC
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    
    // Afficher la nouvelle date et heure
    DateTime now = rtc.now();
    Serial.print("Nouvelle date et heure: ");
    Serial.print(now.day());
    Serial.print("/");
    Serial.print(now.month());
    Serial.print("/");
    Serial.print(now.year());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
  }
  
  // Afficher l'heure actuelle en temps réel
  DateTime now = rtc.now();
  Serial.print("Heure actuelle: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
  delay(1000); // Attendre une seconde
}


void HandleClient() {


  if (WiFi.status() == WL_CONNECTED) {
    struct tm *timeinfo;
    timeinfo = localtime(&currentTime);
    currentHour = timeClient.getHours();
    currentMinute = timeClient.getMinutes();
    currentSecond = timeClient.getSeconds();
    currentDay = timeinfo->tm_mday;
    currentMonth = timeinfo->tm_mon + 1;
    currentYear = timeinfo->tm_year + 1900;
  } else {
    if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      //Serial.print(server.argName(i)); // Display the argument
      Argument_Name = server.argName(i);
      //test
      if (server.argName(i) == "setDate") {
        Serial.println("Date & Hour are set");
        currentHour=server.arg("hour").toInt();
        currentMinute=server.arg("minute").toInt();
        currentSecond=1;
        currentYear=server.arg("year").toInt();
        currentMonth=server.arg("month").toInt();
        currentDay=server.arg("day").toInt();
      }
    }
  }
  }
  rtc.adjust(DateTime(currentYear, currentMonth, currentDay, currentHour, currentMinute, currentSecond));
  

  String webpage;
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
                  webpage +=  "c1{display:flex;margin:2% 2% 2% 2%;display: inline-block;}";
                  webpage +=  "c2{display:flex;margin:2% 2% 2% 2%;display: inline-block;}";
                  webpage +=  "e1{display:flex;justify-content: space-between;margin:2% 2% 2% 2%;}";
                  webpage +=  "txttitle{color:#141b6b;}";
                  webpage +=  "subtitle{font-size: 80%;margin:1% 1% 1% 1%;color:#6976ff;}";
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
                      webpage += "<subtitle>If you don't have an internet connection, select the time and date manually</subtitle>";
                        webpage += "<form action='http://"+IPaddress+"' method='POST'>";
                            webpage += "<input type=\"number\" placeholder=\"---Day---\" value=\"";
                            webpage += (currentDay != 0) ? String(currentDay) : "";
                            webpage += "\" name=\"day\" min=\"1\" max=\"31\">";
                            webpage +=  " ";
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
                                webpage +=  " ";
                                webpage +=  "<input type=\"number\" placeholder=\"---Year---\" value=\"";
                                webpage += (currentYear != 0) ? String(currentYear) : "";
                                webpage += "\"name=\"year\"min=\"2022\" max=\"2500\" >";
                                webpage +=  " | ";
                                webpage +=  "<input type=\"number\" placeholder=\"Hour\" value=\"";
                                webpage += (currentHour != 0) ? String(currentHour) : "";
                                webpage += "\"name=\"hour\"min=\"0\" max=\"23\" style=\"width: 60px;\" >";
                                webpage +=  " : ";
                                webpage +=  "<input type=\"number\" placeholder=\"Min\" value=\"";
                                webpage += (currentMinute != 0) ? String(currentMinute) : "";
                                webpage += "\"name=\"minute\"min=\"0\" max=\"59\" style=\"width: 60px;\" >";
                            webpage +=  "<button class=\"button\" name=\"setDate\" input type='submit'>Set</button>";
                        webpage += "</form>";     
                	    webpage +=  "</d1>";
                    webpage +=  "</h2>";
   webpage += "</body>";
  webpage += "</html>";

  server.send(200, "text/html", webpage); // Send a response to the client asking for input

}

void UpdateHour(){
  timeClient.update();
  time_t hourUTC0 = timeClient.getEpochTime();
  struct tm* timeinfo;
  timeinfo = localtime(&hourUTC0);
  // Modifier les composantes de l'heure
  timeinfo->tm_hour+=utc_delay;
  if (timeinfo->tm_hour > 23) {
    timeinfo->tm_hour %= 24;  // Ramener la valeur à l'intérieur de la plage des heures
  }
  
  currentTime = mktime(timeinfo);
}