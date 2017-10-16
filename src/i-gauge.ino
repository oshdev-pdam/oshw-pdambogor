//library
#include <OneWire.h>
#include <DallasTemperature.h>  // DS18B20 library. By : Miles Burton, Tim Newsome, and Guil Barros
#include <SPI.h>              // KOMUNIKASI SD CARD
#include <SD.h>               // SD CARD
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPS++.h>
#include <Adafruit_ADS1015.h>


//Component Initialization
//inisialisasi LCD 16X2 I2C
#define I2C_ADDR    0x27//0x3F // <<----- Add your address here.  Find it from I2C Scanner
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
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

//ADS1115
Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

//inisialisasi RTC
RTC_DS1307 rtc;
DateTime nows;

//GPS
TinyGPSPlus gps;

//GLOBAL VARIABEL UNTUK SD CARD
File file;
char str[13];
String filename, nama;

//SIM800L
#define PHONE_NUMBER "085958553254"

//water icon
byte water[8]=
{
0x04, 0x04, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E, 0x00,

};
byte pipe[8]=
{
0x0E, 0x04, 0x04, 0x1F, 0x1F, 0x03, 0x00, 0x03,
};
byte smile[8]=
{
0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x11, 0x0E, 0x00,
};
//global variable
char a, next_char, tgl[8];
byte index = 0;
int i;
int mVperAmp = 185;
int ACSoffset = 2500; 
unsigned int waktu; //menit
unsigned int numAvg; //second
long reads = 0;
long reads0 = 0;
long reads1 = 0;
unsigned long start;
float flat = -99.99;
float flon = -99.99;
float voltase = 0.0;
float ampere = 0.0;
float tekanan, suhu;
String result;

void setup() {
  Serial.begin(9600); //Serial USB
  Serial1.begin(9600); //SIM800L
  Serial2.begin(9600); //GPS U BLOX

  //LED init
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);

  //SD pin
  pinMode(53, OUTPUT); //SS MEGA 53, UNO 10
  digitalWrite(53, HIGH);

  //LCD init
  lcd.begin(16, 2);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.createChar(0,water);
  lcd.createChar(1,pipe);
  lcd.createChar(2,smile);

  //WELCOME SCREEN
  lcd.setCursor(2, 0);
  lcd.print(F("* I-GAUGE"));
  lcd.write(byte(1));
  lcd.print(F(" *"));
  lcd.setCursor(1, 1);
  lcd.write(byte(0));
  lcd.print(F(" PDAM BOGOR "));
  lcd.write(byte(0));
  delay(2000);

  for (i = 8; i <= 11; i++) {
    digitalWrite(i, HIGH);
    if (i > 8)
      digitalWrite(i - 1, LOW);
    delay(500);
  }

  digitalWrite(11, LOW);
  digitalWrite(10, HIGH);
  lcd.clear();
  //INISIALISASI RTC
  if (! rtc.begin()) {
    lcd.setCursor(0, 0);
    lcd.print(F("RTC ERROR!!!")); //Please run the SetTime
    lcd.setCursor(0, 1);
    lcd.print(F("CONTACT CS"));
    digitalWrite(10, LOW);
    while (1) {
      digitalWrite(8, HIGH); //LED RED
      delay(1000);
      digitalWrite(8, LOW);
      delay(2000);
    }
  }
  lcd.setCursor(0, 0);
  lcd.print(F("REAL TIME CLOCK")); //Please run the SetTime
  lcd.setCursor(5, 1);
  lcd.print(F("READY"));
  lcd.write(byte(2));
  delay(1000);

  //INIT SD CARD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("- SD CARD INIT -"));
  lcd.setCursor(0, 1);

  if (!SD.begin(53)) {
    lcd.print(F("SD CARD ERROR!!!"));
    digitalWrite(10, LOW);
    digitalWrite(8, HIGH); //LED RED
    while (1) {
    }
  }
  lcd.setCursor(0, 1);
  lcd.print(F("- SD CARD OK!  -"));
  delay(1000);
  lcd.clear();

  //INIT GPS
  GPS_ON();

  //INIT SIM800L & SEND SMS
  lcd.print(F("CHECK SIM800L"));
  //ConnectAT("AT", 100);
  sim800l();

  //init sensor
  ads.begin();      //ADS1115
  sensors.begin();  //DS18B20
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);
  delay(500);
  //AMBIL INTERVAL PENGUKURAN
  lcd.setCursor(0, 0);
  lcd.print(F("TIME INTERVAL"));
  configs(); //pengambilan nilai waktu penyimpanan data
  lcd.print(waktu);
  lcd.print(F(" MINUTES"));
  delay(1000);
  lcd.clear();
  lcd.print(F("BURST INTERVAL"));
  lcd.setCursor(0, 1);
  lcd.print(numAvg);
  lcd.print(F(" SECONDS"));
  delay(1000);
  
  for (i = 8; i <= 11; i++) {
    digitalWrite(i, LOW);
}
  digitalWrite(11, HIGH);

  lcd.clear();
  ambil();
  Alarm.timerRepeat(waktu * 60, ambil);
}

