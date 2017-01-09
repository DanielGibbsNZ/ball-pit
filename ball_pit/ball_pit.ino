#include <EEPROM.h>
#include <SoftwareSerial.h>

///////////////
// CONSTANTS //
///////////////

#define LCD_RX_PIN 17
#define LCD_TX_PIN 17
#define DISTANCE_SENSOR_PIN 5
#define BUTTON_1_PIN 2
#define BUTTON_2_PIN 4
#define BUTTON_3_PIN 15
#define BUTTON_4_PIN 16
#define SPEAKER_PIN 14

// Memory addresses.
#define MEMORY_ADDRESS_NUM_BALLS 0x0

// Ball sensing configuration.
#define BALL_DISTANCE_THRESHOLD 350
#define BALL_DISTANCE_NUM_READS 1

// Timed mode configuration.
#define TIMER_BEEP_MIN 1000
#define TIMER_BEEP_MAX 2000
#define TIMER_DURATION 60

//Target mode configuration.
#define TARGET_NUMBER 50

// Mode configuration.
#define NUM_MODES 3
#define NORMAL_MODE 0
#define TIMED_MODE 1
#define TARGET_MODE 2

// Debug mode should always be 999 and not be included in the number of modes.
#define DEBUG_MODE 999

/////////////
// GLOBALS //
/////////////

// The LCD display.
SoftwareSerial lcd = SoftwareSerial(LCD_RX_PIN, LCD_TX_PIN);

// If the LCD display needs an update.
bool display_needs_update = false;
bool display_title_needs_update = false;

// Whether there is currently a ball in front of the sensor.
// Used to detect changes in the sensor.
bool is_ball = false;

// The last value read from the distance sensor.
float sensor_value = 0;

// The current mode.
int mode = NORMAL_MODE;

// The total number of balls counted.
unsigned long num_balls = 0;

// If the timer is running.
bool timer_running = false;

// The number of balls counted during timed mode.
unsigned long num_balls_timed = 0;

// The number of seconds remaining in timed mode.
unsigned int time_remaining = TIMER_DURATION;

// The time that the timed mode timer started.
unsigned long timer_start = 0;

// The number of balls remaining in target mode.
unsigned long balls_remaining = TARGET_NUMBER;

// The number of balls currently counted.
unsigned long balls_got = 0;

// The time is has taken in target mode.
unsigned int time_taken = 0;

/////////////////////////
// SETUP AND MAIN LOOP //
/////////////////////////

