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

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
//create object to control an LCD.
//number of lines in display=1
LCD4Bit_mod lcd = LCD4Bit_mod(2);


void setup() {
    Serial.begin(9600);
    pinMode(13, OUTPUT);  //we'll use the debug LED to output a heartbeat
    lcd.init();
    //optionally, now set up our application-specific display settings, overriding whatever the lcd did in lcd.init()
    //lcd.commandWrite(0x0F);//cursor on, display on, blink on.  (nasty!)
    lcd.clear();
    // Start up the library
    sensors.begin();   
}

void loop() {
    tmElements_t tm;
    float tempvalue;
    char lcdtime[32] = "";
    char lcdtemp[16] = "";
    char test[5] = "1234";
    digitalWrite(13, HIGH);
    if (RTC.read(tm)) {      
      char *pTime = lcdtime;
      lcd.cursorTo(1, 0);  //line=1, x=0
      pTime = build_time_str(pTime, tm.Hour, 2, ':');    
      pTime = build_time_str(pTime, tm.Minute, 2, ':');
      pTime = build_time_str(pTime, tm.Second, 2, ' ');         
      lcd.printIn(lcdtime);
     }
     tempvalue = get_temp(sensors);
     dtostrf(tempvalue, 3, 2, lcdtemp);
     lcd.cursorTo(2, 0);
     lcd.printIn(lcdtemp);
     delay(1000);
     digitalWrite(13, LOW);
}

float get_temp(DallasTemperature sensors)
{
  sensors.requestTemperatures(); // Send the command to get temperatures
  return sensors.getTempCByIndex(0); 
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
