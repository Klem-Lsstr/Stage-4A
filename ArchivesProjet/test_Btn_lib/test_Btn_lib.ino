#include <Arduino.h>
#include <OneButton.h>

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

  void Clicked(){
    Serial.println("Simple Click        tac");
  }

  void DoubleClicked(){

    Serial.println("Double Click        tac tac");
  }

  void LongPressed(){
    Serial.println("Long Click          taaaaaaac");
  }

  void handle(){
    button.tick();
  }
};

Button button(0);

void setup() {
  Serial.begin(115200);
}

void loop() {
  button.handle();
}