void setup() {
  // Set up LCD display.
  digitalWrite(LCD_TX_PIN, HIGH);
  delay(1000);

  // Set up the pin modes.
  pinMode(LCD_TX_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(BUTTON_1_PIN, INPUT);
  pinMode(BUTTON_2_PIN, INPUT);
  pinMode(BUTTON_3_PIN, INPUT);
  pinMode(BUTTON_4_PIN, INPUT);

  // Start the LCD and display the title.
  lcd.begin(9600);
  update_display_title();

  // Play boot sound.
  play_boot_tune();

  delay(1000);
  load_num_balls();
}

void loop() {
  // Check the buttons every 20 cycles.
  static int loop_count = 0;
  if (loop_count % 20 == 0) {
    check_buttons();
    loop_count = 0;
  }
  loop_count++;

  // Check the distance sensor.
  check_sensor();

  // Update the timer if needed.
  if (mode == TIMED_MODE) {
    update_timer();
  }

  //Update the count if needed
  if(mode == TARGET_MODE){
    update_count();
  }

  // Update the display every 10 cycles in debug mode.
  if ((loop_count % 10 == 0) && mode == DEBUG_MODE) {
    display_needs_update = true;
  }

  // Update the display if needed.
  if (display_title_needs_update) {
    update_display_title();
  }
  if (display_needs_update) {
    update_display();
  }

  // Loop roughly 100 times a second.
  delay(10);
}

/////////////
// BUTTONS //
/////////////

void check_buttons() {
  bool button1 = (digitalRead(BUTTON_1_PIN) == 0);
  bool button2 = (digitalRead(BUTTON_2_PIN) == 0);
  bool button3 = (digitalRead(BUTTON_3_PIN) == 0);
  bool button4 = (digitalRead(BUTTON_4_PIN) == 0);

  // If button 3 and button 4 are pressed together, enter debug mode.
  if (button3 && button4) {
    enter_debug_mode();
    return;
  }

  // If button 3 is pressed, enter the next mode.
  if (button3) {
    enter_next_mode();
    return;
  }

  // If buttons 1 and/or 2 are pressed in normal mode, adjust the number of balls counted.
  //   Button 1: Increment the count by 10
  //   Button 2: Increment the count by 1
  //   Buttons 1 and 2: Reset the count to 0
  if (mode == NORMAL_MODE) {
    if (button1 && button2) {
      set_num_balls(0);
    } else if (button1) {
      set_num_balls(num_balls + 10);
    } else if (button2) {
      set_num_balls(num_balls + 1);
    }
  } 

  // If button 4 is pressed in either timed or target modes, start or reset the timer.
  else if (mode == TIMED_MODE || mode == TARGET_MODE) {
    if (button4 && !timer_running) {
      // If the timer hasn't started yet, start it.
      // Otherwise, reset the timer.
      if (timer_start == 0) {
        timer_start = millis();
        timer_running = true;
        play_timer_start_tune();
        display_needs_update = true;
      } else {
        reset_timer();
        beep();
        display_needs_update = true;
      }
    }
  }
}

void set_num_balls(unsigned long value) {
  num_balls = value;
  save_num_balls();
  beep();
  display_needs_update = true;
}

///////////
// MODES //
///////////

void enter_next_mode() {
  // If the current mode is debug, switch back to normal mode.
  // Otherwise switch to the next mode.
  if (mode == DEBUG_MODE) {
    mode = NORMAL_MODE;
  } else {
    // Switch to the next mode.
    mode = (mode + 1) % NUM_MODES;
  }

  // Beep and update the display.
  beep();
  display_title_needs_update = true;
  display_needs_update = true;

  // When entering timed mode, reset everything.
  if (mode == TIMED_MODE || mode == TARGET_MODE) {
    reset_timer();
  }
}

void enter_debug_mode() {
  if (mode == DEBUG_MODE) {
    return;
  }
  mode = DEBUG_MODE;
  beep();
  display_title_needs_update = true;
  display_needs_update = true;
}

////////////
// SENSOR //
////////////

void check_sensor() {
  // Read the distance sensor pin several times and take the average to eliminate any noise.
  float distance_sum = 0;
  unsigned int i;
  for (i = 0; i < BALL_DISTANCE_NUM_READS; i++) {
    distance_sum += analogRead(DISTANCE_SENSOR_PIN);
  }
  sensor_value = distance_sum / BALL_DISTANCE_NUM_READS;

  // Detect whether there is a ball in front of the sensor.
  bool is_ball_now = sensor_value > BALL_DISTANCE_THRESHOLD;

  // If the sensor previously didn't sense a ball and it does now, then a ball has been detected.
  if (is_ball_now && !is_ball) {
    if (mode == NORMAL_MODE) {
      increment_num_balls();
    } else if (mode == TIMED_MODE) {
      increment_num_balls_timed();
    } else if(mode == TARGET_MODE){
      increment_num_balls_target();
    } else if (mode == DEBUG_MODE) {
      beep();
    }
    display_needs_update = true;
  }
  is_ball = is_ball_now;
}

void increment_num_balls() {
  num_balls++;
  if (num_balls % 1000 == 0) {
    play_victory_tune();
  } else if (num_balls % 100 == 0) {
    play_short_tune();
  } else {
    beep();
  }
  save_num_balls();
}

void increment_num_balls_timed() {
  if (timer_running) {
    num_balls_timed++;
    // Balls in timer mode also count for normal mode.
    num_balls++;
    timer_beep();
    save_num_balls();
  }
}

void increment_num_balls_target(){
  if(timer_running){
    balls_got++;
    numi_balls++;
    timer_beep();
    save_num_balls();
  }
}

///////////
// COUNT //
///////////

void update_count(){
  unsigned long now = millis();
  unsigned long total_time = now - timer_start;
  float total_seconds = total_time/1000.0;
  int total_remaining = TARGET_NUMBER - balls_got;

  if(balls_remaining = 0){
    timer_running = false;
    display_needs_update = true;
    play_timer_end_tune();
  } else if(balls_remaining != total_remaining){
    balls_remaining = total_remaining;
    time_taken = (unsigned int)total_seconds;
    display_needs_update = true;    
    //print out the time and balls remaining
  }
}

///////////
// TIMER //
///////////

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
      play_timer_end_tune();
    } else if ((unsigned int)remaining_seconds != time_remaining) {
      time_remaining = (unsigned int)remaining_seconds;
      display_needs_update = true;
    }
  }
}

void reset_timer() {
  num_balls_timed = 0;
  timer_running = false;
  time_remaining = TIMER_DURATION;
  timer_start = 0;
  balls_remaining = TARGET_NUMBER;
  balls_got = 0;

}

/////////////
// DISPLAY //
/////////////