void loop() {
 Alarm.delay(0);
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
  //INISIALISASI GPS - WAKTU TUNGGU MENCARI SATELIT
  lcd.clear();
  lcd.print(F("--  GPS INIT  --"));
  lcd.setCursor(0, 1);
  //GET COORDINATES
  displayInfo();
  delay(500);

  lcd.clear();
  //DISPLAY LONG & LAT
  lcd.print(F("LONGITUDE"));
  lcd.setCursor(0, 1);
  if (flon < 0)
    lcd.print("W ");
  else
    lcd.print("E ");
  i = int(abs(flon));
  lcd.print(i);
  lcd.print(char(223));
  tekanan = ((abs(flon) - float(i)) * 60.0000000);
  lcd.print(int(tekanan));
  lcd.print("'");
  tekanan = (tekanan - float(int(tekanan))) * 60.00;
  lcd.print(tekanan);
  lcd.print("\"");
  delay(1000);

  lcd.clear();
  lcd.print(F("LATITUDE"));
  lcd.setCursor(0, 1);
  if (flon < 0)
    lcd.print("S ");
  else
    lcd.print("N ");
  i = int(abs(flat));
  lcd.print(i);
  lcd.print(char(223));
  tekanan = ((abs(flat) - float(i)) * 60.0000000);
  lcd.print(int(tekanan));
  lcd.print("'");
  tekanan = (tekanan - float(int(tekanan))) * 60.00;
  lcd.print(tekanan);
  lcd.print("\"");
  delay(1000);
  lcd.clear();
  i = '0';
  tekanan = '0';
  index = 1;
}

void displayInfo() {
  lcd.print(F(" Waiting Signal"));
  for (start = millis(); millis() - start < 1000;) {
    while (Serial.available()) {
      char g = Serial.read();
      gps.encode(g);
    }
  }

  if (gps.location.isUpdated())  {
    flat = gps.location.lat();
    flon = gps.location.lng();
    lcd.setCursor(0, 1);
    lcd.print("                ");    
    lcd.setCursor(0, 1);
    lcd.print(F("- GPS DETECTED -"));
    digitalWrite(11, HIGH);
    
  }
  if (gps.charsProcessed() < 10)  {
    lcd.setCursor(0, 1);
    lcd.print("                ");  
    lcd.setCursor(0, 1);  
    lcd.print(F("NO GPS DETECTED!"));
    digitalWrite(10, LOW);
    digitalWrite(8, HIGH);
  }
  delay(1000);
}

void sim800l() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("SEND SMS"));
  Serial1.print(F("AT+CMGF=1\r\n"));
  delay(100);
  Serial1.print("AT+CMGS=" + String (PHONE_NUMBER) + "\r\n");
  delay(100);
  Serial1.print(F("Testing Kirim SMS via SIM800L"));
  delay(100);
  Serial1.print((char)26);
  delay(100);
}

void ConnectAT(String cmd, int t) {
  i = 0;
  while (1) {
    Serial1.println(cmd);
    while (Serial1.available()) {
      if (Serial1.find("OK"))
        i = 8;
    }
    delay(t);
    if (i > 5) {
      break;
    }
    i++;
  }
  if (i == 8) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("OK");
    result = "OK";
    delay(1000);
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("SIM800L ERROR");
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
    while (1) {
    }
  }
}

