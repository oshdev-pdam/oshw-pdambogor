#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

#define CLIENT 3
#define SERVER_ADDRESS 5

// Singleton instance of the radio driver
#define RFM95_CS 10
#define RFM95_RST 7
#define RFM95_INT 2

#define RF95_FREQ 915.0

RH_RF95 rf95(10, 2); //CS, INTERUPT

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(rf95, CLIENT); // please choise the client

String teks;
// Dont put this on the stack:
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
uint8_t from;
byte error = 0;
int iterasi=0;
byte i;

void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  digitalWrite(7, HIGH);
  delay(1000);
  digitalWrite(A0, HIGH);
  Serial.print(F("LoRa endNode Uno Client "));
  Serial.println(CLIENT);
  // manual reset
  digitalWrite(7, LOW);  delay(10);
  digitalWrite(7, HIGH); delay(10);
  while (!manager.init()) {
    Serial.println(F("LoRa radio init failed"));
    while (1);
  }
  Serial.println(F("LoRa radio init OK!"));

  // Setup ISM frequency
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println(F("setFrequency failed"));
    while (1);
  }
  Serial.println(F("Set Freq to: 915.0"));

  // Setup Power,dBm
  //rf95.setTxPower(23, false);
  rf95.setTxPower(22, true);
  delay(1000);
  Serial.println(F("Setup completed"));
  setTime(1, 15, 3, 1, 12, 2017); //setTime(int hr,int min,int sec,int dy, int mnth, int yr)
  ambil();
  Alarm.timerRepeat(5*60, ambil);
}

void loop() {
  Alarm.delay(0);
}

void ambil() {
  //rf95.setModeIdle();
  error=0;
  while (error < 15) {
    bersihdata();
    digitalWrite(A0, HIGH);
    delay(500);
    Serial.println(F("Sending to server"));
    //check server
    teks = "FF ";
    uint8_t data[teks.length()];
    teks.toCharArray(data, teks.length());
    Serial.println(teks);
    // Send a OXFF to manager_server
    if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS)) {
      // Now wait for a reply from the server
      if (manager.recvfromAckTimeout(buf, &len, 2000, &from))    {
        Serial.print(F("got reply from : 0x"));
        Serial.print(from, HEX);
        Serial.print(F(" : "));
        Serial.println((char*)buf);
        teks = "";
        for (byte i = 0; i <= len; i++) {
          teks += (char)buf[i];
        }
        if (teks == "OK") {
          digitalWrite(A0, LOW); //nyala
          iterasi++;
          teks = "";
          //ID, Date, longitude, latitude, pressure, temperature, volt, ampere, burst interval, data interval
          teks = String(year());      teks += '-';
          if(month()<10) teks += "0" + String(month());
          if(month()>=10) teks += String(month());
          teks += '-';
          if (day()<10) teks += "0" + String(day());
          if(day()>=10) teks += String(day());
          teks += ' ';
          if (hour()<10) teks += "0" + String(hour());
          if (hour()>=10) teks += String(hour());
          teks += ":";
          if (minute()<10) teks += "0" + String(minute());
          if (minute()>=10) teks += String(minute());
          teks += ":";
          if (second()<10) teks += "0" + String(second());
          if (second()>=10) teks += String(second());
          teks += "','";
          teks += String(float(random(110, 120)), 4);; //longitude
          teks += "','";
          teks += String(float(random(-4, -7)), 5);; //latitude
          teks += "','";
          teks += String(float(random(2, 40)), 2); //pressure
          teks += "','";
          teks += String(float(random(20, 50)), 2); //temperature
          teks += "','";
          teks += String(float(random(3, 6)), 2); //volt
          teks += "','";
          teks += String(float(random(1, 5)), 2); //ampere
          teks += "','";
          teks += "BATTERY',"; //source
          teks += "'BOGOR"; //ID
          teks += CLIENT; //ID NUMBER
          teks +="','";
          teks += random(1, 30); //burst interval
          teks += "','";
          teks += random(1, 15); //data interval
          teks += "#&";
          uint8_t data[teks.length()];
          teks.toCharArray(data, teks.length());
          Serial.println(teks);
          manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS);
//          rf95.sleep();
          error = 0;
          delay(1000);
          digitalWrite(A0, HIGH);
          break;
        }
      }
      else    {
        Serial.println(F("No reply, server running?"));
        digitalWrite(A0, LOW);
        error++;
      }
    }
    else {
      Serial.print(error);
      Serial.println(F("sendtoWait failed"));
      digitalWrite(A0, LOW);
      error++;
    }
    delay(3000);
  }

if(error==15){
  Serial.println(F("wait another time"));
  }
}

void bersihdata() {
  for ( i = 0; i < sizeof(buf); ++i ) {
    buf[i] = (char)0;
  }
  teks = "";
  from = '0';
}