void update_display_title() {
  // Update the title of the display.
  lcd.write(0xFE);
  lcd.write(0x01);
  lcd.write(0xFE);
  lcd.write(0x80);

  if (mode == NORMAL_MODE) {
    lcd.print("=== BALL PIT ===");
  } else if (mode == TIMED_MODE) {
    lcd.print("== TIMED MODE ==");
  } else if(mode == TARGET_NUMBER);
    lcd.print("== TARGET MODE ==");
  } else if (mode == DEBUG_MODE) {
    lcd.print("---- DEBUG -----");
  }
  display_title_needs_update = false;
}

void update_display() {
  // Update the second line of the LCD display.
  lcd.write(0xFE);
  lcd.write(0xC0);

  // Write the line to a buffer before printing so that we can pad it with spaces.
  char buf[17];
  if (mode == NORMAL_MODE) {
    // Show the total number of balls counted.
    if (num_balls == 1) {
      sprintf(buf, "1 ball      ");
    } else {
      sprintf(buf, "%ld balls", num_balls);
      sprintf(buf, "%-16s", buf);
    }
  } else if (mode == TIMED_MODE) {
    // Show the number of balls counted and the time remaining.
    if (num_balls_timed == 1) {
      sprintf(buf, "1 ball     %02ds", time_remaining);
    } else {
      sprintf(buf, "%ld balls", num_balls_timed);
      sprintf(buf, "%-12s %02ds", buf, time_remaining);
    }
  } else if(mode = TARGET_MODE){
    // Show the number of balls remaining and the current time elapsed.
    if (balls_remaining == 1){
      sprintf(buf, "1 ball     %02ds", time_taken);
    } else{
      sprintf(buf, "%ld balls", balls_remaining);
      sprintf(buf, "%-12s %02ds", buf, time_taken);
    }
  } else if (mode == DEBUG_MODE) {
    // Show the sensor value.
    sprintf(buf, "%-16d", (unsigned int)sensor_value);
  }
  lcd.print(buf);
  display_needs_update = false;
}

/////////////
// SPEAKER //
/////////////

void beep() {
  // The default beep gets slightly higher as the number of balls increases.
  sound(1000 + (num_balls / 10), 40000);
}

void timer_beep() {
  // The timed mode beep gets higher as there is less time remaining.
  float time_elapsed = TIMER_DURATION - time_remaining;
  float t = time_elapsed / TIMER_DURATION;
  float freq = (t * (TIMER_BEEP_MAX - TIMER_BEEP_MIN) + TIMER_BEEP_MIN);
  sound(freq, 40000);
}

void play_boot_tune() {
  sound(1046, 50000); // C6
  sound(1174, 50000); // D6
  sound(1318, 50000); // E6
  sound(1396, 50000); // F6
  sound(1567, 50000); // G6
}

void play_short_tune() {
  sound(1046, 50000); // C6
  sound(1318, 50000); // E6
  sound(1567, 50000); // G6
}

void play_victory_tune() {
  sound(783, 100000); // G5
  sound(1046, 100000); // C6
  sound(1318, 100000); // E6
  sound(1567, 225000); // G6
  sound(1318, 75000); // E6
  sound(1567, 600000); // G6
}

void play_timer_start_tune() {
  sound(1046, 100000); // C6
  sound(1318, 100000); // E6
  sound(1567, 100000); // G6
  sound(2093, 100000); // C7
}

void play_timer_end_tune() {
  sound(2093, 100000); // C7
  sound(1567, 100000); // G6
  sound(1318, 100000); // E6
  sound(1046, 100000); // C6
}

void sound(unsigned int freq, unsigned long duration) {
  unsigned long us;
  unsigned long num_cycles, i;
  us = (1000000 / (freq * 2));
  num_cycles = (duration / (us * 2));
  for (i = 0; i < num_cycles; i++) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delayMicroseconds(us);
    digitalWrite(SPEAKER_PIN, LOW);
    delayMicroseconds(us);
  }
}

////////////
// MEMORY //
////////////

void load_num_balls() {
  num_balls = load_unsigned_long(MEMORY_ADDRESS_NUM_BALLS);
  beep();
  display_needs_update = true;
}

void save_num_balls() {
  save_unsigned_long(MEMORY_ADDRESS_NUM_BALLS, num_balls);
}

unsigned load_unsigned_long(int address) {
  unsigned long four = EEPROM.read(address);
  unsigned long three = EEPROM.read(address + 1);
  unsigned long two = EEPROM.read(address + 2);
  unsigned long one = EEPROM.read(address + 3);

  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void save_unsigned_long(int address, unsigned long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}
