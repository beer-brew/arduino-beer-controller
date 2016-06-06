#include <stdio.h>
#include <LCD4Bit_mod.h>
#include <string.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define PIN_RELAY1 3
#define PIN_RELAY2 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
//create object to control an LCD.
//number of lines in display=1
LCD4Bit_mod lcd = LCD4Bit_mod(2);
int  adc_key_val[5] ={30, 150, 360, 535, 760 };
int NUM_KEYS = 5;
int key = -1;
boolean relay = false;
int stage = -1;
int sub_stage = -1;
int stage0_temp = 68;
int stage1_temp = 60;
int stage1_time_h = 1;
int stage1_time_m = 15;
int stage2_temp = 75;
int stage3_time_h = 1;
int stage3_time_m = 15;
int stage3_temp = 100;
int stage4_temp = 25;
int start_work = 0;
float last_temp = 0;
tmElements_t start_tm;


int convert_key(unsigned int input)
{
  int k;
  for (k = 0; k < NUM_KEYS; k++)
  {
    if (input < adc_key_val[k])
    {     
      return k;
    }
  }
  if (k >= NUM_KEYS)
  {
    return -1;     // No valid key pressed
  }
}

// Convert ADC value to key number
int get_key()
{
  unsigned int input;
  int key;
  input = analogRead(0);    // read the value from the sensor  
  key = convert_key(input);
  delay(10);
  input = analogRead(0);    // read the value from the sensor  
  int key1 = convert_key(input);
  if (key == key1)
  {
    return key;
  }
  else
  {
    return -1;
  }
}

