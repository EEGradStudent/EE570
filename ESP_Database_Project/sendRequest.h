/*
* ------------------------------------------------------------------------------------------------
* Project/Program Name : ESP8266 Dual Sensor Demo - Networking Declarations
* File Name            : sendRequest.h
* Author               : Mark P.
* Date                 : 09 NOV 2025
* Version              : 1.0.1
*
* Purpose:
*   Header for networking/utility helpers used by the ESP8266 sketch. Declares:
*     - connectionDetails(): prints Wi-Fi connection information to Serial.
*     - postToServer(): sends URL-encoded measurements to a backend over HTTPS.
*
* Inputs:
*   See function parameter docs below.
*
* Outputs:
*   See function descriptions below.
*
* Example Application:
*   After connecting to Wi-Fi, call connectionDetails() to log network info.
*   When a measurement is taken, call postToServer() with the timestamp,
*   time zone, and sensor values to send data to your API endpoint.
*
* Dependencies:
*   - Arduino core for ESP8266
*   - <Arduino.h>, <ESP8266WiFi.h>
*
* Usage Notes:
*   - This file contains declarations only; implementations live in sendRequest.cpp.
*   - Keep the function signatures unchanged so the sketch links correctly.
* ------------------------------------------------------------------------------------------------
*/

#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

// Declaration only: prints SSID, IP, RSSI, etc. to the Serial monitor.
void connectionDetails();

/**
 * Perform an HTTPS POST (implemented in sendRequest.cpp).
 *
 * @param baseUrl     Base URL, e.g., "https://markpulido.io/api"
 * @param path        Endpoint path, e.g., "/ingest.php"
 * @param nodeName    Logical node/sensor name to tag the reading
 * @param isoUtc      ISO-8601 UTC timestamp, e.g., "2025-11-10T19:30:00Z"
 * @param tzRegion    IANA time zone name, e.g., "America/Los_Angeles"
 * @param distance_cm Distance measurement in centimeters
 * @param sound_db    Relative sound level (not calibrated SPL)
 * @param httpCodeOut (out) HTTP status code returned by the server
 * @param bodyOut     (out) Response body as a String
 *
 * @return true if an HTTP transaction was attempted (status in httpCodeOut),
 *         false if setup/connection failed before sending.
 */
bool postToServer(
  const String& baseUrl,   // e.g. "https://markpulido.io/api"
  const String& path,      // e.g. "/ingest.php"
  const String& nodeName,
  const String& isoUtc,    // "2025-11-10T19:30:00Z"
  const String& tzRegion,  // "America/Los_Angeles"
  float distance_cm,
  float sound_db,
  int& httpCodeOut,
  String& bodyOut
);
