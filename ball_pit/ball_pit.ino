#include <EEPROM.h>
#include <SoftwareSerial.h>

#define LCD_RX_PIN 17
#define LCD_TX_PIN 17
#define DISTANCE_SENSOR_PIN 5
#define BUTTON_1_PIN 2
#define BUTTON_2_PIN 4
#define SPEAKER_PIN 14

#define BALL_DISTANCE_THRESHOLD 300
#define BALL_DISTANCE_NUM_READS 1

SoftwareSerial lcd = SoftwareSerial(LCD_RX_PIN, LCD_TX_PIN);

long num_balls = 0;
bool is_ball = false;
bool display_needs_update = false;

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
  
  delay(1000);
  load_num_balls();
}

int loop_count = 0;

void loop() {
  sense_ball();

  if (loop_count % 20 == 0) {
    check_buttons();
    loop_count = 0;
  }
  
  if (display_needs_update) {
    update_display();
  }
  
  loop_count++;
  
  delay(10);
}

void check_buttons() {
  bool button1 = (digitalRead(BUTTON_1_PIN) == 0);
  bool button2 = (digitalRead(BUTTON_2_PIN) == 0);
  
  if (button1 && button2) {
    num_balls = 0;
    beep();
    save_num_balls();
    display_needs_update = true;
  } else if (button1) {
    num_balls += 10;
    beep();
    save_num_balls();
    display_needs_update = true;
  } else if (button2) {
    num_balls++;
    beep();
    save_num_balls();
    display_needs_update = true;
  }
}

void sense_ball() {
  // Read the distance sensor pin 5 times and take the average to eliminate any noise.
  float distance_sum = 0;
  int i;
  for (i = 0; i < BALL_DISTANCE_NUM_READS; i++) {
    distance_sum += analogRead(DISTANCE_SENSOR_PIN);
  }
  float distance = distance_sum / BALL_DISTANCE_NUM_READS;

  // Detect whether a new ball has been seen or not.
  bool new_is_ball = distance > BALL_DISTANCE_THRESHOLD;
  if (new_is_ball && !is_ball) {
    num_balls++;
    beep();
    save_num_balls();
    display_needs_update = true;
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
  display_needs_update = false;
}

void beep() {
  sound(1000 + num_balls, 20000);
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

void load_num_balls() {
  long four = EEPROM.read(0);
  long three = EEPROM.read(1);
  long two = EEPROM.read(2);
  long one = EEPROM.read(3);

  num_balls = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
  beep();
  display_needs_update = true;
}

void save_num_balls() {
  byte four = (num_balls & 0xFF);
  byte three = ((num_balls >> 8) & 0xFF);
  byte two = ((num_balls >> 16) & 0xFF);
  byte one = ((num_balls >> 24) & 0xFF);

  EEPROM.write(0, four);
  EEPROM.write(1, three);
  EEPROM.write(2, two);
  EEPROM.write(3, one);
}
