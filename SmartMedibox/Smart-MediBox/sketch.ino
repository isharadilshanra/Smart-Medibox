#include <Wire.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>
#include <DHTesp.h> 
#include <WiFi.h> 
#include <time.h> 

#define NTP_SERVER     "pool.ntp.org" 
#define UTC_OFFSET     0 
#define UTC_OFFSET_DST 0 

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1 
#define SDA 21 
#define SCL 22 
#define SCREEN_ADDRESS 0x3C

#define BUZZER 5
#define LED_1 15
#define LED_2 17
#define LED_3 19
#define LED_4 18
#define LED_5 16
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define PB_SNOOZE 4
#define DHT_PIN 12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor; 

int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

int timezone_offset = 0;
int timezone_dst_offset = 0;

int number_of_alarms = 2;
int alarm_hours[] = {4, 1};
int alarm_minutes[] = {12, 10};
bool alarm_enabled[] = {true, true};

bool alarm_snooze_active = false;
unsigned long snooze_time = 0;

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 5; 
String modes[] = {"1.Set Time Zone", "2.Set Alarm 1", "3.Set Alarm 2", "4.View Alarms", "5.Delete Alarm"};

void setup() {

  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_SNOOZE, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  pinMode(LED_5, OUTPUT);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for ( ;; ); // Don't proceed, loop forever
  }

  display.display();
  delay(2000);

  display.clearDisplay();
  print_line("Welcome to Medibox", 2, 0, 0);
  delay(2000);

  display.clearDisplay();
  print_line("Please wait", 2, 0, 0);
  delay(1000);

  display.clearDisplay();
  print_line("Booting...", 2, 0, 0);
  delay(2000);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WiFi", 2, 0, 0);
  }

  display.clearDisplay();
  print_line("Connected to WiFi", 2, 0, 0);

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {
  update_time_and_check_alarm();
  check_temprature_and_humidity();
  if (digitalRead(PB_OK) == LOW) {
    delay(1000);
    Serial.println("Menu");
    go_to_menu();
  }
  
}

void print_line(String text, int text_size, int row, int column) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.print(text);
  display.display();
}


void print_time_now() {

  display.clearDisplay();
  
  display.setTextSize(1); // 6x8px font
  display.setTextColor(SSD1306_WHITE);
  
  // Starting at x=0 (far left)
  int x_pos = 0;
  
  // Days (DD)
  display.setCursor(x_pos, 0);
  if(days < 10) display.print("0");
  display.print(days);
  x_pos += 12; 

  // Separator
  display.setCursor(x_pos, 0);
  display.print(":");
  x_pos += 6;

  // Hours (HH)
  display.setCursor(x_pos, 0);
  if(hours < 10) display.print("0");
  display.print(hours);
  x_pos += 12;

  // Separator
  display.setCursor(x_pos, 0);
  display.print(":");
  x_pos += 6;

  // Minutes (MM)
  display.setCursor(x_pos, 0);
  if(minutes < 10) display.print("0");
  display.print(minutes);
  x_pos += 12;

  // Separator 
  display.setCursor(x_pos, 0);
  display.print(":");
  x_pos += 6;

  // Seconds (SS) 
  display.setCursor(x_pos, 0);
  if(seconds < 10) display.print("0");
  display.print(seconds);

  display.display();
}


void update_time_and_check_alarm(void) {
  display.clearDisplay();
  update_time();
  print_time_now();

  for (int i = 0; i < number_of_alarms; i++) {
    if (alarm_enabled[i] && hours == alarm_hours[i] && minutes == alarm_minutes[i]) {
      ring_alarm();
      alarm_enabled[i] = false;
    }
    else if (alarm_snooze_active && (millis() - snooze_time >= 30000)) { //30 second snooze
      ring_alarm();
    }
  }
}
void update_time(void) {
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char day_str[8];
  char hour_str[8];
  char min_str[8];
  char sec_str[8];

  strftime(day_str,8, "%d", &timeinfo);
  strftime(sec_str,8, "%S", &timeinfo);
  strftime(hour_str,8, "%H", &timeinfo);
  strftime(min_str,8, "%M", &timeinfo);

  hours = atoi(hour_str);
  minutes = atoi(min_str);
  days = atoi(day_str);
  seconds = atoi(sec_str);
}

void ring_alarm(void) {
  display.clearDisplay();
  print_line("Medicine Time", 2, 0, 0);
  digitalWrite(LED_1, HIGH); 

  bool break_happened = false;

  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {

    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) { 
        delay(200);
        break_happened = true;
        alarm_snooze_active = false;
        break;
      } else if (digitalRead(PB_SNOOZE) == LOW) { 
        delay(200);
        alarm_snooze_active = true;
        snooze_time = millis(); 
        break_happened = true;
        break;
      }

      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  delay(200);
  digitalWrite(LED_1, LOW); 
}

void go_to_menu(void) {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 1, 0, 0);

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      current_mode += 1;
      current_mode %= max_modes;
      delay(200);
    } else if (pressed == PB_DOWN) {
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
        delay(200);
      }
    } else if (pressed == PB_OK) {
      Serial.println(current_mode);
      delay(200);
      run_mode(current_mode);
    } else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
}

int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    } else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    } else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    } else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }
    update_time();
  }
}

