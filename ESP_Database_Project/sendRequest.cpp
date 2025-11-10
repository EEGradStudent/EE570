/*
* ------------------------------------------------------------------------------------------------
* Project/Program Name : ESP8266 Dual Sensor Demo - Networking Helpers                           *
* File Name            : sendRequest.cpp                                                         *
* Author               : Mark P.                                                            *
* Date                 : 09 NOV 2025                                                            *
* Version              : 1.0.1                                                                   *
*                                                                                               *
* Purpose:                                                                                       *
*   Provide helper routines for the ESP8266 sketch:                                              *
*   - connectionDetails(): print current Wi-Fi connection info to Serial.                        *
*   - postToServer(): perform an HTTPS POST (URL-encoded form) to a backend API.                 *
*                                                                                               *
* Inputs:                                                                                        *
*   connectionDetails(): none (reads current Wi-Fi state).                                       *
*   postToServer():                                                                              *
*     - baseUrl   : Base URL (e.g., "https://example.com/api").                                  *
*     - path      : Path appended to baseUrl (e.g., "/ingest.php").                              *
*     - nodeName  : Logical node identifier string.                                              *
*     - isoUtc    : ISO-8601 UTC timestamp string.                                               *
*     - tzRegion  : IANA time zone string (e.g., "America/Los_Angeles").                         *
*     - distance_cm : Distance reading in centimeters.                                           *
*     - sound_db    : Relative sound level reading.                                              *
*     - httpCodeOut : (out) HTTP status code returned by server.                                 *
*     - bodyOut     : (out) Response payload as a String.                                        *
*                                                                                               *
* Outputs:                                                                                       *
*   - Serial diagnostics for connectionDetails().                                                 *
*   - Returns true/false from postToServer() indicating whether an HTTP transaction occurred;     *
*     HTTP status is placed in httpCodeOut, and server body in bodyOut.                          *
*                                                                                               *
* Example Application:                                                                           *
*   Call connectionDetails() after connecting to Wi-Fi.                                          *
*   Use postToServer() to send sensor readings to a web API using URL-encoded form data.         *
*                                                                                               *
* Dependencies:                                                                                  *
*   - Arduino core for ESP8266                                                                   *
*   - <ESP8266WiFi.h>, <WiFiClientSecureBearSSL.h>, <ESP8266HTTPClient.h>                        *
*   - "sendRequest.h" (declarations for these functions)                                         *
*                                                                                               *
* Usage Notes:                                                                                   *
*   - TLS is set to "insecure" (certificate not validated). For production, configure proper     *
*     certificate validation (fingerprint or CA cert).                                           *
*   - Body fields are URL-encoded to be safe for form submission.                                *
*   - Keep the code logic identical to the original (comments added only).                       *
* ------------------------------------------------------------------------------------------------
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#include "sendRequest.h"

// Print a summary of the current Wi-Fi connection.
// Single, unique definition so sketches can call it from setup().
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

// Percent-encode a string for application/x-www-form-urlencoded bodies.
// Leaves unreserved chars alone; encodes others as %HH.
static String urlEncode(const String& s) {
  String out; out.reserve(s.length()*3);
  const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = (unsigned char)s[i];
    if (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~') {
      out += char(c);                  // unreserved, keep as-is
    } else {
      out += '%';                      // encode as %HH
      out += hex[(c>>4)&0xF];
      out += hex[c&0xF];
    }
  }
  return out;
}

// Perform an HTTPS POST with URL-encoded form data.
// NOTE: do NOT mark this function 'static' and do NOT put it inside a namespace.
// Returns true if an HTTP transaction was attempted; status is placed in httpCodeOut.
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
  httpCodeOut = 0; 
  bodyOut = "";

  // Use a secure client; setInsecure() skips certificate verification.
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  HTTPClient https;

  // Compose full URL (e.g., https://domain.com/api/ingest.php).
  String full = baseUrl + path;
  if (!https.begin(*client, full)) return false; // begin() failed to init

  // Send classic form data.
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Build URL-encoded body.
  String body = "node_name="     + urlEncode(nodeName) +
                "&measured_iso=" + urlEncode(isoUtc) +
                "&tz_region="    + urlEncode(tzRegion) +
                "&distance_cm="  + String(distance_cm, 2) +
                "&sound_db="     + String(sound_db, 2);

  // Execute POST and collect results.
  httpCodeOut = https.POST(body);
  bodyOut = https.getString();

  // Always end() to free resources.
  https.end();

  // Return whether we at least attempted an HTTP request (httpCodeOut > 0).
  return (httpCodeOut > 0);
}
