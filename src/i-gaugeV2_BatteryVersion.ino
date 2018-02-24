//library
#include <OneWire.h>
#include <DallasTemperature.h>  // DS18B20 library. By : Miles Burton, Tim Newsome, and Guil Barros
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPS++.h>
#include <Adafruit_ADS1015.h>
#include <Narcoleptic.h>

//Component Initialization
String ID = "BOGOR02";

//LCD 16X2 I2C
#define I2C_ADDR    0x3f //0x3f //<<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

//D18B20
#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress DS18B20;

//ADS1115
Adafruit_ADS1115 ads(0x48);
#define arus 0
#define tegangan 1
#define pressure 3

//RTC
RTC_DS1307 rtc;
DateTime nows;

//GPS
TinyGPSPlus gps;

//SD CARD
#define SSpin 53
File file;
char str[13];
String filename, nama;

//define pin RGB LED
#define GPower 31
#define BPower 29
#define RState 27
#define GState 25
#define BState 23

//icon
byte water[8] = {
  0x04, 0x04, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E, 0x00,
};
byte pipe[8] = {
  0x0E, 0x04, 0x04, 0x1F, 0x1F, 0x03, 0x00, 0x03,
};
byte smile[8] = {
  0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x11, 0x0E, 0x00,
};
byte clocks1[8] = {
  0x00, 0x00, 0x00, 0x03, 0x0C, 0x09, 0x11, 0x13
};
byte clocks2[8] = {
  0x00, 0x00, 0x00, 0x18, 0x06, 0x02, 0x01, 0x01,
};
byte clocks3[8] = {
  0x10, 0x10, 0x08, 0x0C, 0x03, 0x00, 0x00, 0x00,
};
byte clocks4[8] = {
  0x01, 0x01, 0x02, 0x06, 0x18, 0x00, 0x00, 0x00,
};

//GLOBAL VARIABLE
char g, sdcard[25];
byte a, b, c, bulan, hari, jam, menit, detik;
byte error = 0;
int iterasi = 0;
int index = 0;
int i, tahun, kode;
int mVperAmp = 185; //185 for 5A, 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500;
unsigned int interval; //menit
unsigned int burst; //second
unsigned long reads = 0; //pressure
unsigned long reads0 = 0; //volt & arus
unsigned long lastSendTime, start, waktu;;
float offset = 0;
float flat = -98.76543;
float flon = 789.1234;
float hdop;
float voltase = 0.00;
float ampere = 0.00;
float tekanan, suhu;
String result, y, source;
String teks, operators;
String network, APN, USER, PWD, sms, kuota, noHP;


