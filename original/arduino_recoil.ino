#include <Mouse.h>

bool active = false;

void setup() {
  Serial.begin(9600);
  Mouse.begin();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 's') active = true;
    if (cmd == 'x') active = false;
  }

  if (active) {
    Mouse.move(-10, 12);
    delay(10);
    Mouse.move(10, -10);
    delay(10);
  }
}
