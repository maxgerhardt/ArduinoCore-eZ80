/*
  Arduino.h - Main include file for the Arduino SDK
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef Arduino_h
#define Arduino_h

#include "api/ArduinoAPI.h"

#ifdef __cplusplus
extern "C"{
#endif

#define interrupts() __asm("ei")
#define noInterrupts() __asm("di")

// avr-libc defines _NOP() since 1.6.2
#ifndef _NOP
#define _NOP() do { __asm("nop"); } while (0)
#endif

#define NOT_A_PIN 255
#define NOT_A_PORT 255

#define NOT_ON_TIMER 0
#define TIMER4 1
#define TIMER5 2

#define digitalPinToPort(pin) ( (pin < NUM_TOTAL_PINS) ? digital_pin_to_port[pin] : NOT_A_PIN )
#define digitalPinToBitPosition(pin) ( (pin < NUM_TOTAL_PINS) ? digital_pin_to_bit_position[pin] : NOT_A_PIN )
#define digitalPinToBitMask(pin) ( (pin < NUM_TOTAL_PINS) ? digital_pin_to_bit_mask[pin] : NOT_A_PIN )
#define digitalPinToTimer(pin) ( (pin < NUM_TOTAL_PINS) ? digital_pin_to_timer[pin] : NOT_ON_TIMER )
#define analogPinToBitPosition(pin) ( (digitalPinToAnalogInput(pin) != NOT_A_PIN) ? digital_pin_to_bit_position[pin + ANALOG_INPUT_OFFSET] : NOT_A_PIN )
#define analogPinToBitMask(pin) ( (digitalPinToAnalogInput(pin) != NOT_A_PIN) ? digital_pin_to_bit_mask[pin + ANALOG_INPUT_OFFSET] : NOT_A_PIN )

#define portToPortStruct(port) ( (port < NUM_TOTAL_PORTS) ? ((PORT_t *)&PORTA + port) : NULL)
#define digitalPinToPortStruct(pin) ( (pin < NUM_TOTAL_PINS) ? ((PORT_t *)&PORTA + digitalPinToPort(pin)) : NULL)
#define getPINnCTRLregister(port, bit_pos) ( ((port != NULL) && (bit_pos < NOT_A_PIN)) ? ((volatile uint8_t *)&(port->PIN0CTRL) + bit_pos) : NULL )
#define digitalPinToInterrupt(P) (P)

#define portOutputRegister(P) ( (volatile uint8_t *)( &portToPortStruct(P)->OUT ) )
#define portInputRegister(P) ( (volatile uint8_t *)( &portToPortStruct(P)->IN ) )
#define portModeRegister(P) ( (volatile uint8_t *)( &portToPortStruct(P)->DIR ) )

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
/* cpp inports todo */
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "pins_arduino.h"

#ifdef __cplusplus
} // extern "C"
#endif

#endif