void setup() {
  Serial.begin(9600);  // Serial USB
  Serial1.begin(9600);  // SIM800L
  //Serial2.begin(9600);  // GPS U BLOX
  Serial.println(F("I-GAUGE"));

  //LCD init
  Serial.println(F("INIT LCD"));
  lcd.begin(16, 2);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.createChar(0, water);
  lcd.createChar(1, pipe);
  lcd.createChar(2, clocks1);
  lcd.createChar(3, clocks2);
  lcd.createChar(4, clocks3);
  lcd.createChar(5, clocks4);
  lcd.createChar(6, smile);

  //LED status init
  pinMode(RState, OUTPUT);  // Red
  pinMode(GState, OUTPUT);  // Green
  pinMode(GState, OUTPUT);  // Blue

  /*   //LED supply init
     pinMode(GPower, OUTPUT);  // Green
     pinMode(BPower, OUTPUT);  // Blue
  */
  for (i = 23; i <= 31; i++) {
    digitalWrite(i, LOW);
  }

  //WELCOME SCREEN
  Serial.println(F("WELCOME"));
  for (i = 0; i <= 2; i++) {
    Serial.println(F("I-GAUGE PDAM BOGOR"));
    Serial.flush();
    //digitalWrite(RState - 2 * i, HIGH);
    lcd.setCursor(2, 0);
    lcd.print(F("* I-GAUGE"));
    lcd.write(byte(1));
    lcd.print(F(" *"));
    lcd.setCursor(1, 1);
    lcd.write(byte(0));
    lcd.print(F(" PDAM BOGOR "));
    lcd.write(byte(0));
    Narcoleptic.delay(2000);
    //digitalWrite(RState - 2 * i, LOW);
  }

  //INIT ADS1115
  Serial.println(F("INIT ADS"));
  Serial.flush();
  // disable ADC internal
  ADCSRA = 0;
  //power_all_disable ();
  ads.begin();

  ledOff();
  lcd.clear();
  //INISIALISASI RTC
  if (! rtc.begin() || ! rtc.isrunning()) {
    lcd.setCursor(0, 0); lcd.print(F("  RTC ERROR!!!")); //Please run the SetTime
    lcd.setCursor(0, 1); lcd.print(F("   CONTACT CS"));
    Serial.println(F("RTC ERROR!!!"));
    Serial.flush();
    while (1) {
      digitalWrite(RState, HIGH);  Narcoleptic.delay(500); //LED RED
      digitalWrite(RState, LOW);   Narcoleptic.delay(500);
    }
  }

  //GET TIME FROM RTC
  for (i = 0; i < 2; i++) {
    //digitalWrite(GState, HIGH); //GREEN
    nows = rtc.now();
    lcd.setCursor(4, 1);  lcd2digits(nows.hour());
    lcd.write(':');       lcd2digits(nows.minute());
    lcd.write(':');       lcd2digits(nows.second());
    lcd.setCursor(3, 0);  lcd2digits(nows.day());
    lcd.write('/');       lcd2digits(nows.month());
    lcd.write('/');       lcd.print(nows.year());
    Narcoleptic.delay(1000);
    //digitalWrite(GState, LOW);
    //Narcoleptic.delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);   lcd.write(byte(2));
  lcd.setCursor(1, 0);   lcd.write(byte(3));
  lcd.setCursor(0, 1);   lcd.write(byte(4));
  lcd.setCursor(1, 1);   lcd.write(byte(5));
  lcd.setCursor(14, 0);  lcd.write(byte(2));
  lcd.setCursor(15, 0);  lcd.write(byte(3));
  lcd.setCursor(14, 1);  lcd.write(byte(4));
  lcd.setCursor(15, 1);  lcd.write(byte(5));
  lcd.setCursor(6, 0);   lcd.print(F("RTC!"));
  lcd.setCursor(5, 1);   lcd.print(F("READY"));
  lcd.write(byte(6));
  Serial.println(F("RTC OK!!!"));
  Serial.flush();
  //RTC OK - LED CYAN
  //digitalWrite(GState, HIGH); //STATUS OK = GREEN
  //digitalWrite(GState, HIGH); //BLUE
  //delay(2000);

  //INIT SD CARD
  ledOff();
  pinMode(SSpin, OUTPUT); //SS MEGA 53, UNO 10
  digitalWrite(SSpin, HIGH);
  //digitalWrite(GState, HIGH); //GREEN
  //delay(500);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(F("- SD CARD INIT -"));
  lcd.setCursor(0, 1);

  if (!SD.begin(SSpin)) { //SD CARD ERROR
    lcd.print(F("SD CARD ERROR!!!"));
    Serial.println(F("SD CARD ERROR!!!"));
    digitalWrite(GState, LOW);
    digitalWrite(RState, HIGH); //LED RED
    while (1) {}
  }

  lcd.setCursor(0, 1);
  lcd.print(F("- SD CARD OK!! -"));
  Serial.println(F("SD CARD OK!!!"));
  Serial.flush();
  //digitalWrite(GState, LOW);
  //digitalWrite(RState, HIGH); // RED
  //digitalWrite(GState, HIGH); // BLUE => PURPLE
  Narcoleptic.delay(2000);

  //INISIALISASI DS18B20
  lcd.clear();
  //ledOff();
  sensors.begin();  //DS18B20
  sensors.getAddress(DS18B20, 0);
  sensors.setResolution(DS18B20, 9); //sensorDeviceAddress, SENSOR_RESOLUTION
  //digitalWrite(GState, HIGH); //STATUS OK = GREEN
  if ( !sensors.getAddress(DS18B20, 0)) {
    lcd.setCursor(0, 0); lcd.print(F("  TEMPERATURE"));
    lcd.setCursor(0, 1); lcd.print(F("  SENSOR ERROR"));
    Serial.println(F("TEMP SENSOR ERROR!!!"));
    Serial.flush();
    //digitalWrite(RState, HIGH); //STATUS RED + GREEN = YELLOW
  }
  else {
    lcd.setCursor(0, 0); lcd.print(F("  TEMPERATURE"));
    lcd.setCursor(0, 1); lcd.print(F("   SENSOR OK!"));
    Serial.println(F("TEMP SENSOR OK!!!"));
    Serial.flush();
  }

  Narcoleptic.delay(2000);
  //ledOff();

  //AMBIL INTERVAL PENGUKURAN
  //digitalWrite(GState, HIGH); //GREEN
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("GET CONFIG.TXT"));
  lcd.setCursor(0, 1);

  //GETTING DATA INTERVAL & PHONE NUMBER
  //ledOff();
  configs();

  //TIME INTERVAL
  //digitalWrite(GState, HIGH); //GREEN
  Serial.flush();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("TIME INTERVAL"));
  lcd.setCursor(0, 1);
  lcd.print(interval);
  Serial.print(F("TIME INTERVAL = "));
  Serial.print(interval);
  if (interval == 1) {
    lcd.print(F(" MINUTE"));
    Serial.println(F(" MINUTE"));
  }
  else {
    lcd.print(F(" MINUTES"));
    Serial.println(F(" MINUTES"));
  }
  Serial.flush();
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW); //GREEN
  Narcoleptic.delay(500);
  //digitalWrite(GState, HIGH); //GREEN
  lcd.clear();

  //BURST INTERVAL
  lcd.print(F("BURST INTERVAL"));
  Serial.print(F("BURST INTERVAL = "));
  lcd.setCursor(0, 1);
  lcd.print(burst);
  Serial.print(burst);
  if (burst == 1) {
    lcd.print(F(" SECOND"));
    Serial.println(F(" SECOND"));
  }
  else {
    lcd.print(F(" SECONDS"));
    Serial.println(F(" SECONDS"));
  }
  Serial.flush();
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW); //GREEN
  Narcoleptic.delay(500);
  //digitalWrite(GState, HIGH); //GREEN

  //PHONE NUMBER
  lcd.clear();
  lcd.print(F("PHONE NUMBER"));
  lcd.setCursor(0, 1);
  lcd.print(noHP);
  Serial.print(F("No HP = "));
  Serial.println(noHP);
  Serial.flush();
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW); //GREEN
  Narcoleptic.delay(500);
  //digitalWrite(GState, HIGH); //GREEN

  //INIT SIM800L & SEND SMS
  //ledOff();
  lcd.clear();
  lcd.print(F("INIT GSM MODULE"));
  Serial.println(F("INITIALIZATION SIM800L..."));
  Serial.flush();
  Serial1.flush();

  //delay for go to sim800l
  for (i = 0; i < 10; i++) {
    lcd.setCursor(i, 1);
    lcd.print(F("*"));
    Serial.println(i + 1);
    Serial.flush();
    //digitalWrite(RState, LOW); // GREEN
    //digitalWrite(GState, LOW); // BLUE=> CYAN
    Narcoleptic.delay(500);
    //digitalWrite(RState, HIGH);
    //digitalWrite(GState, HIGH);
    Narcoleptic.delay(500);
  }
  //ledOff();
  sim800l();
  lcd.clear();
  lcd.print(F("CHECK BALANCE"));
  cekkuota();
  lcd.setCursor(0, 1);
  lcd.print(F("FINISH!!!"));

  /*  //INIT GPS
    ledOff();
    digitalWrite(RState, LOW);//RED
    Narcoleptic.delay(1000);
    digitalWrite(GState, LOW);//MAGENTA
    Narcoleptic.delay(1000);
    digitalWrite(RState, HIGH);//BLUE
    Narcoleptic.delay(1000);

    //Serial.println(F("INITIALIZATION GPS..."));
    Serial.flush();
    Serial2.flush();
    //GPS_ON();
  */
  Narcoleptic.delay(1000);

  //TURN OFF ALL LED
  //ledOff();
  //digitalWrite(RState, HIGH);
  //digitalWrite(GState, HIGH);
  //digitalWrite(BState, HIGH);
  bersihdata();
  Serial.println(F("I-GAUGE READY TO RECORD DATA"));
  Serial.flush();
  lcd.clear();


  //SET WAKTU PENGAMBILAN DATA
  nows = rtc.now();
  setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());

  //AMBIL DATA
  ambil();
  Alarm.timerRepeat(interval * 60, ambil);
  Alarm.alarmRepeat(5, 0, 0, cekkuota); // 5:00am every day
}

void loop() {
  Alarm.delay(0);
}

void ledOff() {
  digitalWrite(RState, LOW);
  digitalWrite(GState, LOW);
  digitalWrite(GState, LOW);
}

void bersihdata() {
  tahun = '0'; bulan = '0'; hari  = '0'; jam = '0'; menit = '0'; detik = '0';
  reads = '0';  reads0 = '0';  voltase = '0';  ampere = '0'; tekanan = '0'; suhu = '0';
  error = '0'; iterasi = '0'; index = '0'; y = "";  i = '0';
}

void dateTime(uint16_t* date, uint16_t* time) {
  // call back for file timestamps
  DateTime nows = rtc.now();
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(nows.year(), nows.month(), nows.day());
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(nows.hour(), nows.minute(), nows.second());
}

void lcd2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void save2digits(int number) {
  if (number >= 0 && number < 10) {
    file.print('0');
  }
  file.print(number);
}

