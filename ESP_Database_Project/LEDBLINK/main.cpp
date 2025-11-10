/*
 * ============================================================================
 *  File:           main.cpp
 *  Project:        LedBlink (uses Led class from led.h / led.cpp)
 *  Author:         Mark P.
 *  Date:           2025-Nov-09
 *
 *  Purpose (Appendix A Example):
 *    Blink an external LED connected to D6 using the Led class abstraction.
 *    Demonstrates object-based GPIO control (init/on/off) on Arduino/ESP boards.
 *
 *  Hardware/Board:
 *    <ESP8266 NodeMCU / ESP32 / Arduino Uno, etc.>
 *    - Supply Voltage: <3.3V / 5V>
 *
 *  Pin Assignments:
 *    LED_PIN = D6   (ESP8266 NodeMCU: D6 = GPIO12) 
 *    For Arduino Uno/Nano: use pin 6 instead of D6.
 *
 *  Wiring:
 *    D6 (or 6)  ->  220 resistor  -> LED anode (long leg)
 *    LED cathode (short leg) -> GND
 *
 *  Dependencies:
 *    - Arduino framework
 *    - led.h / led.cpp (class Led: init(), on(), off())
 *
 *  Build/Upload (PlatformIO):
 *    - Select the correct environment in the VS Code status bar.
 *    - Optionally set upload_port in platformio.ini if multiple boards attached.
 *
 *  Assumptions & Notes:
 *    - The Led constructor calls init(), so setup() has no extra work.
 *    - Active-HIGH logic: on() drives the pin HIGH to light the LED.
 *
 *  Revision History:
 *    v1.0  <2025-Nov-09>  Initial version.
 * ============================================================================
 */
#include <Arduino.h>
#include "led.h"   // uses class Led declared in led.h

#ifndef LED_PIN
#define LED_PIN D6   // ESP8266/ESP32 boards: D6 works; on Uno use 6
#endif

Led led(LED_PIN);

void setup() {
  // Led constructor already calls init()
}

void loop() {
  led.on();
  delay(100);
  led.off();
  delay(1000);
}
