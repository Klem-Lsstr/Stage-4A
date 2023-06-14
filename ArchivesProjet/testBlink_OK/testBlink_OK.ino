/*
ESP8266 Blink
Blink the blue LED on the ESP8266 module
*/

#define LED 2 //Define blinking LED pin

void setup() {
  pinMode(LED, OUTPUT); // Initialize the LED pin as an output
}


// the loop function runs over and over again forever
//_________________________________________SOS Séquence ...---...
void loop() {
int t=0;
  for (t=0; t<15; t++)
   {
      if (t<3 || t>5&&t<9)//_______________Lettre S ...
      { digitalWrite(LED, LOW); // Turn the LED on (Note that LOW is the voltage level)
        delay(500); // Wait for 0.5 seconds
        digitalWrite(LED, HIGH); // Turn the LED off by making the voltage HIGH
        delay(500); // Wait for 0.5 seconds
      }
      else 
      { 
        if(t>2 && t<6)//____________________Lettre O ---
          { digitalWrite(LED, LOW); // Turn the LED on (Note that LOW is the voltage level)
            delay(2000); // Wait for 2 seconds
            digitalWrite(LED, HIGH); // Turn the LED off by making the voltage HIGH
            delay(2000); // Wait for 2 seconds
          }
        else//___________________________Delay
          {
            delay(1000);
          }
      }
   }
}