void GPS_ON() {
  //INISIALISASI GPS - interval TUNGGU MENCARI SATELIT
  lcd.clear();
  lcd.print(F("--  GPS INIT  --"));
  lcd.setCursor(0, 1);
  lcd.print(F(" Waiting Signal"));
  Serial.flush();
  Serial1.flush();
  Serial2.flush();
  Serial.println(F("Waiting Signal"));
  Narcoleptic.delay(1000);
  Serial.flush();
  Serial1.flush();
  Serial2.flush();
  //GET COORDINATES
  for (i = 0; i < 60; i++) { //180
    displayInfo();
    if (float(hdop) > 0 && float(hdop) <= 2.0) break;
  }

  lcd.clear();
  //ledOff();

  //DISPLAY LONG & LAT
  digitalWrite(GState, HIGH);
  lcd.print(F("LONGITUDE"));
  Serial.flush();
  Serial1.flush();
  Serial2.flush();
  Narcoleptic.delay(1000);
  lcd.setCursor(0, 1);
  if (flon < 0)
    lcd.print("W ");
  else
    lcd.print("E ");
  i = int(abs(flon));
  lcd.print(i); //DERAJAT
  lcd.print(char(223));
  tekanan = ((abs(flon) - float(i)) * 60.0000000); //MENIT
  lcd.print(int(tekanan));
  lcd.print("'");
  tekanan = (tekanan - float(int(tekanan))) * 60.00; //DETIK
  lcd.print(tekanan);
  lcd.print("\"");
  Narcoleptic.delay(1000);

  lcd.clear();
  lcd.print(F("LATITUDE"));
  Serial.println(" ");
  Serial.print(F("LONGITUDE = "));
  Serial.println(flon, 4);
  Serial.print(F("LATITUDE = "));
  Serial.println(flat, 4);
  Serial.flush();
  Serial2.flush();
  lcd.setCursor(0, 1);
  if (flat < 0)
    lcd.print("S ");
  else
    lcd.print("N ");
  i = int(abs(flat));
  lcd.print(i); //DERAJAT
  lcd.print(char(223));
  tekanan = ((abs(flat) - float(i)) * 60.0000000); //MENIT
  lcd.print(int(tekanan));
  lcd.print("'");
  tekanan = (tekanan - float(int(tekanan))) * 60.00; //DETIK
  lcd.print(tekanan);
  lcd.print("\"");
  Narcoleptic.delay(1000);
  lcd.clear();
  i = '0';
  tekanan = '0';
}

void displayInfo() {
  digitalWrite(GState, HIGH); //STATUS BLUE
  start = millis();
  do   {
    while (Serial2.available()) {
      g = Serial2.read();
      gps.encode(g);
      Serial.print(g);
    }
  }
  while (millis() - start < 1000);
  Serial.flush();
  Serial2.flush();
  Narcoleptic.delay(1000);
  digitalWrite(GState, LOW);
  if (gps.location.isUpdated())  {
    flat = gps.location.lat();
    flon = gps.location.lng();
    hdop = float(gps.hdop.value()) / 100.00;
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(F("- GPS DETECTED -"));
    Serial.println(F("- GPS DETECTED -"));
  }
  if (gps.charsProcessed() < 10)  {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(F("NO GPS DETECTED!"));
    Serial.println(F("NO GPS DETECTED!"));
    //ledOff();
    digitalWrite(RState, HIGH); //RED
    Narcoleptic.delay(1000);
  }
  Serial.flush();
  Serial2.flush();
  Narcoleptic.delay(1000);//
}

void ConnectAT(String cmd, int d) {
  i = 0;
  while (1) {
    Serial1.println(cmd);
    while (Serial1.available()) {
      if (Serial1.find("OK"))
        i = 8;
    }
    delay(d);
    if (i > 5) {
      break;
    }
    i++;
  }
  if (i == 8) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("GSM MODULE OK!!");
    Serial.println("OK");
    Serial.flush();
    Serial1.flush();
    result = "OK";
    Narcoleptic.delay(1000);
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("GSM MODULE ERROR");
    Serial.println("SIM800L ERROR");
    Serial.flush();
    Serial1.flush();
    if (a == 5) {
      ledOff();
      digitalWrite(RState, HIGH);
      digitalWrite(GState, HIGH);
      while (1) {}
    }
  }
}

void ceksim() {
cops:
  filename = "";
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);
  Serial.println(F("AT+COPS?"));
  Serial1.println(F("AT+COPS?"));
  delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+COPS:")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
        Serial.print(g);
      }
    }
  }
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);

  a = filename.indexOf('"');
  b = filename.indexOf('"', a + 1);
  y = filename.substring(a + 1, b);
  if (y == "51089") y = "THREE";

  operators = y;
  //option if not register at network
  if (operators == "")  {
    goto cops;
  }
  Serial.print(F("OPERATOR="));
  lcd.print(operators);
  Serial.println(operators);
  y = "";
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(1000);
}

void sinyal() {
signal:
  filename = "";
  //Serial.println(F("AT+CSQ"));
  //Serial.print(F("Signal Quality="));
  Serial1.println(F("AT+CSQ"));
  delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+CSQ: ")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
      }
    }
  }
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);

  a = (filename.substring(0, filename.indexOf(','))).toInt();
  Serial.print(a);
  Serial.print(" ");
  if (a < 10) {
    lcd.print(F("POOR"));
    Serial.println(F("POOR"));
  }
  if (a > 9 && a < 15) {
    lcd.print(F("FAIR"));
    Serial.println(F("FAIR"));
  }
  if (a > 14 && a < 20) {
    lcd.print(F("GOOD"));
    Serial.println(F("GOOD"));
  }
  if (a > 19 && a <= 31) {
    lcd.print(F("EXCELLENT"));
    Serial.println(F("EXCELLENT"));
  }
  if (a == 99) {
    lcd.print(F("UNKNOWN"));
    Serial.println(F("UNKNOWN"));
    goto signal;
  }
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(2000);
}

