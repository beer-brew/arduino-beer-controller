#include <stdio.h>
#include <LCD4Bit_mod.h>
#include <string.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// This is a debug Marco, open it will print some useful information in console.
//#define DEBUG

// Data wire is plugged into port 2 on the Arduino & Temperature Sensor
#define ONE_WIRE_BUS    2
#define PIN_RELAY       3
#define PIN_DEBUG       13

#define STAGE_INVALID       -1
#define STAGE_MASHING_IN    0
#define STAGE_MASHING       1
#define STAGE_MASHING_OUT   2
#define STAGE_BOIL          3
#define STAGE_FERMERTATION  4
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
float HEATER_TEMPERATURE_OVERFLOW = 0.25;
float FROZEN_TEMPERATURE_OVERFLOW = 0.5;
int adc_key_val[5] = { 30, 150, 360, 535, 760 };
bool relay = false;
bool debug = false;
int stage = STAGE_INVALID;
int sub_stage = STAGE_INVALID;
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

/*********************************************
 *
 * Keyboard
 *
 ********************************************/

/**
 * Convert ADC value to key number
 */
KEY_VALUE convert_key() {
    // read the value from the sensor;
    unsigned int input = analogRead(0);
    int k;
    for (k = 0; k < NUM_KEYS; k++) {
        if (input < adc_key_val[k]) {
            return (enum KEY_VALUE) k;
        }
    }
    return KEY_INVALID;
}

/**
 * Get key value using stabilization method
 */
KEY_VALUE get_key() {
    KEY_VALUE key1, key2;
    key1 = convert_key();
    delay(10);
    key2 = convert_key();
    if (key1 == key2) {
#ifdef DEBUG
    if (key1 != KEY_INVALID) {
        char log[64] = "";
        sprintf(log, "Input key: %d", key1);
        Serial.println(log);
    }
#endif
        return key1;
    }
    else {
        return KEY_INVALID;
    }
}

/*********************************************
 *
 * Temperature
 *
 ********************************************/

/**
 * Get temperature by Dallas Sensor
 */
