/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <ESP8266WiFi.h>

//get hour and date
#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// CVUT network
const char* ssid     = "C4-460";
const char* password = "bez_dratu460";

// Appart network
//const char* ssid     = "Sergioleone";
//const char* password = "Sergioleone";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String outputLEDState = "off";

// Assign output variables for the blue LED
#define LED 2 //Define blinking LED pin

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//--------------------------------------------------------------------------Setup
void setup() {

  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(LED, OUTPUT); // Initialize the LED pin as an output
  // Set outputs to LOW
  digitalWrite(LED, LOW); // Turn the LED on (Note that LOW is the voltage level)
           
  //Hour and date
  WiFi.begin("SSID", "password");
  timeClient.begin();

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

//--------------------------------------------------------------------------Loop
void loop(){

  //Hour and date
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  delay(1000);

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            //-----------Begin----------------------------Actions on the LED
            if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("Led on");
              outputLEDState = "on";
              digitalWrite(LED, LOW); // Turn the LED off by making the voltage HIGH
            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("Led off");
              outputLEDState = "off";
              digitalWrite(LED, HIGH); // Turn the LED on
            } 
            
            else if (header.indexOf("GET /clic") >= 0) {
              Serial.println("SOS Clicked");
              //-----------------------------------------Start SOS séquence---------------
                int t=0;
                for (t=0; t<9; t++)
                  {
                      if (t<3 || t>5&&t<9)//_______________Lettre S ...
                      { digitalWrite(LED, LOW); // Turn the LED on 
                        delay(500); // Wait for 0.5 seconds
                        digitalWrite(LED, HIGH); // Turn the LED off by making the voltage HIGH
                        delay(500); // Wait for 0.5 seconds
                      }
                      else //____________________Lettre O ---
                        { 
                        digitalWrite(LED, LOW); // Turn the LED on 
                        delay(2000); // Wait for 2 seconds
                        digitalWrite(LED, HIGH); // Turn the LED off by making the voltage HIGH
                        delay(500); // Wait for 2 seconds
                         }
                        
                      }
                  
                //-----------------------------------------End SOS séquence---------------
            }


            //-----------End----------------------------------Actions on the LED





            // -----------Begin----------------------------------------------------------Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off Led button 
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4682B4FF; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}");
            // CSS to style the One clic Sos button 
            client.println(".sosbutton { background-color: #4682B4FF; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println("</style>");
            
            

            client.println("</head>");

            // Web Page Heading
            client.println("<body><h1>______________________</h1>");
            client.println("<body><h1>ESP-8266 LED Controler</h1>");
            client.println("<body><h1>_________04/23________</h1>");
            client.println("<body><h1> </h1>");

            
            // Display current state, and ON/OFF buttons for Led  
            client.println("<p>LED state = " + outputLEDState + "</p>");
            // If the outputLEDState is off, it displays the ON button       
            if (outputLEDState=="off") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            //Display SOS Button
            client.println("<p>Send SOS</p>");     
            client.println("<p><a href=\"/clic\"><sosbutton class=\"sosbutton\">...---...</sosbutton></a></p>");
            
            client.println("</body></html>");
            // -----------End----------------------------------------------------------Display the HTML web page
            
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