void sim800l() { //udah fix
  lcd.clear();
  lcd.print(F("CHECK GSM MODULE"));
  Serial.println(F("CHECK SIM800L"));
  Serial.flush();
  Serial1.flush();
  //digitalWrite(GState, HIGH);
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW);
  //Narcoleptic.delay(500);

  //Serial.println(F("AT+CSCLK=0"));
  Serial1.println(F("AT+CSCLK=0"));
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  //digitalWrite(GState, HIGH);
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW);
  Narcoleptic.delay(500);

  lcd.setCursor(0, 1);
  for (a = 0; a < 6; a++) {
    ConnectAT(F("AT"), 100);
  }
  Serial.flush();
  Serial1.flush();
  //digitalWrite(GState, HIGH);
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW);
  Narcoleptic.delay(500);

  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("OPS="));
  ceksim();
  //digitalWrite(GState, HIGH);
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW);
  Narcoleptic.delay(500);

  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("SIGNAL="));
  sinyal();
  //digitalWrite(GState, HIGH);
  Narcoleptic.delay(500);
  //digitalWrite(GState, LOW);
  Narcoleptic.delay(500);

  //Serial.println(F("AT+CMGD=1,4"));
  Serial1.println(F("AT+CMGD=1,4"));
  bacaserial(200);
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);
  lcd.clear();
  lcd.print(F("SEND SMS"));
  Serial.println(F("SEND SMS"));
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("SMS "));
  Serial.flush();
  Serial1.flush();
  //Serial.print(F("AT+CMGF=1 "));
  Serial1.println(F("AT+CMGF=1"));
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  lcd.print(F("."));
  Narcoleptic.delay(500);
  //Serial.print(F("AT+CSCS=\"GSM\" "));
  Serial1.println(F("AT+CSCS=\"GSM\""));
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  lcd.print(F("."));
  Narcoleptic.delay(500);
  y = "AT+CMGS=\"" + noHP + "\"";
  //Serial.println(y);
  Serial1.println(y);
  while (Serial1.find(">") == false) {
  }
  lcd.print(F("."));
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);
  start = millis();
  y = "I-GAUGE ID " + ID + " ready send data to server";
  //Serial.println(y);
  Serial1.println(y);
  lcd.print(F("."));
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(1000);
  Serial1.write(26);
  Serial.flush();
  Serial1.flush();
  //WAITING OK
  while (Serial1.available() > 0) {
    while (Serial1.find("OK") == false) { //+CMGS
    }
  }
  //ledOff();
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("SMS SENT"));
  //digitalWrite(GState, HIGH); //BLUE
  Narcoleptic.delay(2000);
  //digitalWrite(GState, LOW);
  Serial.println(F("sms has been sent"));
  Serial.flush();
  Serial1.flush();
  //SIM800L sleep mode
  Serial.println(F("AT+CSCLK=2"));
  Serial1.println(F("AT+CSCLK=2"));
  Narcoleptic.delay(500);
}

void configs() {
  a = 0;
  file = SD.open(F("config.txt"));
  lcd.setCursor(0, 1);
  if (file) {
    while (file.available()) {
      g = file.read();
      sdcard[a++] = g;
    }
  }
  else  {
    lcd.print(F("ERROR READING"));
    Serial.println(F("ERROR READING"));
    //ledOff();
    while (1) {
      digitalWrite(RState, HIGH); //YELLOW RED
      digitalWrite(GState, HIGH);
      Narcoleptic.delay(1000);
      digitalWrite(GState, LOW);
      Narcoleptic.delay(1000);
    }
  }
  file.close();

  filename = String(sdcard);
  Serial.println(filename);
  Serial.flush();
  Serial1.flush();
  for ( a = 0; a < sizeof(sdcard); a++) {
    sdcard[a] = (char)0;
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(200);
    //digitalWrite(GState, LOW);
    //Narcoleptic.delay(100);
  }

  lcd.print(F("FINISH..."));
  a = filename.indexOf("\r");
  interval = filename.substring(0, a).toInt();
  b = filename.indexOf("\r", a + 1);
  burst = filename.substring(a + 1, b).toInt();
  a = filename.indexOf("\r", b + 1);
  noHP = "";//+62
  noHP = filename.substring(b + 2, a);
  filename = '0';
  Narcoleptic.delay(1000);
}

void simpanconfigs() {
  Serial.print("Cek apa ada file config.txt     ");
  Serial.println(SD.exists("/config.txt"));
  Narcoleptic.delay(1000);
  Serial.print("hapus config.txt    ");
  Serial.println(SD.remove("/config.txt"));
  Narcoleptic.delay(1000);
  // set date time callback function
  SdFile::dateTimeCallback(dateTime);
  //ISI dengan data baru
  file = SD.open("/config.txt", FILE_WRITE);
  Serial.println(file);
  file.println(interval);
  file.println(burst);
  file.println(noHP);
  file.close();
  Serial.println(F("config has been changed"));

}

void gpsdata() {
  start = millis();
  do   {
    while (Serial2.available()) {
      g = Serial2.read();
      gps.encode(g);
      Serial.print(g);
    }
  }
  while (millis() - start < 1000);
  Serial.flush();
  Serial2.flush();
  if (gps.location.isUpdated())  {
    flat = gps.location.lat();
    flon = gps.location.lng();
    hdop = float(gps.hdop.value()) / 100.00;
  }
  if (gps.charsProcessed() < 10)  {
  }
}

