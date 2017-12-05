#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <SD.h>               // SD CARD
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

//SERVER ADDRESS
#define SERVER_ADDRESS 5

// Singleton instance of the radio driver
#define RFM95_CS 53
#define RFM95_RST 7
#define RFM95_INT 3

//Component Initialization
//RTC
RTC_DS1307 rtc;
DateTime nows;

//LCD 16X2 I2C
#define I2C_ADDR    0x3F //0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

//water icon
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

RH_RF95 rf95(RFM95_CS, RFM95_INT);
RHReliableDatagram manager(rf95, SERVER_ADDRESS); //rf95, SERVER_ADDRESS

//SD CARD
File file;
String filename;

// Dont put this on the stack:
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
uint8_t from;

//GLOBAL VARIABLE
long lastSendTime, start, waktu;
char g;
byte a, b, i, c, error;
int kode;
String teks, operators, y;
String network, APN, USER, PWD, sms, kuota;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);  // SIM800L
  delay(500);

  //LED status init
  pinMode(34, OUTPUT);  // Red
  pinMode(36, OUTPUT);  // Green
  pinMode(38, OUTPUT);  // Blue
  
  //LCD init
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
  delay(500);
  //WELCOME SCREEN
  Serial.println(F("LoRa Server Mega2560!"));
  for (i = 0; i <= 2; i++) {
    Serial.println(F("I-GAUGE LoRa"));
    digitalWrite(34 + 2 * i, HIGH);
    lcd.setCursor(0, 0);    lcd.print(F(" I-GAUGE "));
    lcd.write(byte(1));    lcd.print(F(" LoRa "));
    lcd.setCursor(0, 1);    lcd.write(byte(0));
    lcd.print(F(" - GATEWAY! - "));
    lcd.write(byte(0));
    delay(2000);
    digitalWrite(34 + 2 * i, LOW);
  }
  ledOff();
  digitalWrite(36, HIGH); //GREEN
  delay(100);

  lcd.clear();

  //INISIALISASI RTC
  if (! rtc.begin() || ! rtc.isrunning()) {
    lcd.setCursor(0, 0); lcd.print(F("- RTC ERROR!!! -")); //Please run the SetTime
    lcd.setCursor(0, 1); lcd.print(F("-  CONTACT CS  -"));
    digitalWrite(36, LOW);
    Serial.println(F("RTC ERROR!!!"));
    while (1) {
      digitalWrite(34, HIGH); //LED RED
      delay(500);//
      digitalWrite(34, LOW);
      delay(500);//
    }
  }

  lcd.clear();
  //GET TIME FROM RTC
  for (i = 0; i < 2; i++) {
    nows = rtc.now();
    lcd.setCursor(4, 1);
    print2digits(nows.hour());

    lcd.write(':');
    print2digits(nows.minute());

    lcd.write(':');
    print2digits(nows.second());

    lcd.setCursor(3, 0);
    print2digits(nows.day());

    lcd.write('/');
    print2digits(nows.month());

    lcd.write('/');
    lcd.print(nows.year());
    delay(1000);
  }
  lcd.clear();
  lcd.setCursor(0, 0);  lcd.write(byte(2));
  lcd.setCursor(1, 0);  lcd.write(byte(3));
  lcd.setCursor(0, 1);  lcd.write(byte(4));
  lcd.setCursor(1, 1);  lcd.write(byte(5));
  lcd.setCursor(14, 0);  lcd.write(byte(2));
  lcd.setCursor(15, 0);  lcd.write(byte(3));
  lcd.setCursor(14, 1);  lcd.write(byte(4));
  lcd.setCursor(15, 1);  lcd.write(byte(5));
  lcd.setCursor(6, 0); lcd.print(F("RTC!")); //Please run the SetTime
  lcd.setCursor(5, 1); lcd.print(F("READY"));
  lcd.write(byte(6));
  Serial.println(F("RTC OK!!!"));
  delay(3000);

  // manual reset
  digitalWrite(RFM95_RST, LOW);  delay(10);
  digitalWrite(RFM95_RST, HIGH); delay(10);
  lcd.clear();

  while (!manager.init()) {
    Serial.println(F("LoRa GATEWAY init failed"));
    lcd.print(F("- LoRa GATEWAY -"));
    lcd.setCursor(0, 1);
    lcd.print(F("- Init  Failed -"));
    digitalWrite(36, LOW);
    digitalWrite(34, HIGH); //RED
    while (1);
  }
  Serial.println(F("LoRa GATEWAY init OK!"));
  lcd.print(F("- LoRa GATEWAY -"));
  lcd.setCursor(0, 1);
  lcd.print(F("--  Init OK!  --"));
  delay(3000);

  //SD CARD
  ledOff();
  digitalWrite(38, HIGH);
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(F("- SD CARD INIT -"));
  lcd.setCursor(0, 1);
  pinMode(10, OUTPUT); //SS for SD Card
  digitalWrite(10, HIGH);
  if (!SD.begin(10)) {
    lcd.print(F("SD CARD ERROR!!!"));
    Serial.println(F("SD CARD ERROR!!!"));
    digitalWrite(34, HIGH); //LED RED
    digitalWrite(38, HIGH); //LED BLUE
    while (1) {}
  }
  digitalWrite(38, HIGH); //LED BLUE
  Serial.println(F("SD CARD OK!!!"));
  lcd.setCursor(0, 1);
  lcd.print(F("- SD CARD OK!! -"));
  delay(3000);

  // Setup ISM frequency
  lcd.clear();
  ledOff();
  lcd.setCursor(0, 0); lcd.print(F(" LoRa set Freq!"));
  lcd.setCursor(0, 1);
  if (!rf95.setFrequency(915.0)) { //freq in MHz
    Serial.println(F("setFrequency failed"));
    digitalWrite(36, HIGH); //STATUS GREEN
    digitalWrite(38, HIGH); //BLUE
    while (1);
  }
  digitalWrite(36, HIGH); //STATUS GREEN
  Serial.println(F("Set Freq to: 915.0"));
  lcd.print(F("Freq = 915.0 MHz"));
  // Setup Power,dBm
  //rf95.setTxPower(23, false);
  rf95.setTxPower(22);
  delay(3000);

  //CHECK SIM800L
  lcd.clear();
  lcd.print(F("CHECKING SIM800L"));
  ConnectAT("AT", 100);
  ceksim();
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("SIM800L READY!");
  delay(2000);

  nows = rtc.now();
  setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());
  
  lcd.clear();
  lcd.print(F("CEK KUOTA"));
  
  cekkuota(); // waktu yang dibutuhkan untuk ccek kuota 25,57 detik

  lcd.setCursor(0,1); lcd.print(F("Sudah dicek"));
  delay(2000);
  Alarm.alarmRepeat(5, 0, 0, cekkuota); // 5:00am every day
 // Alarm.timerRepeat(3*60, cekkuota); // every 3 minutes

  Serial.println(F("Setup completed"));
  lcd.clear();
  lcd.print(F("Setup Completed!"));
  lcd.setCursor(0, 1);
  for (i = 0; i < 5; i++) {
    digitalWrite(36, LOW);
    delay(300);
    digitalWrite(36, HIGH);
    delay(300);
  }
  lcd.print(F("Waiting Data ..."));
  delay(1000);
}

