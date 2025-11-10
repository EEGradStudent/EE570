#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>

// ================== CONFIG ==================
// Apply this offset (hours) to NTP UTC result.
// PST (standard time) = -8
#ifndef NTP_ADD_HOURS
#define NTP_ADD_HOURS -8   // subtract 8 hours
#endif

// Append 'Z' at the end? (Z means UTC; keep 0 when using offsets)
#ifndef APPEND_Z
#define APPEND_Z 0
#endif
// ============================================

static bool ensureWifi(uint32_t ms=8000) {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.print(F("[ntp] Reconnecting WiFi"));
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    if (WiFi.status() == WL_CONNECTED) { Serial.println(F(" OK")); return true; }
    Serial.print('.');
    delay(300);
  }
  Serial.println(F(" FAIL"));
  return false;
}

// Returns ISO-8601 string shifted by NTP_ADD_HOURS from UTC.
// Example (with -8h): "2025-11-10T02:23:50" (no trailing Z).
bool getTimeIsoUtc(const String& /*tzRegion*/, String& outIso) {
  outIso = "";

  if (!ensureWifi()) {
    Serial.println(F("[ntp] WiFi not connected"));
    return false;
  }

  // SNTP (UTC base)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

  // Wait until time is valid (> 2021-01-01 UTC)
  const time_t MIN_VALID = 1609459200UL; // 2021-01-01 00:00:00 UTC
  time_t now = 0;
  uint32_t t0 = millis();
  while ((millis() - t0) < 12000) {
    now = time(nullptr);
    if (now > MIN_VALID) break;
    delay(250);
  }
  if (now <= MIN_VALID) {
    Serial.println(F("[ntp] timeout waiting for SNTP"));
    return false;
  }

  // Apply fixed offset
  long offsetSec = (long)NTP_ADD_HOURS * 3600L; // e.g. -8 * 3600
  time_t shifted = now + offsetSec;

  // Format as ISO-8601 without 'Z'
  struct tm* t = gmtime(&shifted);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", t);

  outIso = String(buf);
#if APPEND_Z
  outIso += 'Z';  // only if you truly want to tag it as UTC (not recommended with offsets)
#endif

  Serial.print("[ntp] ISO (");
  Serial.print(NTP_ADD_HOURS);
  Serial.print("h): ");
  Serial.println(outIso);
  return true;
}
