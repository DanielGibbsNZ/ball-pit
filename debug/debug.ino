#include <SoftwareSerial.h>

#define LCD_RX_PIN 17
#define LCD_TX_PIN 17
#define DISTANCE_SENSOR_PIN 5
#define SPEAKER_PIN 14

SoftwareSerial lcd = SoftwareSerial(LCD_RX_PIN, LCD_TX_PIN);

void setup() {
  // Set up LCD display.
  digitalWrite(LCD_TX_PIN, HIGH);
  delay(1000);

  // Set up the pin modes.
  pinMode(LCD_TX_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  // Set up the top line of the LCD display as it never changes.
  lcd.begin(9600);
  lcd.write(0xFE);
  lcd.write(0x01);
  lcd.write(0xFE);
  lcd.write(0x80);
  lcd.print("==== DEBUG ====");

  // Play boot sound.
  sound(1046, 50000); // C6
  sound(1174, 50000); // D6
  sound(1244, 50000); // Eb6
  sound(1396, 50000); // F6
  sound(1567, 50000); // G6
  
  delay(1000);
}

void loop() {
  // Read the distance sensor pin 5 times and take the average to eliminate any noise.
  float distance_sum = 0;
  int i;
  for (i = 0; i < 5; i++) {
    distance_sum += analogRead(DISTANCE_SENSOR_PIN);
  }
  float distance = distance_sum / 5.0;

  // Update the second line of the LCD display.
  lcd.write(0xFE);
  lcd.write(0xC0);
  char buf[17];
  sprintf(buf, "%f", distance);
  sprintf(buf, "%-16s", buf);
  lcd.print(distance);
  
  delay(10);
}

void sound(int freq, long duration) {
  unsigned long us;
  long duration_, i;
  us = (1000000 / (freq * 2));
  duration_ = (duration / (us * 2));
  for (i = 0; i < duration_; i++) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delayMicroseconds(us);
    digitalWrite(SPEAKER_PIN, LOW);
    delayMicroseconds(us);
  }
}