float get_temperature() {
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

char* format_temperature(char* lcdtemp, float temp) {
    dtostrf(temp, 3, 2, lcdtemp);
    return lcdtemp;
}

/*********************************************
 *
 * Debug
 *
 ********************************************/

void turn_off_debug() {
    if (debug) {
        digitalWrite(PIN_DEBUG, LOW);
        debug = false;
    }
}

void turn_on_debug() {
    if (!debug)
    {
        digitalWrite(PIN_DEBUG, HIGH);
        debug = true;
    }
}

/*********************************************
 *
 * Relay
 *
 ********************************************/

void turn_on_relay() {
    if (!relay) {
        digitalWrite(PIN_RELAY, LOW);
        relay = true;
    }
}

void turn_off_relay() {
    if (relay) {
        digitalWrite(PIN_RELAY, HIGH);
        relay = false;
    }
}

/*********************************************
 *
 * Timer
 *
 ********************************************/

char* build_time_str(char* p, int value, int length, char sepreator) {
    int i = 0;
    if (value < 10 && length == 2) {
        *p = '0';
        p ++;
        i ++;
    }
    itoa(value, p, 10);
    p += (length - i);
    if (sepreator != ' ') {
        *p = sepreator;
        p ++;
    }
    return p;
}

/*********************************************
 *
 * LCD
 *
 ********************************************/

void lcd_print_line1(char* line) {
    lcd.cursorTo(1, 0);
    lcd.printIn(line);
}

void lcd_print_line2(char* line) {
    lcd.cursorTo(2, 0);
    lcd.printIn(line);
}

/*********************************************
 *
 * Business logic
 *
 ********************************************/

void display_set_temperature(int target_temp) {
    char line[17];
    sprintf(line, "Temperature:%3dC", target_temp);
    lcd_print_line2(line);
}

void display_set_time(int time_hour, int time_min, bool setHour) {
    char line[17];
    if (setHour) {
        sprintf(line, "Set time (%02d):%02d", time_hour, time_min);  
    } else {
        sprintf(line, "Set time %02d:(%02d)", time_hour, time_min);
    }
    lcd_print_line2(line);
}

void display_set_stage_line1() {
    char line[17];
    switch (stage) {
        case STAGE_MASHING_IN:
            sprintf(line, "%d) Mashing in   ", stage, sub_stage);
            lcd_print_line1(line);
            break;
        case STAGE_MASHING:
            sprintf(line, "%d) Mashing    :%d", stage, sub_stage);
            lcd_print_line1(line);
            break;
        case STAGE_MASHING_OUT:
            sprintf(line, "%d) Mashing out  ", stage, sub_stage);
            lcd_print_line1(line);
            break;
        case STAGE_BOIL:
            sprintf(line, "%d) Boil       :%d", stage, sub_stage);
            lcd_print_line1(line);
            break;
        case STAGE_FERMERTATION:
            sprintf(line, "%d) Fermertation ", stage, sub_stage);
            lcd_print_line1(line);
        break;
    }
}

void display_set_stage_line2() {
    switch (stage) {
        case STAGE_MASHING_IN:
            display_set_temperature(stage0_temp);
            break;
        case STAGE_MASHING:
            if (sub_stage == 0) {
                display_set_temperature(stage1_temp);
            } else if (sub_stage == 1) {
                display_set_time(stage1_time_h, stage1_time_m, true);
            } else if (sub_stage == 2) {
                display_set_time(stage1_time_h, stage1_time_m, false);
            }
            break;
        case STAGE_MASHING_OUT:
            display_set_temperature(stage2_temp);
            break;
        case STAGE_BOIL:
            if (sub_stage == 0) {
                display_set_time(stage3_time_h, stage3_time_m, true);
            } else if (sub_stage == 1) {
                display_set_time(stage3_time_h, stage3_time_m, false);
            }
            break;
        case STAGE_FERMERTATION:
            display_set_temperature(stage4_temp);
            break;
    }
}

// TODO: Maybe LCD will be twinkle
void display_set_stage() {
    display_set_stage_line1();
    display_set_stage_line2();
}

int switch_stage(int stage) {
    sub_stage = 0;
    return ++stage % MAX_STAGE_COUNT;
}

int switch_sub_stage(int sub_stage) {
    if (stage == STAGE_MASHING) {
        sub_stage = ++sub_stage % 3;
    } else if (stage == STAGE_BOIL) {
        sub_stage = ++sub_stage % 2;
    }
    return sub_stage;
}

int loop_value(int value, int increment, int start_value, int end_value) {
    int temp = value + increment;
    if (temp > end_value) {
        temp = start_value;
    } else if (temp < start_value) {
        temp = end_value;
    }
    return temp;
}

void setting(int plus_minus) {
    switch (stage) {
        case STAGE_MASHING_IN:
            stage0_temp = loop_value(stage0_temp, (1 * plus_minus), 0, 100);
            break;
        case STAGE_MASHING:
            if (sub_stage == 0) {
                stage1_temp = loop_value(stage1_temp, (1 * plus_minus), 0, 100);
            } else if (sub_stage == 1) {
                stage1_time_h = loop_value(stage1_time_h, (1 * plus_minus), 0, 99);
            } else if (sub_stage == 2) {
                stage1_time_m = loop_value(stage1_time_m, (1 * plus_minus), 0, 59);
            }
            break;
        case STAGE_MASHING_OUT:
            stage2_temp = loop_value(stage2_temp, (1 * plus_minus), 0, 100);
            break;
        case STAGE_BOIL:
            if (sub_stage == 0) {
                stage3_time_h = loop_value(stage3_time_h, (1 * plus_minus), 0, 99);
            } else if (sub_stage == 1) {
                stage3_time_m = loop_value(stage3_time_m, (15 * plus_minus), 0, 59);
            }
            break;
        case STAGE_FERMERTATION:
            stage4_temp = loop_value(stage4_temp, (1 * plus_minus), 0, 100);
            break;
    }
}

void display_current_temperature(float current_temp, int target_temp) {
    char lcdtemp[7] = "";
    char line[17];
    format_temperature(lcdtemp, current_temp);
    sprintf(line, "s:%d %sC->%3dC", stage, lcdtemp, target_temp);
    lcd_print_line1(line);
}

void display_current_time(char* current_time) {
    char line[17];
    sprintf(line, "Time:   %s", current_time);
    lcd_print_line2(line);
}

bool check_temperature(float tempvalue, float templimit, float overflow, bool greater_than) {
    if (greater_than) {
        // FIXME: over 100C water temperature overflow?
        templimit = relay ? templimit + overflow : templimit - overflow;
        return tempvalue > templimit;
    } else {
        templimit = relay ? templimit - overflow : templimit + overflow;
        return tempvalue < templimit;
    }
}

bool check_time(int pass_time, int time_hour, int time_min) {
    int set_time = 0;
    set_time = time_hour * 3600 + time_min * 60;
    return set_time - pass_time <= 0;
}

void get_display_time(tmElements_t start_t, tmElements_t end_t, int* pass_time, char* lcdtime) {
    int pass_hour;
    int pass_minute;
    int pass_second;
    char *pTime = lcdtime;
    
    *pass_time = (end_t.Hour - start_t.Hour) * 3600 + (end_t.Minute - start_t.Minute) * 60 + (end_t.Second - start_t.Second);
    pass_hour = floor(*pass_time / 3600);
    pass_minute = floor((*pass_time - (pass_hour * 3600)) / 60);
    pass_second = *pass_time - pass_hour * 3600 - pass_minute * 60;
    pTime = build_time_str(pTime, pass_hour, 2, ':');
    pTime = build_time_str(pTime, pass_minute , 2, ':');
    pTime = build_time_str(pTime, pass_second, 2, ' ');
#ifdef DEBUG
    char log[64] = "";
    sprintf(log, "Time: %02d:%02d:%02d\tpassed: %d", end_t.Hour, end_t.Minute, end_t.Second, *pass_time);
    Serial.println(log);
#endif
}

void work() {
    float tempvalue;
    char lcdtime[17] = "";
    int pass_time = 0;
    char line[17];
    tmElements_t tm;
    bool check_result = false;
    tempvalue = get_temperature();
    RTC.read(tm);
    get_display_time(start_tm, tm, &pass_time, lcdtime);
    switch(stage) {
        case STAGE_MASHING_IN:
            display_current_temperature(tempvalue, stage0_temp);
            display_current_time(lcdtime);
            check_result = check_temperature(tempvalue, (float) stage0_temp, HEATER_TEMPERATURE_OVERFLOW, true);
            break;
        case STAGE_MASHING:
            display_current_temperature(tempvalue, stage1_temp);
            sprintf(line, "%s%02dh:%0.2dm", lcdtime, stage1_time_h, stage1_time_m);
            lcd_print_line2(line);
            check_result = check_temperature(tempvalue, (float) stage1_temp, HEATER_TEMPERATURE_OVERFLOW, true);
            if (check_time(pass_time, stage1_time_h, stage1_time_m)) {
                turn_off_relay();
                stage = switch_stage(stage);
                RTC.read(start_tm);
            }
            break;
        case STAGE_MASHING_OUT:
            display_current_temperature(tempvalue, stage2_temp);
            display_current_time(lcdtime);
            check_result = check_temperature(tempvalue, (float) stage2_temp, HEATER_TEMPERATURE_OVERFLOW, true);
            break;
        case STAGE_BOIL:
            display_current_temperature(tempvalue, stage3_temp);
            display_current_time(lcdtime);
            check_result = check_temperature(tempvalue, (float) stage3_temp, HEATER_TEMPERATURE_OVERFLOW, true);
            break;
        case STAGE_FERMERTATION:
            display_current_temperature(tempvalue, stage4_temp);
            sprintf(line, "Fermertation    ");
            lcd_print_line2(line);
            check_result = check_temperature(tempvalue, (float) stage4_temp, FROZEN_TEMPERATURE_OVERFLOW, false);
            break;
    }
    if (check_result) {
        turn_off_relay();
    } else {
        turn_on_relay();
    }
#ifdef DEBUG
    char log[64] = "";
    sprintf(log, "pass_time:%d, lcdtime: %s", pass_time, lcdtime);
    Serial.println(log);  
    sprintf(log, "stage: %d, relay: \t%s", stage, relay ? "On" : "Off");
    Serial.println(log);
#endif
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

    char line[17];
    sprintf(line, "beer brew");
    lcd_print_line1(line);
}

void loop() {
    KEY_VALUE key = get_key();
    switch (key) {
        case KEY_START:
            if (stage != STAGE_INVALID) {
                start_work = true;
                RTC.read(start_tm);
            }
            break;
        case KEY_UP:
            start_work = false;
            setting(1);
            display_set_stage();
            break;
        case KEY_DOWN:
            start_work = false;
            setting(-1);
            display_set_stage();
            break;
        case KEY_SUB_STAGE:
            start_work = false;
            sub_stage = switch_sub_stage(sub_stage);
            display_set_stage();
            break;
        case KEY_STAGE:
            start_work = false;
            stage = switch_stage(stage);
            display_set_stage();
            break;
        default:
            break;
    }
    if (start_work) {
        turn_on_debug();
        work();
    } else {
        turn_off_debug();
    }
}