void configs() {
  index = 0;
  file = SD.open("/");
  file.close();
  file = SD.open(F("config.txt"));
  lcd.setCursor(0, 1);
  if (file) {
    while (file.available()) {
      next_char = file.read();
      tgl[index++] = next_char;
    }
    file.close();
  }
  else  {
    lcd.print(F("ERROR READING"));
    while (1) {

    }
  }
  filename = String(tgl);
  for ( i = 0; i < 8; i++)
    tgl[i] = 0;
  a = filename.indexOf("\r");
  waktu = filename.substring(0, a).toInt();
  numAvg = filename.substring(a + 1, filename.length()).toInt();
  filename = '0';
}

void ambil() {
  Alarm.delay(0);
  digitalWrite(10, HIGH);
  nows = rtc.now();
  sprintf(tgl, "%d/", nows.year());
  if (SD.exists(tgl))  {
  }
  else SD.mkdir(tgl);

  //PENGECEKAN FOLDER BULAN
  sprintf(tgl, "%d/%d/", nows.year(), nows.month());
  if (SD.exists(tgl)) {
  }
  else SD.mkdir(tgl);

  //SAVE DATA
  filename = String(nows.year()) + '/' + String(nows.month()) + '/' + String(nows.day()) + ".ab";
  filename.toCharArray(str, 13);
  file = SD.open(str, FILE_WRITE);
  nows = rtc.now();
  file.print(nows.year());
  file.print('/');
  save2digits(nows.month());
  file.print('/');
  save2digits(nows.day());
  file.print(' ');
  save2digits(nows.hour());
  file.print(':');
  save2digits(nows.minute());
  file.print(':');
  save2digits(nows.second());
  file.print('|');
  file.print(flon, 6);
  file.print('|');
  file.print(flat, 6);
  file.print('|');
  file.close();
  lcd.clear();

  //ambil data tekanan
  for (i = 0; i < numAvg; i++) {
    reads += analogRead(A0); //tekanan
    reads0+=analogRead(A1);  //voltase & arus
    nows = rtc.now();
    lcd.setCursor(3, 0);
    lcd.print(nows.year());
    lcd.write('/');
    lcd2digits(nows.month());
    lcd.write('/');
    lcd2digits(nows.day());
    
    lcd.setCursor(4, 1);
    lcd2digits(nows.hour());
    lcd.write(':');
    lcd2digits(nows.minute());
    lcd.write(':');
    lcd2digits(nows.second());
    delay(1000);
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("PRES = "));

  tekanan = ((((float)reads / (float)numAvg) - 54.0) / 944.00) * 250.00;
  //voltase = (((float)reads0 / (float)numAvg)*5.0/1024.0)/(R/(2*R));
  voltase = (((float)reads0 / (float)numAvg)*5000.00/1024.00);
  ampere = ((voltase - ACSoffset) / mVperAmp);
  sensors.requestTemperatures();
  suhu = sensors.getTempCByIndex(0);

    //simpan data ke LCD
  file = SD.open(str, FILE_WRITE);
  file.print(tekanan);
  file.print('|');
  file.print(suhu);
  file.print('|');
  file.print(voltase/1000.00);
  file.print('|');
  file.println(ampere);
  file.close();

  Serial1.print(tekanan);
  Serial1.print('|');
  Serial1.print(suhu);
  Serial1.print('|');
  Serial1.print(voltase/1000.00);
  Serial1.print('|');
  Serial1.println(ampere);
  
  //tampilkan data ke LCD
  lcd.print(tekanan, 2);
  lcd.setCursor(13,0);
  lcd.print(F("kPa"));
  lcd.setCursor(0,1);
  lcd.print(F("TEMP  = "));
  lcd.print(suhu, 2);
  lcd.setCursor(14,1);
  lcd.print(char(223));
  lcd.setCursor(15,1);
  lcd.print(F("C"));
  delay(1000);
  lcd.clear();
  lcd.print(F("VOLTAGE = "));
  lcd.print(voltase/1000.00, 2);
  lcd.setCursor(15,0);
  lcd.print(F("V"));
  lcd.setCursor(0,1);
  lcd.print(F("CURRENT = "));
  lcd.print(ampere, 2);
  lcd.setCursor(15,1);
  lcd.print(F("A"));
  delay(1000);

  //bersih variabel
  reads = 0;
  reads0 = 0;
  tekanan = '0';
  suhu = '0';
  voltase = '0';
  ampere = '0';
  digitalWrite(10, LOW);
}


