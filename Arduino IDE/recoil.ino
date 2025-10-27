#include <Mouse.h>

unsigned long timer2 = 0, timer4 = 0;
constexpr double TICK_64MS = 15.6;

bool is_timer_elapsed(unsigned long& timer, unsigned long interval) {
  unsigned long current_time = millis();

  if (current_time - timer >= interval) {
    timer = current_time;
    return true;
  }

  return false;
}

bool auto_recoil = false;
bool auto_bhop = false;

void setup() {
  Serial.begin(9600);
  Mouse.begin();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case 'j':
        auto_bhop = true;
        break;
      case 'a':
        auto_bhop = false;
        break;
      case 's':
        auto_recoil = true;
        break;
      case 'x':
        auto_recoil = false;
        break;
    }
  }

  // auto bhop.
  if (auto_bhop) {
    if (is_timer_elapsed(timer2, TICK_64MS * 39)) {
      Mouse.move(0, 0, -1);
    }
    if (is_timer_elapsed(timer4, TICK_64MS * 2)) {
      Mouse.move(0, 0, -1);
    }
  }

  // recoil.
  if (auto_recoil) {
    Mouse.move(0, 6);
    delay(11);  // delay function takes ms as param.
  }
}