void loop() {
  Alarm.delay(0);
  if (manager.available())  {
    lastSendTime = millis();
    digitalWrite(36, LOW); //STATUS GREEN
    if (manager.recvfromAck(buf, &len, &from))    {
      lcd.clear();
      teks = "";
      lcd.print(F("Request from: "));
      Serial.print(F("got request from : "));
      Serial.print(from);
      lcd.print(from);
    lcd.setCursor(0,1);
    lcd.print(F("                "));
    lcd.setCursor(0,1);
      Serial.print(" > ");
      for (byte i = 0; i < len; i++) {
        teks += (char)buf[i];
      }
      Serial.println(teks);

      uint8_t data[] = "OK";
      if (manager.sendtoWait(data, sizeof(data), from)) { //data,size data, client
        uint8_t len = sizeof(buf);
        uint8_t from;
        if (manager.recvfromAckTimeout(buf, &len, 3000, &from)) { //DATA MASUK
          teks = "";
          for (byte i = 0; i <= len; i++) {
            teks += (char)buf[i];
          }
          Serial.println(teks);
          waktu=millis();
      simpan(teks);
          server();
          Serial.print(F("WAktu yang dibutuhkan untuk menyimpan data dan mengirim ke server="));
      Serial.println(millis()-waktu);
      lcd.setCursor(0, 1);
          lcd.print(F("DATA SAVED"));
          digitalWrite(36, HIGH); //STATUS GREEN
          bersihdata();
        }
      }
    }
  }
}

