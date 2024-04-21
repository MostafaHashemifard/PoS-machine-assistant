#include "ESP8266HTTPClient.h"
#include "ESP8266WiFi.h"
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

#define statusLED D7
#define rstBTN D8

IPAddress apIP(192, 168, 40, 1); // Defining a static IP address: local & gateway

const char *ver = "3.00";
const char *APssid = "PoS_Assistant";
const char *APpassword = "1234ABCD"; //DEBUG_HTTPCLIENT
const char *host = "192.168.8.23";
const uint16_t port = 1362;

// Define a web server at port 80 for HTTP
ESP8266WebServer server(80);
WiFiClient wifiClient;
SoftwareSerial swSer(D5, D6);

char ssid[100];
char pass[100];
char url[100];
int ssidLen = 0;
int passLen = 0;
int urlLen = 0;
int httpCode = 0;
int cnt = 0;
int it = 0;
HTTPClient http;

void handleRoot()
{
  char html[2000];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  // Build an HTML page to display on the web-server root address
  //snprintf ( html, 2000,
  String s = "<html>\
              <head>\
                <meta name='viewport' http-equiv='refresh' content='width=device-width, initial-scale=1'/>\
                <title>PoS Assistant</title>\
                <style>\
                  * {\
                    box-sizing: border-box;\
                  }\
                  input[type=text], select, textarea {\
                    width: 100%;\
                    padding: 12px;\
                    border: 1px solid #ccc;\
                    border-radius: 4px;\
                    resize: vertical;\
                  }\
                  input[type=submit] {\
                    background-color: #4CAF50;\
                    color: white;\
                    padding: 12px 20px;\
                    border: none;\
                    border-radius: 4px;\
                    cursor: pointer;\
                    float: right;\
                  }\
                  label {\
                    padding: 12px 12px 12px 0;\
                    display: inline-block;\
                  }\
                  input[type=submit]:hover {\
                    background-color: #45a049;\
                  }\
                  .container {\
                    border-radius: 5px;\
                    background-color: #f2f2f2;\
                    padding: 20px;\
                  }\
                  .col-25 {\
                    float: left;\
                    width: 25%;\
                    margin-top: 6px;\
                  }\
                  .col-75 {\
                    float: left;\
                    width: 75%;\
                    margin-top: 6px;\
                  }\
                 .row:after {\
                    content: '';\
                    display: table;\
                    clear: both;\
                  }\
                  @media screen and (max-width: 600px) {\
                    .col-25, .col-75, input[type=submit] {\
                      width: 100%;\
                      margin-top: 0;\
                    }\
                  }\
                </style>\
              </head>\
  <body>\
    <div class='container'>\
  <form action='/action_page'>\
    <div class='row'>\
      <div class='col-25'>\
        <label for='ssid'>WiFi SSID</label>\
      </div>\
      <div class='col-75'>\
        <input type='text' id='ssid' name='wifissid' maxlength='30' placeholder='WiFi SSID ...' required>\
      </div>\
    </div>\
    <div class='row'>\
      <div class='col-25'>\
        <label for='password'>WiFi Password</label>\
      </div>\
      <div class='col-75'>\
        <input type='text' id='password' name='wifipassword' maxlength='50' placeholder='WiFi Password ...' required>\
      </div>\
    </div>\
    <div class='row'>\
      <div class='col-25'>\
        <label for='url'>URL</label>\
      </div>\
      <div class='col-75'>\
        <input type='text' id='url' name='urladd' maxlength='100' placeholder='URL Address ...' required>\
      </div>\
    </div>\
    <div class='row'>\
      <input type='submit' value='Submit'>\
    </div>\
  </form>\
</div>\
  </body>\
</html>";
  //);
  server.send ( 200, "text/html", s );
}