void ambil() {
  Alarm.delay(0);
  //ledOff();
  //digitalWrite(GState, HIGH);//GREEN
  nows = rtc.now();
  waktu = nows.unixtime();
  //Serial.println(waktu);
  lcd.clear();
  lcd.print(F("--  GET DATA  --"));
  //WAKE UP SIM800L
  //Serial.println(F("AT+CSCLK=0"));
  Serial1.println(F("AT+CSCLK=0"));
  Serial1.flush();
  Narcoleptic.delay(200);
  //Serial.println(F("AT+CSCLK=0"));
  Serial1.println(F("AT+CSCLK=0"));
  Serial1.flush();
  bacaserial(200);

  //GET TIME
  nows = rtc.now();
  tahun = nows.year();
  bulan = nows.month();
  hari = nows.day();
  jam = nows.hour();
  menit = nows.minute();
  detik = nows.second();

  lcd.clear();

  /* //ambil data GPS
    for (i = 0; i < 2; i++) {
     gpsdata();
    }*/

  //ambil data tekanan, arus, dan voltase
  for (i = 0; i < burst; i++) {
    //digitalWrite(GState, HIGH);
    reads += ads.readADC_SingleEnded(pressure); //tekanan
    reads0 += ads.readADC_SingleEnded(arus); //arus
    nows = rtc.now();
    lcd.setCursor(3, 0);
    lcd.print(nows.year());   lcd.write('/');
    lcd2digits(nows.month()); lcd.write('/');
    lcd2digits(nows.day());
    lcd.setCursor(4, 1);
    lcd2digits(nows.hour());  lcd.write(':');
    lcd2digits(nows.minute());  lcd.write(':');
    lcd2digits(nows.second());
    Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
  }

  //digitalWrite(GState, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("VOLTAGE = "));
  lcd.setCursor(0, 1);
  lcd.print(F("CURRENT = "));


  // KONVERSI
  voltase = ((float)reads / (float)burst) * 0.1875 / 1000.0000; // nilai voltase dari nilai DN
  tekanan = (300.00 * voltase - 150.00) * 0.01 + float(offset);
  voltase = (((float)reads0 / (float)burst) * 0.1875);
  voltase = reads0 * 0.1875;
  ampere = ((voltase - ACSoffset) / mVperAmp);
  voltase = voltase / 1000.00;
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);
  voltase = float(ads.readADC_SingleEnded(tegangan)) * 0.1875 / 1000.0000 * 5.325443787;
  if (voltase >= 8.00) {
    //digitalWrite(BPower, LOW);
    //digitalWrite(GPower, HIGH);
    source = "PLN";
  }
  if (voltase < 8.00) {
    //digitalWrite(BPower, HIGH);
    //digitalWrite(GPower, LOW);
    source = "BATTERY";
  }

  //tampilkan data ke LCD
  lcd.setCursor(10, 0);
  lcd.print(voltase, 2);
  lcd.setCursor(15, 0);
  lcd.print(F("V"));
  lcd.setCursor(10, 1);
  lcd.print(ampere, 2);
  lcd.setCursor(15, 1);
  lcd.print(F("A"));
  Narcoleptic.delay(1000);
  //digitalWrite(GState, LOW);
  //Narcoleptic.delay(1000);
  lcd.clear();
  //digitalWrite(GState, HIGH);
  lcd.print(F("PRES = "));
  lcd.print(tekanan, 2);
  lcd.setCursor(13, 0);
  lcd.print(F("bar"));
  lcd.setCursor(0, 1);
  lcd.print(F("TEMP = "));
  lcd.print(suhu, 2);
  lcd.setCursor(13, 1);
  lcd.print(char(223));
  lcd.setCursor(14, 1);
  lcd.print(F("C"));
  //Narcoleptic.delay(500);
  //digitalWrite(GState, LOW);
  //Narcoleptic.delay(500);


  //kirim ke server
  Serial.println(F("SEND DATA TO SERVER"));
  server(1);

  /*//tampilkan ke serial
    Serial.print(tahun);
    Serial.write('-');
    if (bulan < 10) {
    Serial.print("0"); Serial.print(bulan);
    }
    if (bulan >= 10) {
    Serial.print(bulan);
    }
    Serial.print("-");
    if (hari < 10) {
    Serial.print("0"); Serial.print(hari);
    }
    if (hari >= 10) {
    Serial.print(hari);
    }
    Serial.print(" ");
    if (jam < 10) {
    Serial.print("0"); Serial.print(jam);
    }
    if (jam >= 10) {
    Serial.print(jam);
    }
    Serial.print(":");
    if (menit < 10) {
    Serial.print("0"); Serial.print(menit);
    }
    if (menit >= 10) {
    Serial.print(menit);
    }
    Serial.print(":");
    if (detik < 10) {
    Serial.print("0"); Serial.print(detik);
    }
    if (detik >= 10) {
    Serial.print(detik);
    }

    Serial.write('|');
    Serial.print(flon, 4); Serial.print('|');
    Serial.print(flat, 4); Serial.print('|');
    Serial.print(tekanan, 2); Serial.print('|');
    Serial.print(suhu, 2);    Serial.print('|');
    Serial.print(voltase, 2); Serial.print('|');
    Serial.print(ampere, 2);  Serial.print('|');
    Serial.print(source);      Serial.print('|');
    Serial.print(burst);      Serial.print('|');
    Serial.print(interval);   Serial.print('|');
    Serial.print(noHP);       Serial.print('|');
    Serial.print(operators);  Serial.print('|');
    Serial.print(kode);       Serial.print('|');
    Serial.print(network);  Serial.println('|');
    Serial.flush();
  */
  //SAVE DATA
  //digitalWrite(GState, HIGH);
  filename = "";
  filename = String(tahun);
  if (bulan < 10) {
    filename += "0" + String(bulan);
  }
  if (bulan >= 10) {
    filename += String(bulan);
  }
  if (hari < 10) {
    filename += "0" + String(hari);
  }
  if (hari >= 10) {
    filename += String(hari);
  }
  filename += ".ab";

  filename.toCharArray(str, 13);
  i = SD.exists(str);
  // set date time callback function
  SdFile::dateTimeCallback(dateTime);
  file = SD.open(str, FILE_WRITE);
  if (i == 0) {
    file.print(F("Date (YYYY-MM-DD HH:MM:SS) | "));
    file.print(F("Longitude (DD.DDDDDD°) | Latitude (DD.DDDDDD°) | Pressure (BAR)| Temperature (°C) | "));
    file.print(F("Voltage (V) | Current (AMP) | Source | ID Station | Burst interval (SECOND)| "));
    file.println(F("Phone Number | Operator | Server Code | Network"));
  }

  //simpan data ke SD CARD
  //FORMAT DATA = Date (YYYY-MM-DD HH:MM:SS) | Longitude (DD.DDDDDD°) | Latitude (DD.DDDDDD°) | Pressure (BAR)| Temperature (°C) |
  //              Voltage (V) | Current (AMP) | Source | ID Station | Burst interval (SECOND)|
  //        Data Interval (MINUTE) | Phone Number | Operator | Server Code | Network"));
  //TimeStamp
  file.print(tahun);  file.print('-');
  save2digits(bulan); file.print('-');
  save2digits(hari);  file.print(' ');
  save2digits(jam);   file.print(':');
  save2digits(menit); file.print(':');
  save2digits(detik); file.print('|');
  //longitude
  file.print(flon, 4);    file.print('|');
  //latitude
  file.print(flat, 4);    file.print('|');
  //pressure
  file.print(tekanan, 2);   file.print('|');
  //temperature
  file.print(suhu, 2);    file.print('|');
  //voltage
  file.print(voltase, 2);   file.print('|');
  //current
  file.print(ampere, 2);  file.print('|');
  //source
  file.print(source);      file.print('|');
  //id station
  file.print(ID);      file.print('|');
  //burst
  file.print(burst);      file.print('|');
  //interval
  file.print(interval);
  //phone number
  file.print(noHP);     file.print('|');
  //operator
  file.print(operators);  file.print('|');
  //kode
  file.print(kode); file.print('|');
  //network
  file.print(network);  file.println('|');
  file.flush();
  file.close();

  //bersih variabel
  bersihdata();
  //digitalWrite(GState, LOW);
  Serial.println(F("AT+CSCLK=2"));
  Serial1.println(F("AT+CSCLK=2"));
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);
  //digitalWrite(GState, HIGH);
  while (1) {
    nows = rtc.now();
    start = nows.unixtime() - waktu;
	//Serial.println(start);
	//Serial.flush();

    if (start < ((interval*60)-(1 * 60))) {
      //digitalWrite(RState, HIGH);
      Narcoleptic.delay(8000);
    }
    else {
      //digitalWrite(RState, LOW);
      break;
    }
  }
}

void bacaserial(int wait) {
  start = millis();
  while ((start + wait) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.print(g);
    }
  }
}

