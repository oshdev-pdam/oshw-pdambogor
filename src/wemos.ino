#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

const char *ssid = "hollandaesp"; //must at least 8 character
const char *password = "12345678";//must at least 8 character
ESP8266WebServer server(80);
IPAddress    apIP(10, 10, 10, 1);

byte id = 1;
byte setting = 0;
byte a, b, burst, interval;
String noHP, kirim;
String data, operators, station, latitude, longitude, bujur, lintang, dates;
String tekanan, suhu, volt, arus, offset;
char g;
unsigned long start;

void setup() {
  Serial.begin(57600);
  delay(1000);
  Serial.println();
  Serial.println("Configuring access point...");
  Serial.flush();
  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(F("waiting for connection"));

  while (Serial.find("123") == false) {
  }
  
  //server.sendHeader("Connection", "close");
  //server.sendHeader("Access-Control-Allow-Origin", "*");
  server.on("/", handleroot);
  server.begin();
}

void loop() {
  server.handleClient();
}

String page() {
  String html = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">\r\n";
  html += "<style type=\"text/css\">input {text-align:right;}</style>\r\n\r\n";
  html += "<style>\r\ninput[type=text], select, textarea {\r\n";
  html += " padding: 4px;\r\n border: 1px solid #ccc;\r\n border-radius: 4px;\r\n box-sizing: border-box;\r\n margin-top: 4px;\r\n margin-bottom: 4px;\r\n resize: vertical;\r\nfont-family: \"Lato\", sans-serif;font-size: 16px;}\r\n";
  html += "input[type=submit] {\r\n";
  html += " background-color: #4CAF50;\r\n color: white;\r\n padding: 12px 20px;\r\n border: none;\r\n border-radius: 4px;\r\n cursor: pointer;\r\n}\r\n";
  html += "input[type=submit]:hover {\r\n";
  html += " background-color: green;\r\n}\r\n";
  html += ".container {\r\n";
  html += " border-radius: 5px;\r\n background-color: white;\r\n padding: 2px;\r\n}\r\n";
  html += "</style>\r\n\r\n";
  html += "<style>\r\nlabel{\r\ndisplay:inline-block;\r\nwidth:250px;\r\ntext-align:left;\r\n}\r\n\r\n";
  html += "fieldset{\r\nborder:none;\r\nwidth:500px;\r\n\r\n}\r\n</style>\r\n\r\n";
  html += "<style>\r\n";
  html += "body {font-family: \"Lato\", sans-serif;}\r\n";
  html += "div.tab {\r\n";
  html += "    overflow: hidden;\r\n border: 5px solid white;\r\n background-color: white;\r\n}\r\n";
  html += "div.tab button {\r\n";
  html += " background-color: magenta;\r\n  float: left;\r\n border: none;\r\n outline: none;\r\n cursor: pointer;\r\n padding: 10px 24px;\r\n transition: 0.3s;\r\n font-size: 17px;font-family: \"Lato\", sans-serif;\r\n }\r\n";
  html += "div.tab button:hover {\r\n";
  html += " background-color: blue;\r\n}\r\n";
  html += "div.tab button.active {\r\n";
  html += " background-color: cyan;\r\n}\r\n";
  html += ".tabcontent {\r\n display: none;\r\n padding: 1px 1px;\r\n border: 1px solid white;\r\n border-top: none;\r\n }\r\n</style>\r\n";
  html += "</head>\r\n<title>I_GAUGE</title>\r\n<body>\r\n<h2 style=\"text-align: center;\">\r\n";
  html += "<strong>I-GAUGE CONFIGURATION AND DISPLAY</strong>\r\n</h2>\r\n";

  html += "<div class=\"tab\">\r\n<button class=\"tablinks\" name=\"DISP\" id=\"1\" onclick=\"openCity(event, 'Disp')\" >\r\n<strong>DATA DISPLAY</strong></button>\r\n"; //<a href=\"display\">
  html += "  <button class=\"tablinks\" name=\"CONFIG\" id=\"2\" onclick=\"openCity(event, 'Config')\">\r\n<strong>CONFIGURATION</strong></button>\r\n";//<a href=\"config\"></a>
  html += "</div>\r\n";

  html += "<form action=\"/\" method=\"POST\">\r\n";
  html += "<div id=\"Config\" class=\"tabcontent\">\r\n";
  html += "<fieldset>\r\n";
  //COORDINATE
  html += "<label><strong>I-GAUGE COORDINATE</strong></label>\r\n";
  html += "<label for=\"longitude\">Longitude (&ordm;dd.dddddd)</label><input type=\"text\" name=\"longitude\" size=\"20\"  ";
  html += "value=\"" + longitude + "\" />\r\n";
  //LATITUDE
  html += "<label for=\"latitude\">Latitude  (&ordm;dd.dddddd)</label><input type=\"text\" name=\"latitude\" size=\"20\"  ";
  html += "value=\"" + latitude + "\" />\r\n";
  //INTERVAL DATA
  html += "<label><strong>INTERVAL DATA</strong></label>\r\n";
  //BURST INTERVAL
  html += "<label for=\"burst\">Burst Interval (Second)</label><input type=\"text\" name=\"burst\" size=\"20\" value=\"";
  html += String(burst) + "\">\r\n";
  //DATA INTERVAL
  html += "<label for=\"data\">Data Interval (Minute)</label><input type=\"text\" maxlength=\"2\"name=\"data\" size=\"20\" value=\"";
  html += String(interval) + "\">\r\n";
  //PHONENUMBER
  html += "<label><strong>SMS CENTER</strong></label>\r\n";
  html += "<label for=\"hp\">Phone Number</label><input type=\"text\" name=\"hp\" size=\"20\" value=\"";
  html += noHP + "\">\r\n";
  //OFFSET
  html += "<label><strong>OFFSET</strong></label>\r\n";
  html += "<label for=\"offset\">OFFSET</label><input type=\"text\" name=\"offset\" size=\"20\" value=\"";
  html += offset + "\">\r\n";
  if (setting == 1) html += "<p>SETTING DONE!</p>\r\n";
  if (setting == 2) html += "<p>I-GAUGE BEGIN RECORD!</p>\r\n";

  html += "<p><input name=\"kirim\" type=\"submit\" value=\"SEND\"> &nbsp;&emsp;&ensp; <input name=\"kirim\" type=\"submit\" value=\"RECORD\"></p>\r\n";
  html += "</form>\r\n";
  html += "</fieldset>\r\n";
  html += "</div>\r\n";

  html += "<form action=\"/\" method=\"POST\">\r\n";
  html += "<div id=\"Disp\" class=\"tabcontent\">\r\n";// ????
  html += "<fieldset>\r\n";
  //ID
  html += "<label for=\"station\">I-GAUGE ID</label><input type=\"text\" readonly name=\"station\" size=\"20\"  ";
  html += "value=\"" + station + "\" />\r\n";
  //OPERATOR
  html += "<label for=\"operator\">Seluler Operator</label><input type=\"text\" readonly name=\"operator\" size=\"20\"  ";
  html += "value=\"" + operators + "\" />\r\n";
  //LONGITUDE
  html += "<label for=\"longitude\">Longitude (&ordm;dd.dddddd)</label><input type=\"text\" readonly name=\"longitude\" size=\"20\"  ";
  html += "value=\"" + longitude + "\" />\r\n";
  //LATITUDE
  html += "<label for=\"latitude\">Latitude  (&ordm;dd.dddddd)</label><input type=\"text\" readonly name=\"latitude\" size=\"20\"  ";
  html += "value=\"" + latitude + "\" />\r\n";
  //DATE
  html += "<label for=\"date\">Date (yyyy/mm/dd hh:mm:ss)</label><input type=\"text\" readonly name=\"date\" size=\"20\" ";
  html += "value=\"" + dates + "\">\r\n";
  //NILAI TEKANAN
  html += "<label for=\"pressure\">Pressure (bar)</label><input type=\"text\" readonly name=\"data\" size=\"20\"  ";
  html += "value=\"" + tekanan + "\" />\r\n";
  //NILAI SUHU
  html += "<label for=\"temperature\">Temperature (&ordm;C)</label><input type=\"text\" readonly name=\"temperature\" size=\"20\" ";
  html += "value=\"" + suhu + "\" />\r\n";
  //NILAI VOLTASE
  html += "<label for=\"volt\">Voltage (Volt)</label><input type=\"text\" readonly name=\"volt\" size=\"20\" ";
  html += "value=\"" + volt + "\" />\r\n";
  //NILAI ARUS
  html += "<label for=\"arus\">Current (Ampere)</label><input type=\"text\" readonly name=\"arus\" size=\"20\" ";
  html += "value=\"" + arus + "\" />\r\n";
  //BUTTON
  html += "<p><input name=\"kirim\" type=\"submit\" value=\"REFRESH\"><a href=\"refresh\"></a></p>\r\n";
  html += "</form>\r\n";
  html += "</fieldset>\r\n";
  html += "</div>\r\n";

  html += "<p style=\"text-align: center;\">";
  html += "Copyright (c) 2017 by MANTIS ID</p>\r\n";
  html += "</h7>\r\n";
  html += "<script>\r\n";
  html += "function openCity(evt, cityName) {\r\n";
  html += "    var i, tabcontent, tablinks;\r\n";
  html += "    tabcontent = document.getElementsByClassName(\"tabcontent\");\r\n";
  html += "    for (i = 0; i < tabcontent.length; i++) {\r\n";
  html += "        tabcontent[i].style.display = \"none\";\r\n";
  html += "    }\r\n";
  html += "    tablinks = document.getElementsByClassName(\"tablinks\");\r\n";
  html += "    for (i = 0; i < tablinks.length; i++) {\r\n";
  html += "        tablinks[i].className = tablinks[i].className.replace(\" active\", \"\");\r\n";
  html += "    }\r\n";
  html += "    document.getElementById(cityName).style.display = \"block\";\r\n";
  html += "    evt.currentTarget.className += \" active\";\r\n";
  html += "}\r\n";
  html += "document.getElementById(\"";
  html += String(id) + "\").click();\r\n";
  html += "</script>\r\n";
  html += "</body>\r\n";
  html += "</html>\r\n";;
  return html;
}

