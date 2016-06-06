#include <stdio.h>
#include <LCD4Bit_mod.h>
#include <string.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino & Temperature Sensor
#define ONE_WIRE_BUS    2
#define PIN_RELAY       3
#define PIN_DEBUG       13

#define STAGE_INVALIDA -1
#define STAGE_0         0
#define STAGE_1         1
#define STAGE_2         2
#define STAGE_3         3
#define STAGE_4         4
#define MAX_STAGE_COUNT 5

enum KEY_VALUE {KEY_INVALID = -1, KEY_START, KEY_UP, KEY_DOWN, KEY_SUB_STAGE, KEY_STAGE};

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
//create object to control an LCD.
//number of lines in display=1
LCD4Bit_mod lcd = LCD4Bit_mod(2);

// Global variables
int NUM_KEYS = 5;
int adc_key_val[5] ={30, 150, 360, 535, 760 };
bool relay = false;
bool debug = false;
int stage = STAGE_INVALIDA;
int sub_stage = -1;
bool start_work = false;
float last_temp = 0;
tmElements_t start_tm;

// Stage Configuration
int stage0_temp = 68;
int stage1_temp = 60;
int stage1_time_h = 1;
int stage1_time_m = 15;
int stage2_temp = 75;
int stage3_temp = 100;
int stage3_time_h = 1;
int stage3_time_m = 15;
int stage4_temp = 25;

/**
 * Convert ADC value to key number
 */
KEY_VALUE convert_key()
{
  // read the value from the sensor;
  unsigned int input = analogRead(0);
  int k;
  for (k = 0; k < NUM_KEYS; k++)
  {
    if (input < adc_key_val[k])
    {
      return (enum KEY_VALUE) k;
    }
  }
  return INVALID_VALUE;
}

/**
 * Get key value using stabilization method
 */
KEY_VALUE get_key()
{
  KEY_VALUE key1, key2;
  key1 = convert_key();
  delay(10);
  key2 = convert_key();
  if (key1 == key2)
  {
    return key1;
  }
  else
  {
    return INVALID_VALUE;
  }
}

/**
 * Get temperature by Dallas Sensor
 */
float get_temperature()
{
  // Send the command to get temperatures
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  if (temp > 100 || temp < 0) {
    return last_temp;
  } else {
    last_temp = temp;
    return temp;
  }
}

void turn_off_debug()
{
  if (debug)
  {
    digitalWrite(PIN_DEBUG, LOW);
    debug = false;
  }
}

void turn_on_debug()
{
  if (!debug)
  {
    digitalWrite(PIN_DEBUG, HIGH);
    debug = true;
  }
}

void turn_on_relay()
{
  if (!relay)
  {
    digitalWrite(PIN_RELAY, LOW);
    relay = true;
  }
}

void turn_off_relay()
{
  if (relay)
  {
    digitalWrite(PIN_RELAY, HIGH);
    relay = false;
  }
}

char* build_time_str(char* p, int value, int length, char sepreator)
{
  int i = 0;
  if(value < 10 && length == 2)
  {
      *p = '0';
      p ++;
      i ++;
  }
  itoa(value, p, 10);
  p += (length - i);
  *p = sepreator;
  p ++;
  return p;
}

int switch_stage()
{
  return ++stage % MAX_STAGE_COUNT;
}

void display_stage_init_LCD()
{
  char line2[16];
  switch (stage)
  {
    case STAGE_0:
      lcd.cursorTo(1, 0);
      lcd.printIn("0                ");
      lcd.cursorTo(2, 0);
      sprintf(line2, "Set temp %02d C   ", stage0_temp);
      lcd.printIn(line2);
    break;
    case STAGE_1:
      lcd.cursorTo(1, 0);
      lcd.printIn("1                ");
      lcd.cursorTo(2, 0);
      sprintf(line2, "Set temp %02d C   ", stage1_temp);
      lcd.printIn(line2);
    break;
    case STAGE_2:
      lcd.cursorTo(1, 0);
      lcd.printIn("2                 ");
      lcd.cursorTo(2, 0);
      sprintf(line2, "Set temp %02d C   ", stage2_temp);
      lcd.printIn(line2);
    break;
    case STAGE_3:
      lcd.printIn("3                 ");
      lcd.cursorTo(2, 0);
      sprintf(line2, "Set time (%02d):%02d", stage3_time_h, stage3_time_m);
      lcd.printIn(line2);
    break;
    case STAGE_4:
      lcd.printIn("4                 ");
      lcd.cursorTo(2, 0);
      sprintf(line2, "Set temp %02d C   ", stage4_temp);
      lcd.printIn(line2);
    break;
  }
}