void check_temprature_and_humidity(void) {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  bool state_flag = true;
  if (data.temperature > 32) {
    state_flag = false;
    digitalWrite(LED_2, HIGH);
    print_line("High temprature", 1, 40, 0);
    delay(200);
  } else if (data.temperature < 24) {
    state_flag = false;
    digitalWrite(LED_3, HIGH);
    print_line("low temprature", 1, 40, 0);
    delay(200);
  }

  if (data.humidity > 80) {
    state_flag = false;
    digitalWrite(LED_4, HIGH);
    print_line("High humidity", 1, 50, 0);
    delay(200);
  } else if (data.humidity < 65) {
    state_flag = false;
    digitalWrite(LED_4, HIGH);
    print_line("Low humidity", 1, 50, 0);
    delay(200);
  }

  if (state_flag) {
    digitalWrite(LED_5, LOW);
    delay(200);
  }
}

void run_mode(int mode) {
  if (mode == 0) {
    set_timezone();
  } else if (mode == 1 || mode == 2) {
    set_alarm(mode - 1);
  } else if (mode == 3) {
    view_alarms();
  } else if (mode == 4) {
    delete_alarm();
  }
}

void fetch_timezone(int offset, int dst_offset) {
  timezone_offset = offset;
  timezone_dst_offset = dst_offset;
  configTime(timezone_offset, timezone_dst_offset, NTP_SERVER);
}

void set_timezone() {
  int temp_timezone_offset = timezone_offset / 3600; 
  while (true) {
    display.clearDisplay();
    print_line("Set UTC_Offset for Hour: " + String(temp_timezone_offset), 1, 0, 0);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
        delay(200);
        temp_timezone_offset += 1;
        temp_timezone_offset = temp_timezone_offset % 24;
    } else if (pressed == PB_DOWN) {
        delay(200);
        temp_timezone_offset -= 1;
        if (temp_timezone_offset < 0) {
            temp_timezone_offset = 23;
        }
    } else if (pressed == PB_OK) {
      delay(200);
      fetch_timezone(timezone_offset % 3600 + temp_timezone_offset * 3600, timezone_dst_offset);
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  temp_timezone_offset = (timezone_offset % 3600) / 60; 

  while (true) {
    display.clearDisplay();
    print_line("Set UTC_Offset for Minutes: " + String(temp_timezone_offset), 1, 0, 0);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
        delay(200);
        temp_timezone_offset += 1;
        temp_timezone_offset = temp_timezone_offset % 60;
    } else if (pressed == PB_DOWN) {
        delay(200);
        temp_timezone_offset -= 1;
        if (temp_timezone_offset < 0) {
            temp_timezone_offset = 59;
        }
    } else if (pressed == PB_OK) {
      delay(200);
      fetch_timezone((timezone_offset / 3600) * 3600 + temp_timezone_offset * 60, timezone_dst_offset);
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Timezone set", 1, 0, 0);
  delay(1000);
}

void set_alarm(int alarm) {
    int temp_hour = alarm_hours[alarm];

    while (true) {
        display.clearDisplay();
        print_line("Enter hour: " + String(temp_hour), 1, 0, 0);

        int pressed = wait_for_button_press();
        if (pressed == PB_UP) {
            delay(200);
            temp_hour += 1;
            temp_hour = temp_hour % 24;
        } else if (pressed == PB_DOWN) {
            delay(200);
            temp_hour -= 1;
            if (temp_hour < 0) {
                temp_hour = 23;
            }
        } else if (pressed == PB_OK) {
            delay(200);
            alarm_hours[alarm] = temp_hour;
            break;
        } else if (pressed == PB_CANCEL) {
            delay(200);
            break;
        }
    }

    int temp_minute = alarm_minutes[alarm];

    while (true) {
        display.clearDisplay();
        print_line("Enter minute: " + String(temp_minute), 1, 0, 0);

        int pressed = wait_for_button_press();
        if (pressed == PB_UP) {
            delay(200);
            temp_minute += 1;
            temp_minute = temp_minute % 60;
        } else if (pressed == PB_DOWN) {
            delay(200);
            temp_minute -= 1;
            if (temp_minute < 0) {
                temp_minute = 59;
            }
        } else if (pressed == PB_OK) {
            delay(200);
            alarm_minutes[alarm] = temp_minute;
            break;
        } else if (pressed == PB_CANCEL) {
            delay(200);
            break;
        }
    }

    alarm_enabled[alarm] = true;

    display.clearDisplay();
    print_line("Alarm is set", 1, 0, 0);
    delay(1000);
}

void view_alarms() {
  display.clearDisplay();
  for (int i = 0; i < number_of_alarms; i++) {
    if(alarm_enabled[i] == true){
      print_line("Alarm " + String(i + 1) + ": " + String(alarm_hours[i]) + ":" + String(alarm_minutes[i]), 1, i * 10, 0);
    }    
  }
  delay(3000);
}

void delete_alarm() {
  int alarm_to_delete = 0;
  while (true) {
    display.clearDisplay();
    print_line("Delete Alarm: " + String(alarm_to_delete + 1), 1, 0, 0);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      alarm_to_delete += 1;
      alarm_to_delete %= number_of_alarms;
    } else if (pressed == PB_DOWN) {
      delay(200);
      alarm_to_delete -= 1;
      if (alarm_to_delete < 0) {
        alarm_to_delete = number_of_alarms - 1;
      }
    } else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm_to_delete] = 0;
      alarm_minutes[alarm_to_delete] = 0;
      alarm_enabled[alarm_to_delete] = false;
      break;
    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm deleted", 1, 0, 0);
  delay(1000);
}

