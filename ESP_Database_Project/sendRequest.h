#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

// Declaration only
void connectionDetails();

bool postToServer(
  const String& baseUrl,   // e.g. "https://your-domain.com/api"
  const String& path,      // e.g. "/ingest.php"
  const String& nodeName,
  const String& isoUtc,    // "2025-11-10T19:30:00Z"
  const String& tzRegion,  // "America/Los_Angeles"
  float distance_cm,
  float sound_db,
  int& httpCodeOut,
  String& bodyOut
);
