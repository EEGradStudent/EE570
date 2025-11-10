#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "sendRequest.h"

// --------- USER SETTINGS ----------
const char* WIFI_SSID = "Mark P";
const char* WIFI_PASS = "testingwifi";

const String SERVER_BASE = "https://markpulido.io/api"; // Hostinger /api folder
const String POST_PATH  = "/ingest.php";                  // endpoint that calls sp_insert_sensor_data

// Pins (NodeMCU v2 example)
const int PIN_TRIG      = D5;  // HC-SR04 trig
const int PIN_ECHO      = D1;  // HC-SR04 echo
const int PIN_BTN_ULTRA = D3;  // pushbutton to GND (INPUT_PULLUP)
const int PIN_BTN_SOUND = D7;  // pushbutton to GND (INPUT_PULLUP)
const int PIN_SOUND     = A0;  // MAX4466 analog out

// Default timezone if user just hits Enter on Serial prompt
String tzRegion = "America/Los_Angeles";
// ----------------------------------

extern bool getTimeIsoUtc(const String& tzRegion, String& outIsoUtc);

// app state
enum NodeSel { NODE_NONE=0, NODE_ULTRA=1, NODE_SOUND=2 };
String nodeUltraName = "Ultrasonic_Sensor";
String nodeSoundName = "Sound_Sensor_MAX4466";

unsigned long lastUltraMs = 0, lastSoundMs = 0;
const unsigned long DEBOUNCE = 250;

// ========= REQUIRED FUNCTIONS =========
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

bool read_time(String& isoUtcOut) {
  Serial.print("Getting time for TZ: "); Serial.println(tzRegion);
  return getTimeIsoUtc(tzRegion, isoUtcOut);
}

float read_sensor_1() { // Ultrasonic HC-SR04, returns distance in cm
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 30000UL); // 30ms timeout
  if (duration == 0) return NAN;
  float cm = duration * 0.0343f / 2.0f;
  return cm;
}

float read_sensor_2() { // MAX4466 sound level, crude relative dB
  const int N = 200;
  long sum = 0;
  for (int i=0;i<N;i++) { sum += analogRead(PIN_SOUND); delayMicroseconds(200); }
  float adc = (float)sum / N;            // ~0..1023
  float level = fabs(adc - 512.0f);
  float db = 20.0f * log10f(max(level, 1.0f)); // relative “dB-like”
  if (!isfinite(db)) db = 0.0f;
  return db;
}

bool transmit(NodeSel who, const String& isoUtc, float dist_cm, float sound_db) {
  int code; String resp;
  String node = (who == NODE_ULTRA) ? nodeUltraName : nodeSoundName;
  bool ok = postToServer(SERVER_BASE, POST_PATH, node, isoUtc, tzRegion,
                         dist_cm, sound_db, code, resp);
  Serial.printf("POST -> %d\n", code);
  Serial.println(resp);
  return ok && code >= 200 && code < 300;
}

void check_error(bool ok) {
  if (!ok) Serial.println("[ERROR] transmit failed");
  else     Serial.println("[OK] data sent");
}
// =====================================

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

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BTN_ULTRA, INPUT_PULLUP);
  pinMode(PIN_BTN_SOUND, INPUT_PULLUP);

  Serial.println("\nBooting...");
  promptTimeZone();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  connectionDetails();  // from sendRequest.h
}

void loop() {
  NodeSel who = check_switch();
  if (who == NODE_NONE) { delay(25); return; }

  float dist_cm = 0.0f;
  float sound_db = 0.0f;

  if (who == NODE_ULTRA) {
    dist_cm = read_sensor_1();
    sound_db = 0.0f;                  // send only ultrasonic
  } else { // NODE_SOUND
    sound_db = read_sensor_2();
    dist_cm = 0.0f;                   // send only microphone
  }
  Serial.printf("dist=%.2f cm, sound=%.2f dB\n", dist_cm, sound_db);

  String isoUtc;
  if (!read_time(isoUtc)) {
    Serial.println("[ERROR] timeapi.io fetch failed");
    return;
  }
  Serial.print("ISO UTC: "); Serial.println(isoUtc);

  bool sent = transmit(who, isoUtc, dist_cm, sound_db);
  check_error(sent);

  delay(500); // guard against repeats if button held
}
