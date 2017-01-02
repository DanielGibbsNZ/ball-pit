#include <SoftwareSerial.h>

#define LCD_RX_PIN 17
#define LCD_TX_PIN 17
#define DISTANCE_SENSOR_PIN 5
#define BUTTON_1_PIN 2
#define BUTTON_2_PIN 4
#define SPEAKER_PIN 14

#define BALL_DISTANCE_THRESHOLD 300

SoftwareSerial lcd = SoftwareSerial(LCD_RX_PIN, LCD_TX_PIN);

long num_balls = 0;
bool is_ball = false;

void setup() {
  // Set up LCD display.
  digitalWrite(LCD_TX_PIN, HIGH);
  delay(1000);

  // Set up the pin modes.
  pinMode(LCD_TX_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(BUTTON_1_PIN, INPUT);
  pinMode(BUTTON_2_PIN, INPUT);

  // Set up the top line of the LCD display as it never changes.
  lcd.begin(9600);
  lcd.write(0xFE);
  lcd.write(0x01);
  lcd.write(0xFE);
  lcd.write(0x80);
  lcd.print("=== BALL PIT ===");

  // Play boot sound.
  sound(1046, 50000); // C6
  sound(1174, 50000); // D6
  sound(1318, 50000); // E6
  sound(1396, 50000); // F6
  sound(1567, 50000); // G6

  // Sense the ball, then ignore the outcome.
  sense_ball();
  num_balls = 0;
  
  delay(1000);
}

int loop_count = 0;

void loop() {
  sense_ball();
  check_buttons();

  loop_count++;
  if (loop_count % 10 == 0) {
    update_display();
    loop_count = 0;
  }
  
  delay(10);
}

void check_buttons() {
  bool button1 = (digitalRead(BUTTON_1_PIN) == 0);
  bool button2 = (digitalRead(BUTTON_2_PIN) == 0);
  
  if (button1 && button2) {
    num_balls = 0;
  } else if (button1) {
    num_balls += 10;
  } else if (button2) {
    num_balls++;
  }
}

void sense_ball() {
  // Read the distance sensor pin 5 times and take the average to eliminate any noise.
  float distance_sum = 0;
  int i;
  for (i = 0; i < 5; i++) {
    distance_sum += analogRead(DISTANCE_SENSOR_PIN);
  }
  float distance = distance_sum / 5.0;

  // Detect whether a new ball has been seen or not.
  bool new_is_ball = distance < BALL_DISTANCE_THRESHOLD;
  if (new_is_ball && !is_ball) {
    num_balls++;
    sound(1000 + num_balls, 20000);
  }
  is_ball = new_is_ball;
}

void update_display() {
  // Update the second line of the LCD display.
  lcd.write(0xFE);
  lcd.write(0xC0);

  char buf[17];
  if (num_balls == 1) {
    lcd.print("1 ball          ");
  } else {
    sprintf(buf, "%d balls", num_balls);
    sprintf(buf, "%-16s", buf);
    lcd.print(buf);
  }
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
