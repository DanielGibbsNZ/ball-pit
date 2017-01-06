#include <EEPROM.h>
#include <SoftwareSerial.h>

#define LCD_RX_PIN 17
#define LCD_TX_PIN 17
#define DISTANCE_SENSOR_PIN 5
#define BUTTON_1_PIN 2
#define BUTTON_2_PIN 4
#define BUTTON_3_PIN 15
#define BUTTON_4_PIN 16
#define SPEAKER_PIN 14

#define BALL_DISTANCE_THRESHOLD 300
#define BALL_DISTANCE_NUM_READS 1

#define TIMER_DURATION 60

#define NUM_MODES 2

#define NORMAL_MODE 0
#define TIMED_MODE 1
#define DEBUG_MODE 999

SoftwareSerial lcd = SoftwareSerial(LCD_RX_PIN, LCD_TX_PIN);

int mode = NORMAL_MODE;

unsigned long num_balls = 0;
float distance = 0;

unsigned long num_balls_timed = 0;
bool timer_running = false;
unsigned int time_remaining = TIMER_DURATION;
unsigned long timer_start = 0;

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

  // Start the LCD and display the title.
  lcd.begin(9600);
  update_display_title();

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

  if (mode == TIMED_MODE) {
    update_timer();
  }

  if (display_needs_update || (loop_count % 10 == 0 && mode == DEBUG_MODE)) {
    update_display();
  }

  loop_count++;

  delay(10);
}

void check_buttons() {
  // Buttons 1 and 2 used to change ball count.
  // Button 3 used to change mode.
  // Button 4 used to start/stop timer in timed mode.
  bool button1 = (digitalRead(BUTTON_1_PIN) == 0);
  bool button2 = (digitalRead(BUTTON_2_PIN) == 0);
  bool button3 = (digitalRead(BUTTON_3_PIN) == 0);
  bool button4 = (digitalRead(BUTTON_4_PIN) == 0);

  if (button3 && button4) {
    if (mode == DEBUG_MODE) {
      return;
    }
    mode = DEBUG_MODE;
    update_display_title();
    beep();
    display_needs_update = true;
    return;
  }

  if (mode == NORMAL_MODE) {
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
  } else if (mode == TIMED_MODE) {
    if (button4 && !timer_running) {
      // If the timer hasn't started yet, start it.
      // Otherwise, reset the timer.
      if (timer_start == 0) {
        timer_start = millis();
        timer_running = true;
        sound(1046, 100000); // C6
        sound(1318, 100000); // E6
        sound(1567, 100000); // G6
        sound(2093, 100000); // C7
        display_needs_update = true;
      } else {
        reset_timer();
        beep();
        display_needs_update = true;
      }
    }
  }

  if (button3) {
    if (mode == DEBUG_MODE) {
      mode = NORMAL_MODE;
    } else {
      // Switch to the next mode.
      mode = (mode + 1) % NUM_MODES;
    }

    update_display_title();
    beep();
    display_needs_update = true;

    // When entering timed mode, reset everything.
    if (mode == TIMED_MODE) {
      reset_timer();
    }
  }
}

void reset_timer() {
  num_balls_timed = 0;
  timer_running = false;
  time_remaining = TIMER_DURATION;
  timer_start = 0;
}

void sense_ball() {
  // Read the distance sensor pin 5 times and take the average to eliminate any noise.
  float distance_sum = 0;
  int i;
  for (i = 0; i < BALL_DISTANCE_NUM_READS; i++) {
    distance_sum += analogRead(DISTANCE_SENSOR_PIN);
  }
  distance = distance_sum / BALL_DISTANCE_NUM_READS;

  // Detect whether a new ball has been seen or not.
  bool new_is_ball = distance > BALL_DISTANCE_THRESHOLD;
  if (new_is_ball && !is_ball) {
    if (mode == NORMAL_MODE) {
      num_balls++;
      if (num_balls % 1000 == 0) {
        play_victory_tune();
      } else {
        beep();
      }
      save_num_balls();
    } else if (mode == TIMED_MODE) {
      if (timer_running) {
        num_balls_timed++;
        // Balls in timer mode also count for normal mode.
        num_balls++;
        beep();
      }
    } else if (mode == DEBUG_MODE) {
      beep();
    }
    display_needs_update = true;
  }
  is_ball = new_is_ball;
}

void update_timer() {
  if (timer_running) {
    unsigned long now = millis();
    unsigned long elapsed = now - timer_start;
    float elapsed_seconds = elapsed / 1000.0;
    float remaining_seconds = TIMER_DURATION - elapsed_seconds;

    if (remaining_seconds < 0) {
      time_remaining = 0;
      timer_running = false;
      display_needs_update = true;
      sound(2093, 100000); // C7
      sound(1567, 100000); // G6
      sound(1318, 100000); // E6
      sound(1046, 100000); // C6
    } else if ((unsigned int)remaining_seconds != time_remaining) {
      time_remaining = (unsigned int)remaining_seconds;
      display_needs_update = true;
    }
  }
}

void update_display_title() {
  lcd.write(0xFE);
  lcd.write(0x01);
  lcd.write(0xFE);
  lcd.write(0x80);
  if (mode == NORMAL_MODE) {
    lcd.print("=== BALL PIT ===");
  } else if (mode == TIMED_MODE) {
    lcd.print("== TIMED MODE ==");
  } else if (mode == DEBUG_MODE) {
    lcd.print("---- DEBUG -----");
  }
}

void update_display() {
  // Update the second line of the LCD display.
  lcd.write(0xFE);
  lcd.write(0xC0);

  char buf[17];
  if (mode == NORMAL_MODE) {
    if (num_balls == 1) {
      sprintf(buf, "1 ball          ");
    } else {
      sprintf(buf, "%ld balls", num_balls);
      sprintf(buf, "%-16s", buf);
    }
  } else if (mode == TIMED_MODE) {
    if (num_balls_timed == 1) {
      sprintf(buf, "1 ball       %02ds", time_remaining);
    } else {
      sprintf(buf, "%ld balls", num_balls_timed);
      sprintf(buf, "%-12s %02ds", buf, time_remaining);
    }
  } else if (mode == DEBUG_MODE) {
    sprintf(buf, "%-16d", (unsigned int)distance);
  }
  lcd.print(buf);
  display_needs_update = false;
}

void beep() {
  sound(1000 + num_balls, 20000);
}

void play_victory_tune() {
  sound(392, 100000); // G4
  sound(523, 100000); // C5
  sound(659, 100000); // E5
  sound(783, 225000); // G5
  sound(659, 75000); // E5
  sound(783, 600000); // G5
}

void sound(unsigned int freq, unsigned long duration) {
  unsigned long us;
  unsigned long duration_, i;
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
  unsigned long four = EEPROM.read(0);
  unsigned long three = EEPROM.read(1);
  unsigned long two = EEPROM.read(2);
  unsigned long one = EEPROM.read(3);

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
