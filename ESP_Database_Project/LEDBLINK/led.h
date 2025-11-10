/*
 * ============================================================================
 *  File:           led.h
 *  Project:        LedBlink
 *  Author:         Mark P.
 *  Date:           2025-Nov-09
 *
 *  Purpose (Appendix A Example):
 *    Declare a simple Led class that abstracts a digital output pin with
 *    init(), on(), and off() methods for driving an LED.
 *
 *  Hardware/Board:
 *    <ESP8266 NodeMCU / ESP32 / Arduino Uno, etc.>
 *
 *  Pin Usage:
 *    The LED pin is provided via constructor (e.g., D6 or 6 depending on board).
 *
 *  Dependencies:
 *    - Arduino.h
 *
 *  Notes:
 *    - The constructor stores the pin and calls init().
 * ============================================================================
 */


#ifndef MY_LED_H
#define MY_LED_H
#include <Arduino.h>
class Led {
  
  private:
    byte pin;
    
  public:
    Led(byte pin);
    void init();
    void on();
    void off();
};
#endif