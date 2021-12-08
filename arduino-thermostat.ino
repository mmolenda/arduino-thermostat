#include <DallasTemperature.h>
#include <Wire.h>
#include "RTClib.h"

// Config
const float DAY_TEMP_MIN = 25.95;
const float DAY_TEMP_MAX = 26.05;
const float NIGHT_TEMP_MIN = 20.45;
const float NIGHT_TEMP_MAX = 20.55;
const int DAY_START_HOUR = 9;
const int DAY_END_HOUR = 21;
const int PAST_READS_COUNT = 5;

// Constants
const int SENSOR_PIN = 8;
const int RELAY_PIN = 13;
const int LCD_FIELD_1 = 9;
const int LCD_FIELD_2 = 10;
const int LCD_FIELD_3 = 11;
const int LCD_FIELD_4 = 12;
const int LCD_SEG_A = 14;
const int LCD_SEG_B = 15;
const int LCD_SEG_C = 2;
const int LCD_SEG_D = 3;
const int LCD_SEG_E = 4;
const int LCD_SEG_F = 5;
const int LCD_SEG_G = 6;
const int LCD_SEG_DP = 7;
const int LCD_FIELDS_COUNT = 4;
const int LCD_FIELDS[LCD_FIELDS_COUNT] = {LCD_FIELD_1, LCD_FIELD_2, LCD_FIELD_3, LCD_FIELD_4};
const int LCD_SEGMENTS_COUNT = 8;
const int LCD_SEGMENTS[LCD_SEGMENTS_COUNT] = {LCD_SEG_A, LCD_SEG_B, LCD_SEG_C, LCD_SEG_D, LCD_SEG_E, LCD_SEG_F, LCD_SEG_G, LCD_SEG_DP};

// Global vars
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
RTC_DS1307 RTC;
float tempCelsius;
float pastReads[PAST_READS_COUNT];
float refTempMin;
float refTempMax;
char timeSymbol;
int counter = 0;
float pastAvgTemp;
float tempWeighted;
int currentHeat = HIGH;

void setup() {
  sensors.begin();
  Wire.begin();
  RTC.begin();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  if (! RTC.isrunning()) {
    RTC.adjust(DateTime(__DATE__,__TIME__));
  }
  for (int i = 0; i < LCD_FIELDS_COUNT; i++) {
    pinMode(LCD_FIELDS[i], OUTPUT);
  }
  for (int i = 0; i < LCD_SEGMENTS_COUNT; i++) {
    pinMode(LCD_SEGMENTS[i], OUTPUT);
  }
  Serial.begin(9600);
}

void loop() {
  clearLcdSegments();
  sensors.requestTemperatures();
  tempCelsius = sensors.getTempCByIndex(0);
  pastReads[counter] = tempCelsius;
  DateTime now = RTC.now();

  if (now.hour() >= DAY_START_HOUR && now.hour() < DAY_END_HOUR) {
    refTempMin = DAY_TEMP_MIN;
    refTempMax = DAY_TEMP_MAX;
    timeSymbol = 'd';
  } else {
    refTempMin = NIGHT_TEMP_MIN;
    refTempMax = NIGHT_TEMP_MAX;
    timeSymbol = 'n';
  }
  
  pastAvgTemp = countAverage(pastReads);
  if (currentHeat == HIGH) {
      // not heating
      tempWeighted = tempCelsius * 0.9;
  } else {
      // heating
      tempWeighted = tempCelsius * 1.1;
  }
  if ((tempCelsius < refTempMin) || (tempCelsius < refTempMax && tempWeighted > pastAvgTemp)) {
      // heating when temp below min or rising
      digitalWrite(RELAY_PIN, LOW);
      currentHeat = LOW;
      logTemp(now, timeSymbol, tempCelsius, pastAvgTemp, true);
    } else {
      // not heating when temp above max or declining
      digitalWrite(RELAY_PIN, HIGH);
      currentHeat = HIGH;
      logTemp(now, timeSymbol, tempCelsius, pastAvgTemp, false);
  }
  displayNumberLcd(tempCelsius, timeSymbol, currentHeat);

  if (counter < PAST_READS_COUNT - 1 ) {
    counter++;
  } else {
    counter = 0;
  }
}