String htm(byte num) {
  String y;
  if (num >= 0 && num < 10) {
    y = "0" + String(num);
  }
  else  y = String(num);
  return y;
}

void handleroot() {
  //BUKA KOMUNIKASI DENGAN ARDUINO
  Serial.println("Go");
    while (Serial.find("&") == false) {
    }
    delay(500);
    Serial.println("%");
    data = "";
    //cek kemungkinan terjadi error communication
    while (1) {
    if (Serial.available()) {
      g = Serial.read();
      data += g;
      if (g == '*')break;
    }
    }

    //FORMAT DATA = ID | LONGITUDE | LATITUDE | DATE | PRESSURE | TEMPERATURE | VOLTAGE | CURRENT | BURST INTERVAL | DATA INTERVAL | PHONE NUMBER | OPERATOR
    //contoh = ID | 123 | -5 | 2017/11/23 12:12:12|12|23|5.0|1.2|2|10|0812345|TELKOMSEL|offset*
    a = data.indexOf('|');
    station = data.substring(0, a);
    b = data.indexOf('|', a + 1);
    longitude = data.substring(a + 1, b);
    a = data.indexOf('|', b + 1);
    latitude = data.substring(b + 1, a);
    b = data.indexOf('|', a + 1);
    dates = data.substring(a + 1, b);
    a = data.indexOf('|', b + 1);
    tekanan = data.substring(b + 1, a);
    b = data.indexOf('|', a + 1);
    suhu = data.substring(a + 1, b);
    a = data.indexOf('|', b + 1);
    volt = data.substring(b + 1, a);
    b = data.indexOf('|', a + 1);
    arus = data.substring(a + 1, b);
    a = data.indexOf('|', b + 1);
    burst = data.substring(b + 1, a).toInt();
    b = data.indexOf('|', a + 1);
    interval = data.substring(a + 1, b).toInt();
    a = data.indexOf('|', b + 1);
    noHP = data.substring(b + 1, a);
    b = data.indexOf('|', a + 1);
    operators = data.substring(a + 1, b);
    offset = data.substring(b + 1, data.length() - 1);

    if (server.args() == 10)
    { kirim = server.arg("kirim");
    setting = 0;
    id = 1;
    }
    if (server.args() == 7) {
    longitude = server.arg("longitude");
    latitude = server.arg("latitude");
    burst = server.arg("burst").toInt();
    interval = server.arg("data").toInt();
    noHP = server.arg("hp");
    offset = server.arg("offset");
    kirim = server.arg("kirim"); //VALUE = SEND | RECORD
    if (kirim == "SEND") {
      setting = 1;
    }
    if (kirim == "RECORD") {
      setting = 2;
    }
    id = 2;
    }
    if (kirim == "")kirim = "None";

    //format data : ^ LONGITUDE | LATITUDE | BURST INTERVAL | DATA INTERVAL | PHONENUMBER | KIRIM #
    Serial.print("^");
    Serial.print(longitude);
    Serial.print("|");
    Serial.print(latitude);
    Serial.print("|");
    Serial.print(burst);
    Serial.print("|");
    Serial.print(interval);
    Serial.print("|");
    Serial.print(noHP);
    Serial.print("|");
    Serial.print(offset);
    Serial.print("|");
    Serial.print(kirim);
    Serial.println("#");
  
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", page());

}

