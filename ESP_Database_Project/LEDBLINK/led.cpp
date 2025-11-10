/*
 * ============================================================================
 *  File:           led.cpp
 *  Project:        LedBlink
 *  Author:         <Your Name>
 *  Date:           <YYYY-MM-DD>
 *
 *  Purpose (Appendix A Example):
 *    Implement the Led class methods to control a GPIO pin as an LED.
 *
 *  Implementation Notes:
 *    - init() sets the pinMode to OUTPUT and ensures the LED starts off (LOW).
 *    - on() drives the pin HIGH; off() drives it LOW (active-high wiring).
 *    - The constructor saves the pin number and calls init().
 * ============================================================================
 */

#include "Led.h"
Led::Led(byte pin) {
  // Use 'this->' to make the difference between the
  // 'pin' attribute of the class and the 
  // local variable 'pin' created from the parameter.
  this->pin = pin;
  init();
}
void Led::init() {
  pinMode(pin, OUTPUT);
  // Always try to avoid duplicate code.
  // Instead of writing digitalWrite(pin, LOW) here,
  // call the function off() which already does that
  off();
}
void Led::on() {
  digitalWrite(pin, HIGH);
}
void Led::off() {
  digitalWrite(pin, LOW);
}