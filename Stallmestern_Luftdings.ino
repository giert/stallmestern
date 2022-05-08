#include <TimeLib.h>
#include <DS1307RTC.h>
#include <SPI.h>
#include <SD.h>
#include "sgp30.h"
#include "SCD30.h"

#define FILE_NAME "ut.csv"

#define INTERVAL_MS 6000
//#define INTERVAL_MS 60000
#define DUST_PIN 8
#define LOUDNESS_PIN 0

#define RED 1
#define GREEN 1
#define BLUE 1
#define YELLOW 1
#define WHITE 1

unsigned long prevMeasureMillis = 0;
unsigned long lowpulseoccupancy = 0;

void setup() {
  setupLED();
  setupSerial();
  setupStorage();
  setupFile();
  
  setupDust();
  setupLoudness();
  setupAir();

  Serial.println(F("Stallmestern Luftdings startet og begynner målinger"));
}

void loop() {
  setRunning(GREEN);
  lowpulseoccupancy += pulseIn(DUST_PIN, LOW);

  if (millis() - prevMeasureMillis > INTERVAL_MS) {
    setBlocking(RED);    
    writeMeasurement(readDust() + readLoudness() + readAir());
    prevMeasureMillis = millis();
  }
}

void setupLED(){
  pinMode(LED_BUILTIN, OUTPUT);
  setRunning(YELLOW);
  delay(200);
}

void setupSerial() {
  setBlocking(WHITE);
  Wire.begin();
  Serial.begin(9600);
  while (!Serial); // wait for serial
  delay(200);
  Serial.println(F("Stallmestern Luftdings starter opp"));
}

void setupStorage() {
  setBlocking(BLUE);
  pinMode(SS, OUTPUT);
  if (!SD.begin(SS)) {
    Serial.println(F("Error : SD failure, push the reset button"));
    while (!SD.begin(SS));
  }
}

void setupFile() {
  setBlocking(RED);
  if (!createFile()) {
    Serial.println((String) F("Error : could not open ") + FILE_NAME);
    while (!createFile());
  }
}

bool createFile(){
  File file = SD.open(FILE_NAME, FILE_WRITE);
  if (file) {
    file.println((String) F("Time, Dust, Relative loudness, CO2 (ppm), Temp (C), Humidity (%)"));
    file.close();
    return true;
  }
  return false;
}

void setupDust() {
  pinMode(DUST_PIN, INPUT);
  Serial.println(F("Støvsensor startet"));
}

void setupLoudness() {
  Serial.println(F("Støysensor startet"));
}

void setupAir() {
  scd30.initialize();
  Serial.println(F("Luftkvalitetsmåler startet"));
}

String readDust() {
  const float ratio = lowpulseoccupancy / (INTERVAL_MS * 10.0); // Integer percentage 0=>100
  const float concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve

  lowpulseoccupancy = 0;

  return commaValue((String) concentration);
}

String readLoudness() {
  return commaValue((String) analogRead(LOUDNESS_PIN));
}

String readAir() {
  float results[3] = {0};
  scd30.getCarbonDioxideConcentration(results);
  return commaValue((String) results[0]) + commaValue((String) results[1]) + commaValue((String) results[2]);
}

String commaValue(String value){
  return ',' + value;
}

// 22/03/16-17:29:42
String getTime() {
  tmElements_t tm;
  if (RTC.read(tm)) {
    return (String) tmYearToCalendar(tm.Year) +
           '/' +
           tm.Month +
           '/' +
           tm.Day +
           ' ' +
           tm.Hour +
           ':' +
           tm.Minute +
           ':' +
           tm.Second;
  } else {
    if (RTC.chipPresent()) {
      Serial.println(F("Error : RTC not running"));
    } else {
      Serial.println(F("Error : RTC not detected"));
    }
    return (String) millis(); // fallback: millis since start
  }
}

void writeMeasurement(String measurements) {
  if (!writeFile(getTime() + measurements)) {
    Serial.println((String) F("Error : could not open ") + FILE_NAME);
    while(!writeFile(getTime() + measurements));
  }
}

bool writeFile(String text){
  File file = SD.open(FILE_NAME, FILE_WRITE);
  if (file) {
    file.println(text);
    file.close();
    Serial.println(text);
    return true;
  }
  return false;
}

void setRunning(int color){
    digitalWrite(LED_BUILTIN, HIGH);
}

void setBlocking(int color){
    digitalWrite(LED_BUILTIN, LOW);
}