// TODO: 2
int switch_sub_stage(LCD4Bit_mod lcd, int sub_stage)
{
  char line1[16];
  char line2[16];
  switch(sub_stage){
    case -1 :
    case 2 :
      if(stage == STAGE_1){
        lcd.cursorTo(1, 0);
        lcd.printIn("1                ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C   ", stage1_temp);
        lcd.printIn(line2);
      }else if(stage == STAGE_3){
        lcd.cursorTo(1, 0);
        lcd.printIn("3                 ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time (%02d):%02d", stage3_time_h, stage3_time_m);
        lcd.printIn(line2);
      }
      sub_stage = 0;
    break;
    case 0 :
      if(stage == STAGE_1){
        lcd.cursorTo(1, 0);
        sprintf(line1, "1 temp set %02d C ", stage1_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time (%02d):%02d  ", stage1_time_h, stage1_time_m);
        lcd.printIn(line2);
        sub_stage ++;
      }else if(stage == STAGE_3){
        lcd.cursorTo(1, 0);
        lcd.printIn("3                ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time %02d:(%02d)  ", stage3_time_h, stage3_time_m);
        lcd.printIn(line2);
        sub_stage += 2;
      }
    break;
    case 1 :
      if(stage == STAGE_1){
        lcd.cursorTo(1, 0);
        sprintf(line1, "1 temp set %02d C ", stage1_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time %02d:(%02d)  ", stage1_time_h, stage1_time_m);
        lcd.printIn(line2);
        sub_stage ++;
      }
    break;
  }
  return sub_stage;
}


// TODO: 3
void setting(int plus_minus)
{
  char line1[16];
  char line2[16];
  switch (stage){
      case 0 :
        stage0_temp += (1 * plus_minus);
        stage0_temp = stage0_temp > 100 ? 100 : stage0_temp;
        stage0_temp = stage0_temp < 0 ? 0 : stage0_temp;
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C", stage0_temp);
        lcd.printIn(line2);
      break;
      case 1 :
        if(sub_stage == 0) {
          stage1_temp += (1 * plus_minus);
          stage1_temp = stage1_temp > 100 ? 100 : stage1_temp;
          stage1_temp = stage1_temp < 0 ? 0 : stage1_temp;
          lcd.cursorTo(2, 0);
          sprintf(line2, "Set temp %02d C", stage1_temp);
          lcd.printIn(line2);
        }else if(sub_stage == 1){
          stage1_time_h += (1 * plus_minus);
          stage1_time_h = stage1_time_h > 99 ? 99 : stage1_time_h;
          stage1_time_h = stage1_time_h < 0 ? 0 : stage1_time_h;
          lcd.cursorTo(2, 0);
          sprintf(line2, "Set time (%02d):%02d", stage1_time_h, stage1_time_m);
          lcd.printIn(line2);
        }else if(sub_stage == 2){
          stage1_time_m += (1 * plus_minus);
          stage1_time_m = stage1_time_m == 60 ? 0 : stage1_time_m;
          stage1_time_m = stage1_time_m < 0 ? 0 : stage1_time_m;
          lcd.cursorTo(2, 0);
          sprintf(line2, "Set time %02d:(%02d)", stage1_time_h, stage1_time_m);
          lcd.printIn(line2);
        }
        break;
      case 2 :
        stage2_temp += (1 * plus_minus);
        stage2_temp = stage2_temp > 100 ? 100 : stage2_temp;
        stage2_temp = stage2_temp < 0 ? 0 : stage2_temp;
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C", stage2_temp);
        lcd.printIn(line2);
      break;
      case 3 :
        if(sub_stage == 0){
          stage3_time_h += (1 * plus_minus);
          stage3_time_h = stage3_time_h > 99 ? 99 : stage3_time_h;
          stage3_time_h = stage3_time_h < 0 ? 0 : stage3_time_h;
          lcd.cursorTo(2, 0);
          sprintf(line2, "Set time (%02d):%02d", stage3_time_h, stage3_time_m);
          lcd.printIn(line2);
        }else if(sub_stage == 2){
          stage3_time_m += (15 * plus_minus);
          stage3_time_m = stage3_time_m == 60 ? 0 : stage3_time_m;
          stage3_time_m = stage3_time_m < 0 ? 0 : stage3_time_m;
          lcd.cursorTo(2, 0);
          sprintf(line2, "Set time %02d:(%02d)", stage3_time_h, stage3_time_m);
          lcd.printIn(line2);
        }
        break;
      case 4 :
        stage4_temp += (1 * plus_minus);
        stage4_temp = stage4_temp > 100 ? 100 : stage4_temp;
        stage4_temp = stage4_temp < 0 ? 0 : stage4_temp;
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C", stage4_temp);
        lcd.printIn(line2);
      break;
    }
}

void display_current_temperature(int current_temp, int target_temp)
{
  char line[16];
  char lcdtemp[16] = "";
  lcd.cursorTo(1, 0);
  dtostrf(current_temp, 3, 2, lcdtemp);
  sprintf(line, "s:%d %sC->%02dC", stage, lcdtemp, target_temp);
  lcd.printIn(line);
}

void display_current_time(char *current_time)
{
  char line[16];
  lcd.cursorTo(2, 0);
  sprintf(line, "%s        ", current_time);
  lcd.printIn(line);
}

void check_temperature(float templimit, char *log)
{
  char log[64] = "";
  // FIXME: over 100C water temperature overflow?
  templimit = relay ? templimit + 0.25 : templimit - 0.25;
  if (tempvalue > templimit)
  {
    turn_off_relay();
  }
  else
  {
    turn_on_relay();
  }
  sprintf(log, "stage: %d, relay: \t%s", stage, relay ? "On" : "Off");
  Serial.println(log);
}

int get_current_time(char *lcdtime)
{
  int pass_time;
  tmElements_t tm;
  char log[64] = "";
  int pass_hour;
  int pass_minute;
  int pass_second;
  if (RTC.read(tm))
  {
    pass_time = (tm.Hour - start_tm.Hour) * 3600 + (tm.Minute - start_tm.Minute) * 60 + (tm.Second - start_tm.Second);
    pass_hour = floor(pass_time / 3600);
    pass_minute = floor((pass_time - (pass_hour * 3600)) / 60);
    pass_second = pass_time - pass_hour * 3600 - pass_minute * 60;
    lcdtime = build_time_str(lcdtime, pass_hour, 2, ':');
    lcdtime = build_time_str(lcdtime, pass_minute , 2, ':');
    lcdtime = build_time_str(lcdtime, pass_second, 2, ' ');
  }
  sprintf(log, "Time: %02d:%02d:%02d\t%d", tm.Hour, tm.Minute, tm.Second, pass_time);
  Serial.println(log);
  return pass_time;
}

void work()
{
    float tempvalue;
    char lcdtime[32] = "";
    int pass_time = 0;
    tempvalue = get_temperature();
    pass_time = get_current_time(*lcdtime);
    switch(stage) {
      case STAGE_0:
        display_current_temperature(temperature, stage0_temp);
        display_current_time(lcdtime);
        check_temperature((float) stage0_temp);
      break;
      case STAGE_1:
        display_current_temperature(temperature, stage1_temp);
        char line[16];
        lcd.cursorTo(2, 0);
        sprintf(line, "%s%02dh:%02dm", lcdtime, stage1_time_h, stage1_time_m);
        lcd.printIn(line);
        check_temperature((float) stage1_temp);
        int set_time = stage1_time_h * 3600 + stage1_time_m * 60;
        if(set_time - pass_time <= 0)
        {
          turn_off_relay();
          stage = switch_stage();
          RTC.read(start_tm);
        }
      break;
      case STAGE_2:
        display_current_temperature(temperature, stage2_temp);
        display_current_time(lcdtime);
        check_temperature((float) stage2_temp);
      break;
      case STAGE_3:
        display_current_temperature(temperature, stage3_temp);
        display_current_time(lcdtime);
        check_temperature((float) stage3_temp);
      break;
      case STAGE_4:
        display_current_temperature(temperature, stage4_temp);
        lcd.cursorTo(2, 0);
        lcd.printIn("Freezing ");
        float templimit = (float) stage4_temp;
        templimit = relay ? templimit - 0.5 : templimit + 0.5;
        if (tempvalue < templimit)
        {
          turn_off_relay();
        }
        else
        {
          turn_on_relay();
        }
        char log[64] = "";
        sprintf(log, "freezeing, relay: \t%s, current limit temp: %f", relay ? "On" : "Off", templimit);
        Serial.println(log);
      break;
    }
}
/*********************************************
 *
 * Main function setup(); -> while(1) { loop();}
 *
 ********************************************/

void setup() {
    Serial.begin(9600);
    pinMode(PIN_DEBUG, OUTPUT);
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, HIGH);
    digitalWrite(PIN_DEBUG, LOW);
    lcd.init();
    lcd.clear();
    sensors.begin();

    lcd.cursorTo(1, 0);
    lcd.printIn("beer brew");
}

void loop() {
  KEY_VALUE key = get_key();
  switch (key)
  {
    case KEY_START:
      start_work = true;
      RTC.read(start_tm);
    break;
    case KEY_UP:
      start_work = false;
      setting(1);
    break;
    case KEY_DOWN:
      start_work = false;
      setting(-1);
    break;
    case KEY_SUB_STAGE:
      start_work = false;
      sub_stage = switch_sub_stage(lcd, sub_stage);
    break;
    case KEY_STAGE:
      start_work = false;
      sub_stage = 0;
      stage = switch_stage();
      display_stage_init_LCD();
    break;
    default:
    break;
  }
  if (start_work)
  {
    turn_on_debug();
    work();
  }
  else
  {
    turn_off_debug();
  }
}
