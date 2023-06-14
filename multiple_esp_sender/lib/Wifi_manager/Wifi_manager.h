#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "certificate.h"

// Bytes reserved for ssid and password in EEPROM
#define EEPROM_SIZE 97

// Connection settings for access point
const char* ap_ssid     = "FysioTherapy_setup";
const char* ap_password = "FysioTherapy";

// Positions of ssid and password in EEPROM
int ssid_len_start = 0;
int ssid_start = 1;
int passwd_len_start = 33;
int passwd_start = 34;

// Webserver on port 80 for inputting ssid and password if a connection cannot be made
WebServer server ( 80 );
char htmlResponse[3000];

// Class for managing the wifi connection
class Wifi_manager{
private:
  static void handleRoot() {
    // Handle root page
    snprintf ( htmlResponse, 3000,
  "<!DOCTYPE html>\
  <html lang=\"en\">\
    <head>\
      <meta charset=\"utf-8\">\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    </head>\
    <body>\
            <h1>Network Setup</h1>\
            <input type='text' name='ssid' placeholder='Network Name' id='ssid' size=10 autofocus> \
            <br><input type='password' name='password' placeholder='Network Password' id='password' size=10 autofocus> \
            <div>\
            <br><button id='save_button' onclick='save()'>Save</button>\
            </div>\
      <script>\
        var ssid;\
        var password;\
        function save(){\
          ssid = document.getElementById('ssid').value;\
          password = document.getElementById('password').value;\
          var xhttp = new XMLHttpRequest();\
          xhttp.open('GET', '/save?ssid=' + ssid + '&password=' + password, true);\
          xhttp.send();\
        }\
      </script>\
    </body>\
  </html>"); 

    server.send ( 200, "text/html", htmlResponse );  

  }

  // Handles saving the ssid and password to EEPROM
  static void handleSave() {

    if((server.arg("ssid")!= "") && (server.arg("password")!= "")){
      // SSID and password are entered. Save to EEPROM and reboot.
      if((server.arg("ssid").length() <= 32) && (server.arg("password").length() <= 63)){
        // values are not too large, so save them
        EEPROM.write(ssid_len_start, (byte)server.arg("ssid").length());
        for(unsigned int i = 0; i < server.arg("ssid").length(); i++){
          EEPROM.write(ssid_start+i, (byte)server.arg("ssid")[i]);
        }

        EEPROM.write(passwd_len_start, (byte)server.arg("password").length());
        for(unsigned int i = 0; i < server.arg("password").length(); i++){
          EEPROM.write(passwd_start+i, (byte)server.arg("password")[i]);
        }
      }
    }

    EEPROM.commit();
    ESP.restart();

  }

public:
  WiFiClientSecure secure_client;

  void begin(){
    EEPROM.begin(EEPROM_SIZE);
  }

  // Tries to connect to wifi. If it fails, it returns false and you should run the webserver
  bool connect_wifi(){
    WiFi.mode(WIFI_STA);
    if(EEPROM.read(ssid_len_start) <= 32){
      // SSID is set, don't start webserver but retreive the details from EEPROM and connect to wifi

      int ssid_length = EEPROM.read(ssid_len_start);

      // Read ssid from EEPROM
      String ssid = "";
      for(unsigned int i = 0; i < ssid_length; i++){
        ssid += (char)EEPROM.read(i+ssid_start);
      }

      int passwd_length = EEPROM.read(passwd_len_start);

      // Read password from EEPROM
      String passwd = "";
      for(unsigned int i = 0; i < passwd_length; i++){
        passwd += (char)EEPROM.read(i+passwd_start);
      }

      // Try to connect to wifi
      WiFi.begin(ssid.c_str(), passwd.c_str());

      unsigned long start_time = millis();
      while(WiFi.status() != WL_CONNECTED){
        delay(100);
        if(millis() >= (start_time + (1000*15))){
          break; // Stop trying to connect after 15 seconds
        }
      }
      if(WiFi.status() != WL_CONNECTED){
        // Assume no connection is possible. Stop connecting and enter AP mode
        return false;
      }
      this->secure_client.setCACert(test_root_ca); // Set the root certificate
      return true; // Return true only when connected to wifi
    }
    else{Serial.println("SSID too long."); return false;} // SSID length is too long, so it won't work anyway
  }

  // Runs the webserver
  void run_webserver(){
    Serial.println("Starting webserver");
    WiFi.softAP(ap_ssid, ap_password);
    // Serial.println("IP address: ");
    IPAddress ip = WiFi.softAPIP();
    // Serial.println(ip);

    server.on ( "/", handleRoot );
    server.on ("/save", handleSave);

    server.begin();
    // Serial.println ( "HTTP server started" );

    while(true){
      server.handleClient();
    }
  }
};

#endif // WIFI_MANAGER_H