void handleForm()
{
  String tempSsid = server.arg("wifissid");
  String tempPass = server.arg("wifipassword");
  String tempUrl = server.arg("urladd");
  //
  tempSsid.toCharArray(ssid, tempSsid.length() + 1);
  tempPass.toCharArray(pass, tempPass.length() + 1);
  tempUrl.toCharArray(url, tempUrl.length() + 1);
  //
  ssidLen = tempSsid.length() + 1;
  passLen = tempPass.length() + 1;
  urlLen = tempUrl.length() + 1;
  //
  ssidEepromWrite();
  passEepromWrite();
  urlEepromWrite();
  //
  String s = "<a href='/'> Go Back </a>";
  server.send(200, "text/html", s); //Send web page
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void setup()
{
  pinMode(statusLED, OUTPUT);
  pinMode(rstBTN, INPUT);
  EEPROM.begin(512);
  Serial.begin(115200);
  swSer.begin(9600);
  //
  if (String(char(EEPROM.read(0x01))) == "f")
  {
    ssidLen = readIntFromEEPROM(0x02);
    for (int i = 0; i < ssidLen; i++)
    {
      ssid[i] = char(EEPROM.read(0x04 + i));
    }
    ssid[ssidLen] = '\0';
  }
  else
    it++;
  if (String(char(EEPROM.read(0x33))) == "f")
  {
    passLen = readIntFromEEPROM(0x34);
    for (int i = 0; i < passLen; i++)
    {
      pass[i] = char(EEPROM.read(0x36 + i));
    }
    pass[passLen] = '\0';
  }
  else
    it++;
  if (String(char(EEPROM.read(0x74))) == "f")
  {
    urlLen = readIntFromEEPROM(0x75);
    for (int i = 0; i < urlLen; i++)
    {
      url[i] = char(EEPROM.read(0x78 + i));
    }
    url[urlLen] = '\0';
  }
  else
    it++;

  if (it != 0)
  {
    //set-up the custom IP address
    WiFi.mode(WIFI_AP);//WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));   // subnet FF FF FF 00

    /* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAP(APssid, APpassword);

    IPAddress myIP = WiFi.softAPIP();

    server.on ("/", handleRoot );
    server.on("/action_page", handleForm);
    server.onNotFound ( handleNotFound );

    server.begin();
  }
  else
  {
    WiFi.mode(WIFI_STA);
  }
}

int httpGetSend(String S)
{
  WiFiClientSecure Client;
  int ret;
  
  http.begin(S);
  http.addHeader("Content-Type", "text/plain");
  ret = http.GET();
  http.end();
  return ret;
}

void loop()
{
  if (it != 0) //Access point mode
  {
    digitalWrite(statusLED, LOW);
    server.handleClient();
  }
  else
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.hostname("PoS-Assistant");
      WiFi.begin(String(ssid), String(pass));
      while (WiFi.status() != WL_CONNECTED)
      {
        if(digitalRead(rstBTN) == HIGH)
        {
          digitalWrite(statusLED, LOW);
          delay(3000);
          flashEeprom();
          digitalWrite(statusLED, HIGH);
        }
        digitalWrite(statusLED, LOW);
        delay(200);
        digitalWrite(statusLED, HIGH);
        delay(200);
        digitalWrite(statusLED, LOW);
        delay(200);
        digitalWrite(statusLED, HIGH);
        delay(500);
      }
    }
      
    digitalWrite(statusLED, HIGH);
    if(digitalRead(rstBTN) == HIGH)
    {
      digitalWrite(statusLED, LOW);
      delay(3000);
      flashEeprom();
      digitalWrite(statusLED, HIGH);
    }

    String price = swSer.readString();
    price.trim();
    if(price.length())
    {
      digitalWrite(statusLED, LOW);
      delay(200);
      digitalWrite(statusLED, HIGH);
      delay(200);
      
      while(!wifiClient.connect(url, port))
      {
        Serial.println("Connection to host failed");
        digitalWrite(statusLED, LOW);
        delay(20);
        digitalWrite(statusLED, HIGH);
        delay(20);
        digitalWrite(statusLED, LOW);
        delay(20);
        digitalWrite(statusLED, HIGH);
        delay(20);
      }
      String cmd = "@@PNA@@01100";
      if (price.length() < 10)
        cmd += "0" + String(price.length());
      else
        cmd += String(price.length());
      cmd += price + "00000200000000011";
      
      //wifiClient.println("@@PNA@@0110004100000000200000000011");
      wifiClient.println(cmd);
      delay(250);
      while (wifiClient.available() > 0)
      {
        char c = wifiClient.read();
        delay(5);
      }
      delay(10);
      wifiClient.stop();
    }
    delay(10);
  }
}

void ssidEepromWrite()
{
  EEPROM.begin(512);
  for (int i = 0; i < ssidLen; i++)
  {
    EEPROM.write(0x04 + i, ssid[i]);
  }
  EEPROM.write(0x01, 'f');
  writeIntIntoEEPROM(0x02, ssidLen);
  EEPROM.commit();
}

void passEepromWrite()
{
  EEPROM.begin(512);
  for (int i = 0; i < passLen; i++)
  {
    EEPROM.write(0x36 + i, pass[i]);
  }
  EEPROM.write(0x33, 'f');
  writeIntIntoEEPROM(0x34, passLen);
  EEPROM.commit();
}

void urlEepromWrite() //end of this entry would be: 78+100=178 =approx= 200.
{
  EEPROM.begin(512);
  for (int i = 0; i < urlLen; i++)
  {
    EEPROM.write(0x78 + i, url[i]);
  }
  EEPROM.write(0x74, 'f');
  writeIntIntoEEPROM(0x75, urlLen);
  EEPROM.commit();
}

void writeIntIntoEEPROM(int address, int number)
{
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}

void flashEeprom()
{
  EEPROM.begin(512);
  for (int i = 0; i < 512; i++)
  {
    EEPROM.write(0x00 + i, '.');
  }
  writeIntIntoEEPROM(0x02, 1);
  writeIntIntoEEPROM(0x34, 1);
  writeIntIntoEEPROM(0x66, 1);
  writeIntIntoEEPROM(0x75, 1);
  EEPROM.commit();
}