void server(byte t) {
  //CHECK GPRS ATTACHED OR NOT
serve:
  filename = "";
  //operators="TELKOMSEL";
  Serial.print(F("AT+CGATT? "));
  Serial1.println(F("AT+CGATT?"));
  Serial.flush();
  Serial1.flush();
  delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+CGATT: ")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
      }
    }
  }
  Serial.println(filename);
  Serial.flush();
  Serial1.flush();
  if (filename.toInt() == 1) {
    //kirim data ke server
    //ledOff();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //Serial.println(F("AT+CIPSHUT"));
    Serial1.println(F("AT+CIPSHUT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //ATUR APN SESUAI DENGAN PROVIDER
    apn(operators);
    //CONNECTION TYPE
    //Serial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    Serial1.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //APN ID
    result = "AT+SAPBR=3,1,\"APN\",\"" + APN + "\"";
    //Serial.println(result);
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //APN USER
    result = "AT+SAPBR=3,1,\"USER\",\"" + USER + "\"";
    //Serial.println(result);
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //APN PASSWORD
    result = "AT+SAPBR=3,1,\"PWD\",\"" + PWD + "\"";
    //Serial.println(result);
    Serial1.println(result);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //OPEN BEARER
    start = millis();
    //Serial.println(F("AT+SAPBR=1,1"));
    Serial1.println(F("AT+SAPBR=1,1"));
    bacaserial(1000);
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //QUERY BEARER
    //Serial.println(F("AT+SAPBR=2,1"));
    Serial1.println(F("AT+SAPBR=2,1"));
    start = millis();
    while (start + 3000 > millis()) {
      while (Serial1.available() > 0) {
        Serial.print(Serial1.read());
        if (Serial1.find("OK")) {
          i = 5;
          break;
        }
      }
      if (i == 5) {
        break;
      }
    }
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //TERMINATE HTTP SERVICE
    //Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);
    //INITIALIZE HTTP SERVICE
    //Serial.println(F("AT+HTTPINIT"));
    Serial1.println(F("AT+HTTPINIT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);
    //SET HTTP PARAMETERS VALUE
    //Serial.println(F("AT+HTTPPARA=\"CID\",1"));
    Serial1.println(F("AT+HTTPPARA=\"CID\",1"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);
    if (t == 1) { // send data measurement to server
      //SET HTTP URL
      Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      bacaserial(1000);
      Serial.flush();
      Serial1.flush();
      //http://www.mantisid.id/api/product/pdam_dt_c.php?="Data":"'2017-11-05 10:00:00', '111.111111', '-6.2222222', '400.33', '34.00', '5.05', '1.66', 'pdam_001', '5', '3'"
      //Formatnya Date, longitude, latitude, pressure, temperature, volt, ampere, source, id, burst interval, data interval
      y = "{\"Data\":\"'";
      y += String(nows.year()) + "-";
      if (bulan < 10) {
        y += "0" + String(bulan) + "-";
      }
      if (bulan >= 10) {
        y += String(bulan) + "-";
      }
      if (hari < 10) {
        y += "0" + String(hari) + " ";
      }
      if (hari >= 10) {
        y += String(hari) + " ";
      }
      if (jam < 10) {
        y += "0" + String(jam) + ":";
      }
      if (jam >= 10) {
        y += String(jam) + ":";
      }
      if (menit < 10) {
        y += "0" + String(menit) + ":";
      }
      if (menit >= 10) {
        y += String(menit) + ":";
      }
      if (detik < 10) {
        y += "0" + String(detik);
      }
      if (detik >= 10) {
        y += String(detik);
      }
      y += "','";
      y += String(flon, 4) + "','";
      y += String(flat, 4) + "','";
      y += String(tekanan, 2) + "','";
      y += String(suhu, 2) + "','";
      y += String(voltase, 2) + "','";
      y += String(ampere, 2) + "','";
      y += String(source) + "','";
      y += String(ID) + "','";
      y += String(burst) + "','";
      y += String(interval) + "'\"}";
    }
    if (t == 2) { // send data kuota to server
      //GET TIME
      nows = rtc.now();
      tahun = nows.year();
      bulan = nows.month();
      hari = nows.day();
      jam = nows.hour();
      menit = nows.minute();
      detik = nows.second();

      //SET HTTP URL
      Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_sim_c.php\""));
      Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_sim_c.php\""));
      bacaserial(1000);
      Serial.flush();
      Serial1.flush();
      //http://www.mantisid.id/api/product/pdam_sim_c.php
      //Formatnya YYY-MM-DD HH:MM:SS, ID, PULSA, KUOTA
      y = "{\"Data\":\"'";
      y += String(nows.year()) + "-";
      if (bulan < 10) {
        y += "0" + String(bulan) + "-";
      }
      if (bulan >= 10) {
        y += String(bulan) + "-";
      }
      if (hari < 10) {
        y += "0" + String(hari) + " ";
      }
      if (hari >= 10) {
        y += String(hari) + " ";
      }
      if (jam < 10) {
        y += "0" + String(jam) + ":";
      }
      if (jam >= 10) {
        y += String(jam) + ":";
      }
      if (menit < 10) {
        y += "0" + String(menit) + ":";
      }
      if (menit >= 10) {
        y += String(menit) + ":";
      }
      if (detik < 10) {
        y += "0" + String(detik);
      }
      if (detik >= 10) {
        y += String(detik);
      }
      y += "','";
      y += String(ID) + "','";
      y += String(sms) + "','";
      y += String(kuota) + "'\"}";

      //simpan data sisa pulsa dan kuota ke dalam sd card
      result = "pulsa.ab";

      result.toCharArray(str, 13);
      // set date time callback function
      SdFile::dateTimeCallback(dateTime);
      file = SD.open(str, FILE_WRITE);
      file.println(y);
      file.flush();
      file.close();
    }
    //SET HTTP DATA FOR SENDING TO SERVER
    result = "AT+HTTPDATA=" + String(y.length() + 1) + ",15000";
    //Serial.println(result);
    Serial1.println(result);
    while (Serial1.available() > 0) {
      while (Serial1.find("DOWNLOAD") == false) {
      }
    }

    //SEND DATA
    Serial.println(y);
    Serial1.println(y);
    Serial.flush();
    Serial1.flush();
    bacaserial(1000);

    //HTTP METHOD ACTION
    filename = "";
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    start = millis();
    //Serial.println(F("AT+HTTPACTION=1"));
    Serial1.println(F("AT+HTTPACTION=1"));
    while (Serial1.available() > 0) {
      while (Serial1.find("OK") == false) {
        if (Serial1.find("ERROR")) {
          goto serve;
        }
      }
    }
    Serial.flush();
    Serial1.flush();
    a = '0';
    b = '0';
    //CHECK KODE HTTPACTION
    while ((start + 20000) > millis()) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
        Serial.print(g);
        a = filename.indexOf(":");
        b = filename.length();
        if (b - a > 12)break;
      }
      if (b - a > 12) break;
    }
    Serial.println();
    Serial.flush();
    Serial1.flush();
    a = '0';
    b = '0';
    Narcoleptic.delay(500);
    //Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //digitalWrite(GState, HIGH);
    //Narcoleptic.delay(500);
    //digitalWrite(GState, LOW);
    Narcoleptic.delay(500);
    //Serial.println(F("AT+SAPBR=0,1"));
    Serial1.println(F("AT+SAPBR=0,1"));
    while (start + 10000 > millis()) {
      while (Serial1.available() > 0) {
        if (Serial1.find("OK")) {
          a = 1;
          break;
        }
      }
      if (a = 1) break;
    }
    a = '0';
    Serial.println("");
    Serial.println(filename);
    Serial.flush();
    Serial1.flush();
    a = filename.indexOf(',');
    b = filename.indexOf(',', a + 1);
    kode = filename.substring(a + 1, b).toInt();
    statuscode(kode);
    Serial.flush();
    Serial1.flush();
  }
  else {
    network = "Error";
    kode = 999;
  }
}

