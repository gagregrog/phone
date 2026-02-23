#include <Arduino.h>

#define IN1 25
#define IN2 32
#define ENA 27
#define TRIGGER 14

// ms (20 Hz) --> 20 Cycles/Sec -> 1 Cycle / 50ms
const unsigned long halfPeriod = 25;

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  digitalWrite(ENA, LOW);
  
  pinMode(TRIGGER, INPUT_PULLUP);
}

void loop() {
  bool isTriggered = digitalRead(TRIGGER) == LOW;
  Serial.println(isTriggered);
  if (isTriggered) {
    digitalWrite(ENA, HIGH);

    // Direction 1
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    delay(halfPeriod);

    // Direction 2
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    delay(halfPeriod);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(ENA, LOW);
    delay(10);
  }
}