void simpan(String text) {
  //save to sd card
  nows = rtc.now();
  filename = "";
  filename = String(nows.year());
  if (nows.month() < 10) {
    filename += "0" + String(nows.month());
  }
  if (nows.month() >= 10) {
    filename += String(nows.month());
  }
  if (nows.day() < 10) {
    filename += "0" + String(nows.day());
  }
  if (nows.day() >= 10) {
    filename += String(nows.day());
  }
  filename += ".ab";


  boolean a = SD.exists(filename);
  file = SD.open(filename, FILE_WRITE);
  if (a == 0) {
    file.print(F("TimeStamp|Date From Receiver|longitude|latitude|pressure|"));
    file.print(F("temperature|volt|ampere|Source|ID Station|burst interval|"));
    file.println(F("data interval|RSSI|SNR|Time|Sent to Server"));
  }
  text = text.substring(0, text.lastIndexOf('#'));
  teks = text;
  Serial.println(text);

  file.print(nows.year());  file.print('-');
  save2digits(nows.month()); file.print('-');
  save2digits(nows.day());  file.print(' ');
  save2digits(nows.hour());   file.print(':');
  save2digits(nows.minute()); file.print(':');
  save2digits(nows.second()); file.print('|');
  file.print(text);
  file.print("|");
  file.print(String(rf95.lastRssi()));
  file.print("|");
  file.print(String(rf95.lastSNR()));
  file.print("|");
  file.print(millis() - lastSendTime);
  file.print("|");

  file.flush();
  file.close();
  Serial.println(F("DATA SAVED"));
  Serial.flush();
}

void server() {
  //parsing teks dari NODE
  //Date|longitude|latitude|pressure|temperature|volt|ampere|source|ID|burst interval|data interval
  y = "{\"Data\":\"'";
  y += teks; //DATE
  y += "'\"}"; //DATA INTERVAL

  Serial.println(y);
  //kirim ke server string y
  senddata(1);
  file = SD.open(filename, FILE_WRITE);
  file.println(network);
  file.flush();  file.close();
  bersihdata();
}

