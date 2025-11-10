/*
 * ---------------------------------------------------------------------------
 * Project/Program Name : ESP8266 NTP ISO Time Helper
 * File Name            : ntp_time_iso.ino
 * Author               : Mark P.
 * Date                 : Nov 09, 2025
 * Version              : 1.0.1
 *
 * Purpose:
 *   Fetch UTC time via SNTP on an ESP8266 and return an ISO-8601 timestamp
 *   string shifted by a fixed hour offset (NTP_ADD_HOURS). Provides a small
 *   Wi-Fi reconnect helper and optional trailing 'Z' tag.
 *
 * Inputs:
 *   - WiFi connection (must be configured elsewhere in the sketch)
 *   - Compile-time macro NTP_ADD_HOURS (int, hours to add to UTC; e.g., -8)
 *   - Compile-time macro APPEND_Z (0/1; when 1, appends 'Z' to the string)
 *   - getTimeIsoUtc(const String& tzRegion, String& outIso):
 *       tzRegion: Ignored by this implementation (reserved for future use)
 *       outIso  : Output parameter that receives the formatted timestamp
 *
 * Outputs:
 *   - Returns 'bool' from getTimeIsoUtc: true on success, false on failure
 *   - Populates 'outIso' with "YYYY-MM-DDTHH:MM:SS" (and optionally 'Z')
 *   - Serial debug messages describing Wi-Fi/NTP status
 *
 * Example Application:
 *   String iso; 
 *   if (getTimeIsoUtc("America/Los_Angeles", iso)) {
 *     Serial.println(iso);  // e.g., "2025-11-10T02:23:50"
 *   } else {
 *     Serial.println("Time fetch failed");
 *   }
 *
 * Dependencies:
 *   - Arduino core for ESP8266
 *   - <ESP8266WiFi.h> for networking
 *   - <time.h> for SNTP/time functions
 *   - Network access to "pool.ntp.org", "time.nist.gov", "time.google.com"
 *
 * Usage Notes:
 *   - Uses a fixed offset (NTP_ADD_HOURS); no automatic DST handling.
 *   - Blocks while waiting for valid SNTP time (up to ~12 seconds).
 *   - APPEND_Z should stay 0 when using offsets (since 'Z' denotes UTC).
 *   - ensureWifi() retries for ~8 seconds; adjust if needed for your network.
 * ---------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>

// ================== CONFIG ==================
// Apply this offset (hours) to NTP's UTC result.
//   Example: PST (standard, not daylight) = UTC-8
#ifndef NTP_ADD_HOURS
#define NTP_ADD_HOURS -8   // negative values subtract hours from UTC
#endif

// Append 'Z' at the end of the formatted ISO string?
//   'Z' indicates the string is *in UTC*. Keep this 0 when using offsets.
#ifndef APPEND_Z
#define APPEND_Z 0
#endif
// ============================================

/**
 * @brief Ensure Wi-Fi is connected, retrying for a limited time.
 *
 * @param ms   Maximum time to wait in milliseconds (default ~8s).
 * @return true  if already connected or connection becomes available.
 * @return false if not connected within the timeout window.
 */
static bool ensureWifi(uint32_t ms = 8000) {
  // Fast path: already connected
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.print(F("[ntp] Reconnecting WiFi"));
  uint32_t t0 = millis();

  // Poll the Wi-Fi status until connected or time expires
  while (millis() - t0 < ms) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F(" OK"));
      return true;
    }
    Serial.print('.');   // progress indicator
    delay(300);          // short backoff between polls
  }

  // Timed out
  Serial.println(F(" FAIL"));
  return false;
}

/**
 * @brief Get an ISO-8601 time string based on NTP (UTC) with a fixed hour offset.
 *
 * This function:
 *   1) Verifies Wi-Fi connectivity.
 *   2) Boots SNTP using common time servers (UTC base).
 *   3) Waits until the system time is valid.
 *   4) Applies the compile-time offset NTP_ADD_HOURS.
 *   5) Formats the result as "YYYY-MM-DDTHH:MM:SS" (optionally with 'Z').
 *
 * @param tzRegion  Reserved for future use (ignored).
 * @param outIso    Output string that receives the formatted timestamp.
 * @return true on success, false on failure (e.g., Wi-Fi/NTP timeout).
 */
bool getTimeIsoUtc(const String& /*tzRegion*/, String& outIso) {
  outIso = "";  // clear output

  // 1) Ensure Wi-Fi is connected before talking to NTP
  if (!ensureWifi()) {
    Serial.println(F("[ntp] WiFi not connected"));
    return false;
  }

  // 2) Configure SNTP with three well-known servers (redundancy).
  //    Offsets are set to zero here; we apply our own offset later.
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

  // 3) Wait until the SNTP time becomes valid.
  //    Use a minimum epoch threshold (2021-01-01) to detect readiness.
  const time_t MIN_VALID = 1609459200UL; // 2021-01-01 00:00:00 UTC
  time_t now = 0;
  uint32_t t0 = millis();

  // Poll for up to ~12s; many networks respond much faster.
  while ((millis() - t0) < 12000) {
    now = time(nullptr);       // current epoch seconds (UTC)
    if (now > MIN_VALID) break;  // time looks valid, stop waiting
    delay(250);                  // small backoff before next poll
  }

  // If we never crossed the threshold, SNTP likely failed or is too slow.
  if (now <= MIN_VALID) {
    Serial.println(F("[ntp] timeout waiting for SNTP"));
    return false;
  }

  // 4) Apply the user-selected fixed offset (in hours).
  //    Casting to long avoids overflow on platforms where 'int' is 16-bit.
  long offsetSec = (long)NTP_ADD_HOURS * 3600L;
  time_t shifted = now + offsetSec;

  // 5) Convert to broken-down time in UTC space. We use gmtime()
  //    because we've already incorporated the offset into 'shifted'.
  struct tm* t = gmtime(&shifted);

  // Format as ISO-8601 "YYYY-MM-DDTHH:MM:SS"
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", t);

  // Store in the Arduino String
  outIso = String(buf);

  // Optional: append 'Z' (only appropriate when representing pure UTC)
  #if APPEND_Z
    outIso += 'Z';
  #endif

  // Debug print so you can see the final representation and offset used
  Serial.print("[ntp] ISO (");
  Serial.print(NTP_ADD_HOURS);
  Serial.print("h): ");
  Serial.println(outIso);

  return true;
}
