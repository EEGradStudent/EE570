#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include "sendRequest.h"

// Single, unique definition
void connectionDetails() {
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.print("Channel:\t");
  Serial.println(WiFi.channel());
  Serial.print("RSSI:\t");
  Serial.println(WiFi.RSSI());
  Serial.print("DNS IP:\t");
  Serial.println(WiFi.dnsIP(1));
  Serial.print("Gateway IP:\t");
  Serial.println(WiFi.gatewayIP());
  Serial.println("--------------------");
}

// helper
static String urlEncode(const String& s) {
  String out; out.reserve(s.length()*3);
  const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = (unsigned char)s[i];
    if (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~') out += char(c);
    else { out += '%'; out += hex[(c>>4)&0xF]; out += hex[c&0xF]; }
  }
  return out;
}

// NOTE: do NOT mark this function 'static' and do NOT put it inside a namespace.
bool postToServer(
  const String& baseUrl,
  const String& path,
  const String& nodeName,
  const String& isoUtc,
  const String& tzRegion,
  float distance_cm,
  float sound_db,
  int& httpCodeOut,
  String& bodyOut
) {
  httpCodeOut = 0; bodyOut = "";

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  String full = baseUrl + path; // e.g. https://domain.com/api/ingest.php
  if (!https.begin(*client, full)) return false;

  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "node_name="     + urlEncode(nodeName) +
                "&measured_iso=" + urlEncode(isoUtc) +
                "&tz_region="    + urlEncode(tzRegion) +
                "&distance_cm="  + String(distance_cm, 2) +
                "&sound_db="     + String(sound_db, 2);

  httpCodeOut = https.POST(body);
  bodyOut = https.getString();
  https.end();
  return (httpCodeOut > 0);
}