float countAverage(float values[]) {
  float sum = 0;
  int divider = 0;
  for (int i = 0; i < PAST_READS_COUNT; i++) {
    if (values[i] != 0) {
      divider++;
    }
    sum = sum + values[i];
  }
  return sum / divider;
}

void displayNumberLcd(float value, char dayOrNight, int currentHeat) {
  char valueBits[4];
  dtostrf(value, 4, 1, valueBits);
  for (int x = 0; x < 1000; x++) {
    int lshift = 0;
    for (int i = 0; i < 4; i++) {
      char tempBit = valueBits[i];
      if (tempBit == '.') {
        lshift = 1;
      }
      printCharLcd(LCD_FIELDS[i - lshift], tempBit);
      delay(1);
    }
    printCharLcd(LCD_FIELDS[3], dayOrNight);
    delay(1);
    if (currentHeat == LOW) {
      printCharLcd(LCD_FIELDS[3], '.');
    }
    delay(1);
  } 
}

void toggleLcdField(int field) {
  for (int i = 0; i < LCD_FIELDS_COUNT; i++) {
    digitalWrite(LCD_FIELDS[i], HIGH);
  }
  digitalWrite(field, LOW);
}

void clearLcdSegments() {
  for (int j = 0; j < LCD_SEGMENTS_COUNT; j++) {
    digitalWrite(LCD_SEGMENTS[j], LOW);
  }
}

void printCharLcd(int field, char symbol) {
  int segmentStatesAll[12][LCD_SEGMENTS_COUNT] = {
    {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW, LOW},  // 0
    {LOW, HIGH, HIGH, LOW, LOW, LOW, LOW, LOW},      // 1
    {HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH, LOW},   // 2
    {HIGH, HIGH, HIGH, HIGH, LOW, LOW, HIGH, LOW},   // 3
    {LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH, LOW},    // 4
    {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH, LOW},   // 5
    {HIGH, LOW, HIGH, HIGH, HIGH, HIGH, HIGH, LOW},  // 6
    {HIGH, HIGH, HIGH, LOW, LOW, LOW, LOW, LOW},     // 7
    {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW}, // 8
    {HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH, LOW},  // 9
    {LOW, HIGH, HIGH, HIGH, HIGH, LOW, HIGH, LOW},   // d
    {LOW, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW}      // n
  };
  clearLcdSegments();
  toggleLcdField(field);
  if (symbol == '.') {
    digitalWrite(LCD_SEG_DP, HIGH);
  } else {
    int segmentStatesRow;
    switch (symbol) {
      case '0':
        segmentStatesRow = 0;
        break;
      case '1':
        segmentStatesRow = 1;
        break;
      case '2':
        segmentStatesRow = 2;
        break;
      case '3':
        segmentStatesRow = 3;
        break;
      case '4':
        segmentStatesRow = 4;
        break;
      case '5':
        segmentStatesRow = 5;
        break;
      case '6':
        segmentStatesRow = 6;
        break;
      case '7':
        segmentStatesRow = 7;
        break;
      case '8':
        segmentStatesRow = 8;
        break;
      case '9':
        segmentStatesRow = 9;
        break;
      case 'd':
        segmentStatesRow = 10;
        break;
      case 'n':
        segmentStatesRow = 11;
        break;
    }
    for (int i = 0; i < LCD_SEGMENTS_COUNT; i++) {
        digitalWrite(LCD_SEGMENTS[i], segmentStatesAll[segmentStatesRow][i]);
    }
  }
}

void logTemp(DateTime now, char timeSymbol, float temp, float avgTemp, bool relayState) {
  Serial.print(now.unixtime());
  Serial.print(",");
  char buffer[23];
  sprintf(buffer, "%d-%02d-%02dT%02d:%02d:%02d,", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  Serial.print(buffer);
  Serial.print(timeSymbol);
  Serial.print(",");
  String serializedTemp = String(temp, 2);
  Serial.print(serializedTemp);
  Serial.print(",");
  String serializedAvgTemp = String(avgTemp, 2);
  Serial.print(serializedAvgTemp);
  Serial.print(",");
  Serial.print(relayState);
  Serial.print("\n");
}