void server1() {   //parsing teks dari NODE
  //Date|longitude|latitude|pressure|temperature|volt|ampere|source|ID|burst interval|data interval
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("Sending Data..."));
  y = "{\"Data\":\"'";
  y += teks;   // DATE
  y += "'\"}"; // DATA INTERVAL
  teks = "";
  Serial.println(y);
  //kirim ke server string y
  senddata();

  file = SD.open(filename, FILE_WRITE);
  //phone number
  file.print(noHP);     file.print('|');
  //operator
  file.print(operators);  file.print('|');
  //kode
  file.print(kode); file.print('|');
  //network
  file.print(network);  file.println('|');
  file.flush();  file.close();
}

void senddata() {
  //ledOff();
  error = 0;
  Serial.flush();
  Serial1.flush();
serve:
  filename = "";
  Narcoleptic.delay(400);
  //Serial.print(F("AT+CGATT? "));
  Serial1.println(F("AT+CGATT?"));
  Narcoleptic.delay(100);
  while (Serial1.available() > 0) {
    if (Serial1.find("+CGATT: ")) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
      }
    }
  }
  Serial.println(filename);
  if (filename.toInt() == 1) {
    //kirim data ke server
    Narcoleptic.delay(400);
    //Serial.println(F("AT+CIPSHUT"));
    Serial1.println(F("AT+CIPSHUT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();

    //ATUR APN SESUAI DENGAN PROVIDER
    apn(operators);
    //CONNECTION TYPE
    //Serial.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    Serial1.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //APN ID
    teks = "AT+SAPBR=3,1,\"APN\",\"" + APN + "\"";
    //Serial.println(teks);
    Serial1.println(teks);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //APN USER
    teks = "AT+SAPBR=3,1,\"USER\",\"" + USER + "\"";
    //Serial.println(teks);
    Serial1.println(teks);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //APN PASSWORD
    teks = "AT+SAPBR=3,1,\"PWD\",\"" + PWD + "\"";
    //Serial.println(teks);
    Serial1.println(teks);
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //OPEN BEARER
    start = millis();
    //Serial.println(F("AT+SAPBR=1,1"));
    Serial1.println(F("AT+SAPBR=1,1"));
    bacaserial(1000);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(400);
    //QUERY BEARER
    //Serial.println(F("AT+SAPBR=2,1"));
    Serial1.println(F("AT+SAPBR=2,1"));
    start = millis();
    while (start + 5000 > millis()) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        if (Serial1.find("OK")) {
          i = 5;
          break;
        }
        Serial.print(g);
      }
      if (i == 5) {
        break;
      }
    }
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);
    //TERMINATE HTTP SERVICE
    //Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(400);
    //INITIALIZE HTTP SERVICE
    //Serial.println(F("AT+HTTPINIT"));
    Serial1.println(F("AT+HTTPINIT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);
    //SET HTTP PARAMETERS VALUE
    //Serial.println(F("AT+HTTPPARA=\"CID\",1"));
    Serial1.println(F("AT+HTTPPARA=\"CID\",1"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Narcoleptic.delay(500);

    //Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
    Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
    bacaserial(1000);
    Serial.flush();
    Serial1.flush();
    //"Data":"'2017-11-05 10:00:00', '111.111111', '-6.2222222', '400.33', '34.00', '5.05', '1.66', 'pdam_001', '5', '3'"
    //Format: Date, longitude, latitude, pressure, temperature, volt, ampere, SOURCE, id, burst interval, data interval

    //SET HTTP DATA FOR SENDING TO SERVER
    teks = "AT+HTTPDATA=" + String(y.length() + 1) + ",15000";

    //Serial.println(teks);
    Serial1.println(teks);
    while (Serial1.available() > 0) {
      while (Serial1.find("DOWNLOAD") == false) {
      }
    }
    //SEND DATA
    //Serial.println(y);
    Serial1.println(y);
    Serial.flush();
    Serial1.flush();
    bacaserial(1000);
    //HTTP METHOD ACTION
    filename = "";
    Narcoleptic.delay(500);
    start = millis();
    //Serial.println(F("AT+HTTPACTION=1"));
    Serial1.println(F("AT+HTTPACTION=1"));
    while (Serial1.available() > 0) {
      while (Serial1.find("OK") == false) {
        if (Serial1.find("ERROR")) {
          goto serve;
        }
      }
    }
    Serial.flush();
    Serial1.flush();
    a = '0';
    b = '0';
    //CHECK KODE HTTPACTION
    while ((start + 20000) > millis()) {
      while (Serial1.available() > 0) {
        g = Serial1.read();
        filename += g;
        Serial.print(g);
        a = filename.indexOf(":");
        b = filename.length();
        if (b - a > 12)break;
      }
      if (b - a > 12) break;
    }
    Serial.println();
    Serial.flush();
    Serial1.flush();
    a = '0';
    b = '0';
    Narcoleptic.delay(1000);
    //Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    //Serial.println(F("AT+SAPBR=0,1"));
    Serial1.println(F("AT+SAPBR=0,1"));
    while (start + 10000 > millis()) {
      while (Serial1.available() > 0) {
        if (Serial1.find("OK")) {
          a = 1;
          break;
        }
      }
      if (a = 1) break;
    }
    a = '0';
    a = filename.indexOf(',');
    b = filename.indexOf(',', a + 1);
    kode = filename.substring(a + 1, b).toInt();
    statuscode(kode);
    Serial.flush();
    Serial1.flush();
  }
  else {
    error++;
    network = "Error";
    kode = 999;
    if (error < 5) {
      error++;
      goto serve;
    }
  }
  error = 0;
  //SIM800L sleep mode
  Serial.print(kode);
  Serial.print(" ");
  Serial.println(network);
  Serial.println(F("AT+CSCLK=2"));
  Serial1.println(F("AT+CSCLK=2"));
  Serial.flush();
  Serial1.flush();
}

void apn(String nama) {
  if (nama == "TELKOMSEL") {
    APN = "Telkomsel";
    USER = "";
    PWD = "";
  }
  if (nama == "INDOSAT") {
    APN = "indosatgprs";
    USER = "indosat";
    PWD = "indosatgprs";
  }
  if (nama == "EXCELCOM") {
    APN = "internet";
    USER = "";
    PWD = "";
  }
  if (nama == "THREE") {
    APN = "3data";
    USER = "3data";
    PWD = "3data";
  }
}

void statuscode(int w) {
  if (w == 100) {
    network = "Continue";
  }
  if (w == 101) {
    network = "Switching Protocols";
  }
  if (w == 200) {
    network = "OK";
  }
  if (w == 201) {
    network = "Created";
  }
  if (w == 202) {
    network = "Accepted";
  }
  if (w == 203) {
    network = "Non-Authoritative Information";
  }
  if (w == 204) {
    network = "No Content";
  }
  if (w == 205) {
    network = "Reset Content";
  }
  if (w == 206) {
    network = "Partial Content";
  }
  if (w == 300) {
    network = "Multiple Choices";
  }
  if (w == 301) {
    network = "Moved Permanently";
  }
  if (w == 302) {
    network = "Found";
  }
  if (w == 303) {
    network = "See Other";
  }
  if (w == 304) {
    network = "Not Modified";
  }
  if (w == 305) {
    network = "Use Proxy";
  }
  if (w == 307) {
    network = "Temporary Redirect";
  }
  if (w == 400) {
    network = "Bad Request";
  }
  if (w == 401) {
    network = "Unauthorized";
  }
  if (w == 402) {
    network = "Payment Required";
  }
  if (w == 403) {
    network = "Forbidden";
  }
  if (w == 404) {
    network = "Not Found";
  }
  if (w == 405) {
    network = "Method Not Allowed";
  }
  if (w == 406) {
    network = "Not Acceptable";
  }
  if (w == 407) {
    network = "Proxy Authentication Required";
  }
  if (w == 408) {
    network = "Request Time-out";
  }
  if (w == 409) {
    network = "Conflict";
  }
  if (w == 410) {
    network = "Gone";
  }
  if (w == 411) {
    network = "Length Required";
  }
  if (w == 412) {
    network = "Precondition Failed";
  }
  if (w == 413) {
    network = "Request Entity Too Large";
  }
  if (w == 414) {
    network = "Request-URI Too Large";
  }
  if (w == 415) {
    network = "Unsupported Media Type";
  }
  if (w == 416) {
    network = "Requested range not satisfiable";
  }
  if (w == 417) {
    network = "Expectation Failed ";
  }
  if (w == 500) {
    network = "Internal Server Error";
  }
  if (w == 501) {
    network = "Not Implemented";
  }
  if (w == 502) {
    network = "Bad Gateway";
  }
  if (w == 503) {
    network = "Service Unavailable";
  }
  if (w == 504) {
    network = "Gateway Time-out";
  }
  if (w == 505) {
    network = "HTTP Version not supported";
  }
  if (w == 600) {
    network = "Not HTTP PDU";
  }
  if (w == 601) {
    network = "Network Error";
  }
  if (w == 602) {
    network = "No memory";
  }
  if (w == 603) {
    network = "DNS Error";
  }
  if (w == 604) {
    network = "Stack Busy";
  }
}

void cekkuota() {
  c = 0;
  Serial.flush();
  Serial1.flush();
  //Serial.println("AT");
  Serial1.println("AT");
  Serial.flush();
  Serial1.flush();
  bacaserial(100);
  Narcoleptic.delay(500);
  //Serial.println("AT+CSCLK=0");
  Serial1.println("AT+CSCLK=0");
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);
  //Serial.println("AT+CSCLK=0");
  Serial1.println("AT+CSCLK=0");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(500);
top:
  if (c == 5)
  {
    sms = "sisa pulsa tidak diketahui";
    kuota = "sisa kuota tidak diketahui";
    goto down;
  }
  sms = "";
  kuota = "";

  start = millis();
  //Serial.println("AT+COPS?");
  Serial1.println("AT+COPS?");
  Serial.flush();
  Serial1.flush();
  while ((start + 1000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      sms += g;

    }
  }

  Serial.flush();
  Serial1.flush();
  a = sms.indexOf(',');
  if (a == 0) {
    goto top;
  }

  //Serial.println("AT+CSQ");
  Serial1.println("AT+CSQ");
  bacaserial(100);
  //Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(400);
  //Serial.println("AT+CUSD=2");
  Serial1.println("AT+CUSD=2");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(400);

  //Serial.println("AT+CUSD=1");
  Serial1.println("AT+CUSD=1");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  Narcoleptic.delay(400);
  sms = "";
  start = millis();
  //Serial.println("AT+CUSD=1,\"*888#\"");
  Serial1.println("AT+CUSD=1,\"*888#\"");
  Serial.flush();
  Serial1.flush();

  while ((start + 10000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      sms += g;
    }
  }
  Serial.flush();
  Serial1.flush();
  a = sms.indexOf(':');
  b = sms.indexOf('"', a + 1);
  i = sms.substring(a + 1, b - 1).toInt();
  Serial.print("+CUSD: ");
  Serial.println(i);
  if (i == 0) {
    c++;
    goto top;
  }
  a = sms.indexOf(':');
  b = sms.indexOf('.', a + 1);
  b = sms.indexOf('.', b + 1);
  b = sms.indexOf('.', b + 1);
  sms = sms.substring(a + 5, b);

  Narcoleptic.delay(100);
  start = millis();
  //Serial.println("AT+CUSD=1,\"3\"");
  Serial1.println("AT+CUSD=1,\"3\"");
  while ((start + 5000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      kuota += g;
    }
  }
  Serial.flush();
  Serial1.flush();
  a = kuota.indexOf(':');
  b = kuota.indexOf('"', a + 1);
  i = kuota.substring(a + 1, b - 1).toInt();
  if (i == 0) {
    goto top;
  }
  Narcoleptic.delay(100);

  kuota = "";
  start = millis();
  //Serial.println("AT+CUSD=1,\"2\"");
  Serial1.println("AT+CUSD=1,\"2\"");
  while ((start + 5000) > millis()) {
    while (Serial1.available() > 0) {
      g = Serial1.read();
      Serial.write(g);
      kuota += g;
    }
  }
  Serial.flush();
  Serial1.flush();
  a = kuota.indexOf(':');
  b = kuota.indexOf('"', a + 1);
  i = kuota.substring(a + 1, b - 1).toInt();
  if (i == 0) {
    goto top;
  }
  a = kuota.indexOf(':');
  b = kuota.indexOf('.', a + 1);
  kuota = kuota.substring(a + 5, b);

down:
  a = 0; b = 0; i = 0;
  Serial.flush();
  Serial1.flush();
  //Serial.println("AT+CUSD=2");
  Serial1.println("AT+CUSD=2");
  bacaserial(200);
  Serial.flush();
  Serial1.flush();
  Serial.println();
  Serial.println(sms);
  Serial.println(kuota);
  server(2); //send kuota to server
}