float get_temp(DallasTemperature sensors)
{
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temp = sensors.getTempCByIndex(0);
  if (temp > 100 || temp < 0) {
    return last_temp;
  } else {
    last_temp = temp;
    return temp;
  }
}
char* build_time_str(char* p, int value, int length, char sepreator) {
  int i = 0;
  if(value < 10 && length == 2){
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

int switch_stage(LCD4Bit_mod lcd, int stage){  
  char line1[16];
  char line2[16];
  switch (stage){
      case -1 :
      case 4:
        lcd.cursorTo(1, 0);
        lcd.printIn("0                ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C   ", stage0_temp);
        lcd.printIn(line2);
        stage = 0;
      break;
      case 0 :
        lcd.cursorTo(1, 0);
        lcd.printIn("1                ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C   ", stage1_temp);
        lcd.printIn(line2);
        stage ++;
      break;
      case 1:
        lcd.cursorTo(1, 0);
        lcd.printIn("2                 ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C   ", stage2_temp);
        lcd.printIn(line2);
        stage ++;
      break;
      case 2:
        lcd.cursorTo(1, 0);
        lcd.printIn("3                 ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time (%02d):%02d", stage3_time_h, stage3_time_m);
        lcd.printIn(line2);
        stage ++;
      break;
      case 3 :
        lcd.cursorTo(1, 0);
        lcd.printIn("4                ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C   ", stage4_temp);
        lcd.printIn(line2);
        stage ++;
      break;

    }
    return stage;
}

int switch_sub_stage(LCD4Bit_mod lcd, int sub_stage)
{
  char line1[16];
  char line2[16];
  switch(sub_stage){
    case -1 :
    case 2 :
      if(stage == 1){
        lcd.cursorTo(1, 0);
        lcd.printIn("1                ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set temp %02d C   ", stage1_temp);
        lcd.printIn(line2);             
      }else if(stage == 3){
        lcd.cursorTo(1, 0);
        lcd.printIn("3                 ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time (%02d):%02d", stage3_time_h, stage3_time_m);
        lcd.printIn(line2);
      }
      sub_stage = 0;
    break;   
    case 0 :
      if(stage == 1){      
        lcd.cursorTo(1, 0);
        sprintf(line1, "1 temp set %02d C ", stage1_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time (%02d):%02d  ", stage1_time_h, stage1_time_m);
        lcd.printIn(line2);
        sub_stage ++;
      }else if(stage == 3){
        lcd.cursorTo(1, 0);
        lcd.printIn("3                ");
        lcd.cursorTo(2, 0);
        sprintf(line2, "Set time %02d:(%02d)  ", stage3_time_h, stage3_time_m);
        lcd.printIn(line2);
        sub_stage += 2;
      }
    break;
    case 1 :
      if(stage == 1){
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

void work()
{
    tmElements_t tm;
    float tempvalue;
    float templimit;
    char lcdtime[32] = "";
    char lcdtemp[16] = "";
    int set_time;
    int pass_hour;
    int pass_minute;
    int pass_second;
    int pass_time;
    char log[64] = "";
    if (RTC.read(tm)) 
    {
      char *pTime = lcdtime;
      pass_time = (tm.Hour - start_tm.Hour) * 3600 + (tm.Minute - start_tm.Minute) * 60 + (tm.Second - start_tm.Second);
      pass_hour = floor(pass_time / 3600);
      pass_minute = floor((pass_time - (pass_hour * 3600)) / 60);
      pass_second = pass_time - pass_hour * 3600 - pass_minute * 60;
      pTime = build_time_str(pTime, pass_hour, 2, ':');
      pTime = build_time_str(pTime, pass_minute , 2, ':');
      pTime = build_time_str(pTime, pass_second, 2, ' ');
    }
     
    tempvalue = get_temp(sensors);
    char line1[16];
    char line2[16];
    switch(stage){
      case 0 :
        lcd.cursorTo(1, 0);
        dtostrf(tempvalue, 3, 2, lcdtemp);
        sprintf(line1, "s:0 %sC->%02dC", lcdtemp, stage0_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        lcd.printIn(lcdtime);
        lcd.printIn("        ");
        templimit = (float) stage0_temp;
      break;
      case 1 :
        lcd.cursorTo(1, 0);
        dtostrf(tempvalue, 3, 2, lcdtemp);
        sprintf(line1, "s:1 %sC->%02dC", lcdtemp, stage1_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        sprintf(line2, "%s%02dh:%02dm", lcdtime, stage1_time_h, stage1_time_m);
        lcd.printIn(line2);
        templimit = (float) stage1_temp;
      break;
      case 2 :
        lcd.cursorTo(1, 0);
        dtostrf(tempvalue, 3, 2, lcdtemp);
        sprintf(line1, "s:2 %sC->%02dC", lcdtemp, stage2_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        lcd.printIn(lcdtime);
        lcd.printIn("        ");
        templimit = (float) stage2_temp;
      break;
      case 3 :
        lcd.cursorTo(1, 0);
        dtostrf(tempvalue, 3, 2, lcdtemp);
        sprintf(line1, "s:3 %sC->%02dC", lcdtemp, stage3_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        lcd.printIn(lcdtime);
        lcd.printIn("        ");
        templimit = (float) stage3_temp;
      break;      
     case 4:
        lcd.cursorTo(1, 0);
        dtostrf(tempvalue, 3, 2, lcdtemp);
        sprintf(line1, "s:4 %sC->%02dC", lcdtemp, stage4_temp);
        lcd.printIn(line1);
        lcd.cursorTo(2, 0);
        lcd.printIn("Freezing ");
        templimit = (float) stage4_temp;
      break;      
    }
    float limit = templimit;
    // todo: hard code stage4 logic here
    if (stage == 4)
    {
      limit = relay ? limit - 0.5 : limit + 0.5;
      if (tempvalue < limit) {
        if (relay) 
        {
          digitalWrite(PIN_RELAY1, HIGH);
          relay = false;
        }
      } 
      else 
      {
        if (!relay) 
        {
          digitalWrite(PIN_RELAY1, LOW);
          relay = true;
        }
      }
      sprintf(log, "freezeing, tempvalue: %sC relay: \t%s, current limit temp: %f", lcdtemp, (digitalRead(PIN_RELAY1) == LOW) ? "On" : "Off", limit);
      Serial.println(log);
      return;
    }
    limit = relay ? limit + 0.25 : limit - 0.25;
    if (tempvalue > limit) {
      if (relay) 
      {
        digitalWrite(PIN_RELAY1, HIGH);
        relay = false;
      }
    }
    else 
    {
      if (!relay) 
      {
        digitalWrite(PIN_RELAY1, LOW);
        relay = true;
      }
    }
    if(stage == 1){
      set_time = stage1_time_h * 3600 + stage1_time_m * 60;
      if(set_time - pass_time <= 0)
      {  
        digitalWrite(PIN_RELAY1, HIGH);
        stage = 2;
        RTC.read(start_tm);
      }
    }
    sprintf(log, "%d\t%02d:%02d:%02d\t%d\t%s\t%s", stage, tm.Hour, tm.Minute, tm.Second, pass_time, lcdtemp, (digitalRead(PIN_RELAY1) == LOW) ? "On" : "Off");
    Serial.println(log);
}


void setup() {
    Serial.begin(9600);
    pinMode(13, OUTPUT);  //we'll use the debug LED to output a heartbeat
    pinMode(PIN_RELAY1, OUTPUT);
    pinMode(PIN_RELAY2, OUTPUT);
    digitalWrite(PIN_RELAY1, HIGH);
    lcd.init();
    lcd.clear();
    // Start up the library
    sensors.begin(); 

    lcd.cursorTo(1, 0);
    lcd.printIn("beer brew");
}

void loop() {
  digitalWrite(13, HIGH);  
  key = get_key();    // convert into key press
  if(key == 4) //select stage
  {
    start_work = 0;
    sub_stage = 0;
    stage = switch_stage(lcd, stage);
  }
  if(key == 3) //select sub stage
  {
    start_work = 0;
    sub_stage = switch_sub_stage(lcd, sub_stage);
  }
  if(key == 1)
  {
    setting(1);
  }
  if(key == 2)
  {
    setting(-1);
  }
  
  if(key == 0)
  {
    start_work = 1;
    RTC.read(start_tm);
  }
  
  if(start_work == 1)
  {
    work();
  }
  //delay(1000);
  digitalWrite(13, LOW);
}

