/*
* ------------------------------------------------------------------------------------------------
* Project/Program Name : ESP8266 Dual Sensor Demo (Ultrasonic + Sound)                          *
* File Name            : main.cpp                                                               *
* Author               : Mark P.                                                                *
* Date                 : 09 NOV 2025                                                            * 
* Version              : 1.0.1                                                                  *
*                                                                                               *
* Purpose:                                                                                      *
*   Read an HC-SR04 ultrasonic distance sensor and a MAX4466 microphone (relative level)        *
*   with an ESP8266 (NodeMCU v2). Prompt for an IANA time zone, obtain an ISO-8601 UTC          *
*   timestamp, and POST a measurement to a web API endpoint.                                    *
*                                                                                               *
* Inputs:                                                                                       *
*   - Pushbutton on D3 selects Ultrasonic sample (active LOW, INPUT_PULLUP).                    *
*   - Pushbutton on D7 selects Sound sample (active LOW, INPUT_PULLUP).                         *
*   - HC-SR04: TRIG=D5, ECHO=D1.                                                                *
*   - MAX4466: analog OUT=A0.                                                                   *
*   - Wi-Fi credentials (WIFI_SSID, WIFI_PASS).                                                 *
*   - Time zone via Serial prompt (IANA string, default "America/Los_Angeles").                 *
*                                                                                               *
* Outputs:                                                                                      *
*   - HTTP POST to SERVER_BASE + POST_PATH with sensor reading, ISO UTC time, and TZ.           *
*   - Serial monitor diagnostics at 9600 baud.                                                  *
*                                                                                               *
* Example Application:                                                                          *
*   Press D3 to send a distance (cm) reading; press D7 to send a relative sound level          *
*   (“dB-like”). Each payload includes the current ISO UTC timestamp and the selected TZ.        *
*                                                                                               *
* Dependencies:                                                                                 *
*   - Arduino core for ESP8266                                                                  *
*   - <ESP8266WiFi.h>                                                                           *
*   - "sendRequest.h" providing postToServer() and connectionDetails()                          *
*   - getTimeIsoUtc() implementation (talks to timeapi.io)                                      *
*                                                                                               *
* Usage Notes:                                                                                  *
*   - Buttons are debounced in software (250 ms).                                               *
*   - Inputs use INPUT_PULLUP; wire buttons to GND.                                             *
*   - Replace Wi-Fi creds and server endpoint strings with your own.                            *
*   - The “dB” value is a crude, relative estimate (not calibrated SPL).                        *
* ------------------------------------------------------------------------------------------------
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "sendRequest.h"

// --------- USER SETTINGS ----------
// Wi-Fi credentials used by the ESP8266 station interface.
const char* WIFI_SSID = "Mark P";
const char* WIFI_PASS = "testingwifi";

// REST endpoint configuration.
// SERVER_BASE is the base URL; POST_PATH is appended for the ingest endpoint.
const String SERVER_BASE = "https://markpulido.io/api"; // Hostinger /api folder
const String POST_PATH  = "/ingest.php";                 // endpoint that calls sp_insert_sensor_data

// GPIO assignments for NodeMCU v2 board layout.
const int PIN_TRIG      = D5;  // HC-SR04 trig
const int PIN_ECHO      = D1;  // HC-SR04 echo
const int PIN_BTN_ULTRA = D3;  // pushbutton to GND (INPUT_PULLUP)
const int PIN_BTN_SOUND = D7;  // pushbutton to GND (INPUT_PULLUP)
const int PIN_SOUND     = A0;  // MAX4466 analog out

// Default IANA time zone used when user presses ENTER at the Serial prompt.
String tzRegion = "America/Los_Angeles";
// ----------------------------------

// Implemented elsewhere; fills outIsoUtc with ISO-8601 UTC time for tzRegion.
// Returns true on success.
extern bool getTimeIsoUtc(const String& tzRegion, String& outIsoUtc);

// ----- Application state -----
// Indicates which sensor to sample based on button press.
enum NodeSel { NODE_NONE=0, NODE_ULTRA=1, NODE_SOUND=2 };

// Identifiers sent to the server so it can distinguish node types.
String nodeUltraName = "Ultrasonic_Sensor";
String nodeSoundName = "Sound_Sensor_MAX4466";

// Simple debounce bookkeeping for each button.
unsigned long lastUltraMs = 0, lastSoundMs = 0;
const unsigned long DEBOUNCE = 250;

// ========= REQUIRED FUNCTIONS =========

// Check the two momentary switches and return which node to read.
// Buttons are active-LOW due to INPUT_PULLUP.
NodeSel check_switch() {
  // active LOW (INPUT_PULLUP)
  bool ultraPressed = (digitalRead(PIN_BTN_ULTRA) == LOW);
  bool soundPressed = (digitalRead(PIN_BTN_SOUND) == LOW);

  unsigned long now = millis();
  if (ultraPressed && (now - lastUltraMs > DEBOUNCE)) {
    lastUltraMs = now;
    return NODE_ULTRA;
  }
  if (soundPressed && (now - lastSoundMs > DEBOUNCE)) {
    lastSoundMs = now;
    return NODE_SOUND;
  }
  return NODE_NONE;
}

// Resolve current time (ISO-8601 UTC) using the configured tzRegion.
// Returns true on success.
bool read_time(String& isoUtcOut) {
  Serial.print("Getting time for TZ: "); Serial.println(tzRegion);
  return getTimeIsoUtc(tzRegion, isoUtcOut);
}

// Read HC-SR04 ultrasonic sensor and return distance in centimeters.
// Uses a 30 ms pulseIn timeout; returns NaN on timeout.
float read_sensor_1() { // Ultrasonic HC-SR04, returns distance in cm
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 30000UL); // 30ms timeout
  if (duration == 0) return NAN;
  float cm = duration * 0.0343f / 2.0f;
  return cm;
}

// Sample MAX4466 microphone on A0 and compute a crude, relative “dB-like” level.
// This is not calibrated SPL; it serves as a simple activity indicator.
float read_sensor_2() { // MAX4466 sound level, crude relative dB
  const int N = 200;               // number of samples to average
  long sum = 0;
  for (int i=0;i<N;i++) { sum += analogRead(PIN_SOUND); delayMicroseconds(200); }
  float adc = (float)sum / N;      // ~0..1023
  float level = fabs(adc - 512.0f);// AC component around mid-rail
  float db = 20.0f * log10f(max(level, 1.0f)); // relative “dB-like”
  if (!isfinite(db)) db = 0.0f;
  return db;
}

// Package and transmit a reading to the server.
// Returns true if HTTP status is 2xx.
bool transmit(NodeSel who, const String& isoUtc, float dist_cm, float sound_db) {
  int code; String resp;
  String node = (who == NODE_ULTRA) ? nodeUltraName : nodeSoundName;
  bool ok = postToServer(SERVER_BASE, POST_PATH, node, isoUtc, tzRegion,
                         dist_cm, sound_db, code, resp);
  Serial.printf("POST -> %d\n", code);
  Serial.println(resp);
  return ok && code >= 200 && code < 300;
}

// Log success/failure to Serial.
void check_error(bool ok) {
  if (!ok) Serial.println("[ERROR] transmit failed");
  else     Serial.println("[OK] data sent");
}
// =====================================

// Prompt the user once at boot for an IANA time zone.
// If the user just presses ENTER (or times out after 10 s), keep the default.
void promptTimeZone() {
  Serial.println();
  Serial.println(F("Enter IANA time zone (e.g., America/Los_Angeles)."));
  Serial.println(F("Press ENTER to keep default: "));
  String input = "";
  unsigned long t0 = millis();
  while (millis() - t0 < 10000) {  // 10s to type
    if (Serial.available()) {
      char c = Serial.read();
      if (c=='\r' || c=='\n') break;
      input += c;
    }
    delay(10);
  }
  input.trim();
  if (input.length() > 0) tzRegion = input;
  Serial.print("Using TZ: "); Serial.println(tzRegion);
}

void setup() {
  Serial.begin(9600);
  delay(300);

  // Configure GPIOs.
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BTN_ULTRA, INPUT_PULLUP);
  pinMode(PIN_BTN_SOUND, INPUT_PULLUP);

  Serial.println("\nBooting...");
  promptTimeZone();

  // Bring up Wi-Fi in station mode and connect to the configured network.
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  connectionDetails();  // from sendRequest.h: prints IP, RSSI, etc.
}

void loop() {
  // Poll buttons and decide which sensor to sample.
  NodeSel who = check_switch();
  if (who == NODE_NONE) { delay(25); return; }

  float dist_cm = 0.0f;
  float sound_db = 0.0f;

  // Read only the chosen sensor; send 0 for the other field.
  if (who == NODE_ULTRA) {
    dist_cm = read_sensor_1();
    sound_db = 0.0f;                  // send only ultrasonic
  } else { // NODE_SOUND
    sound_db = read_sensor_2();
    dist_cm = 0.0f;                   // send only microphone
  }
  Serial.printf("dist=%.2f cm, sound=%.2f dB\n", dist_cm, sound_db);

  // Resolve timestamp for the current time zone selection.
  String isoUtc;
  if (!read_time(isoUtc)) {
    Serial.println("[ERROR] timeapi.io fetch failed");
    return;
  }
  Serial.print("ISO UTC: "); Serial.println(isoUtc);

  // Transmit payload and report result.
  bool sent = transmit(who, isoUtc, dist_cm, sound_db);
  check_error(sent);

  // Simple guard against repeats when a button is held down.
  delay(500);
}
