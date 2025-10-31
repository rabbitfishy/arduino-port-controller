#include <Mouse.h>

unsigned long timer1 = 0, timer2 = 0, timer3 = 0;
constexpr double TICK_64MS = 15.6;

bool is_timer_elapsed(unsigned long& timer, unsigned long interval) {
  unsigned long current_time = millis();

  if (current_time - timer >= interval) {
    timer = current_time;
    return true;
  }

  return false;
}

bool auto_trigger = false;
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
        auto_trigger = true;
        break;
      case 'x':
        auto_trigger = false;
        break;
    }
  }

  // auto bhop.
  if (auto_bhop) {
    if (is_timer_elapsed(timer2, TICK_64MS * 39)) {
      Mouse.move(0, 0, -1);
    }
    if (is_timer_elapsed(timer3, TICK_64MS * 2)) {
      Mouse.move(0, 0, -1);
    }
  }

  // trigger.
  if (auto_trigger) {
    if (is_timer_elapsed(timer1, 300)) {
      Mouse.click(MOUSE_LEFT);
    }
  }
}