void cekkuota() {
  waktu=millis(); 
  c = 0;
  Serial.flush();
  Serial1.flush();
  //Serial.println("AT+CSCLK");
  Serial1.println("AT+CSCLK");
  Serial.flush();
  Serial1.flush();
  delay(500);
  //Serial.println("AT+CSCLK=0");
  Serial1.println("AT+CSCLK=0");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  delay(400);
top:
  if (c == 5)  {
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
  delay(500);
  //Serial.println("AT+CSQ");
  Serial1.println("AT+CSQ");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();

  //Serial.println("AT+CUSD=2");
  Serial1.println("AT+CUSD=2");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();

  //Serial.println("AT+CUSD=1");
  Serial1.println("AT+CUSD=1");
  bacaserial(100);
  Serial.flush();
  Serial1.flush();
  delay(400);
  sms = "";
  start = millis();
  //Serial.println("AT+CUSD=1,\"*888#\"");
  Serial1.println("AT+CUSD=1,\"*888#\"");
  Serial.flush();
  Serial1.flush();

  while ((start + 5000) > millis()) {
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
  //PERLU DICEK UNTUK KARTU SIMPATI
  a = sms.indexOf(':');
  b = sms.indexOf('/', a + 1);
  sms = sms.substring(a + 5, b + 8);
  start = millis();
  //Serial.println("AT+CUSD=1,\"3\"");
  Serial1.println("AT+CUSD=1,\"3\"");
  while ((start + 3000) > millis()) {
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
  delay(100);

  kuota = "";
  start = millis();
  //Serial.println("AT+CUSD=1,\"2\"");
  Serial1.println("AT+CUSD=1,\"2\"");
  while ((start + 3000) > millis()) {
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
  senddata(0);
  
  Serial.print("Waktu yang dibutuhkan untuk cek kuota=");
  Serial.println(millis()-waktu);
}

void senddata(boolean d) {
  ledOff();
  error = 0;
  Serial.flush();
  Serial1.flush();
serve:
  filename = "";
  delay(400);
  Serial.print(F("AT+CGATT? "));
  Serial1.println(F("AT+CGATT?"));
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
  if (filename.toInt() == 1) {
    //kirim data ke server
    delay(400);
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
    delay(400);
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
    delay(500);
    //TERMINATE HTTP SERVICE
    //Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    delay(400);
    //INITIALIZE HTTP SERVICE
    //Serial.println(F("AT+HTTPINIT"));
    Serial1.println(F("AT+HTTPINIT"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    delay(500);
    //SET HTTP PARAMETERS VALUE
    //Serial.println(F("AT+HTTPPARA=\"CID\",1"));
    Serial1.println(F("AT+HTTPPARA=\"CID\",1"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    delay(500);
    //SET HTTP URL
    if (d == 0) {
      //Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_sim_c.php\""));
      Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_sim_c.php\""));
      bacaserial(1000);
      Serial.flush();
      Serial1.flush();
      //"Data":"'2017-11-05 10:00:00', ID, 'pulsa', 'kuota'"
      //Formatnya Date, ID, pulsa, kuota
      nows = rtc.now();
      y = "{\"Data\":\"'";
      y += String(nows.year()) + "-";
      if (nows.month() < 10) {
        y += "0" + String(nows.month())  + "-";
      }
      if (nows.month()  >= 10) {
        y += String(nows.month() ) + "-";
      }
      if (nows.day()  < 10) {
        y += "0" + String(nows.day()) + " ";
      }
      if (nows.day() >= 10) {
        y += String(nows.day()) + " ";
      }
      if (nows.hour() < 10) {
        y += "0" + String(nows.hour()) + ":";
      }
      if (nows.hour() >= 10) {
        y += String(nows.hour()) + ":";
      }
      if (nows.minute() < 10) {
        y += "0" + String(nows.minute()) + ":";
      }
      if (nows.minute() >= 10) {
        y += String(nows.minute()) + ":";
      }
      if (nows.second() < 10) {
        y += "0" + String(nows.second());
      }
      if (nows.second() >= 10) {
        y += String(nows.second());
      }
      y += "','";
      y += "BOGOR";
    y += SERVER_ADDRESS;
    y +="','";
      y += sms + "','";
      y += kuota + "'\"}";
      //SET HTTP DATA FOR SENDING TO SERVER
      teks = "AT+HTTPDATA=" + String(y.length() + 1) + ",15000";
    }
    if (d == 1) {
      //Serial.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      Serial1.println(F("AT+HTTPPARA=\"URL\",\"http://www.mantisid.id/api/product/pdam_dt_c.php\""));
      bacaserial(1000);
      Serial.flush();
      Serial1.flush();
      //"Data":"'2017-11-05 10:00:00', '111.111111', '-6.2222222', '400.33', '34.00', '5.05', '1.66', 'pdam_001', '5', '3'"
      //Formatnya Date, longitude, latitude, pressure, temperature, volt, ampere, SOURCE, id, burst interval, data interval

      //SET HTTP DATA FOR SENDING TO SERVER
      teks = "AT+HTTPDATA=" + String(y.length() + 1) + ",15000";
    }

    Serial.println(teks);
    Serial1.println(teks);
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
    delay(500);
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
    delay(1000);
    //Serial.println(F("AT+HTTPTERM"));
    Serial1.println(F("AT+HTTPTERM"));
    bacaserial(100);
    Serial.flush();
    Serial1.flush();
    Serial.println(F("AT+SAPBR=0,1"));
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
}

void ceksim() {
cops:
  filename = "";
  Serial.flush();
  Serial1.flush();
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

void bersihdata() {
  for (byte i = 0; i < sizeof(buf); ++i ) {
    buf[i] = (char)0;
  }
  teks = "";  filename = "";  from = '0';
  y = "";  network = "";  lastSendTime = '0';  start = '0';
  kode = '0';  sms = "";  kuota = "";

}

void ledOff() {
  digitalWrite(34, LOW);
  digitalWrite(36, LOW);
  digitalWrite(38, LOW);
}

void save2digits(int number) {
  if (number >= 0 && number < 10) {
    file.print('0');
  }
  file.print(number);
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.print('0');
  }
  lcd.print(number);
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
    Serial.println("OK");
    Serial.flush();
    Serial1.flush();
    teks = "OK";
    delay(1000);
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("SIM800L ERROR");
    Serial.println("SIM800L ERROR");
    Serial.flush();
    Serial1.flush();
    digitalWrite(34, HIGH);
    digitalWrite(38, HIGH);
    while (1) {}
  }
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



