/*
TM16XX.cpp - Library implementation for TM16XX.

Copyright (C) 2011 Ricardo Batista (rjbatista <at> gmail <dot> com)

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TM16XX.h"
#include "string.h"
#define DEFINTENSITY ((byte)7)

TM16XX::TM16XX(byte dataPin, byte clockPin, byte strobePin, byte displays, boolean activateDisplay,
	byte intensity)
{
  this->dataPin = dataPin;
  this->clockPin = clockPin;
  this->strobePin = strobePin;
  this->displays = displays;

  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(strobePin, OUTPUT);

  digitalWrite(strobePin, HIGH);
  digitalWrite(clockPin, HIGH);

  sendCommand(0x40);
  sendCommand((byte)(0x80 | (activateDisplay ? 8 : 0) | min(DEFINTENSITY, intensity)));

  digitalWrite(strobePin, LOW);
  #ifdef ARDUINO_ARCH_ESP32
	delayMicroseconds(2);
  #endif
  send(0xC0);
  for (int i = 0; i < 16; i++) {
    send(0x00);
  }
  digitalWrite(strobePin, HIGH);
}
void TM16XX::setupDisplay(boolean active, byte intensity)
{
  sendCommand((byte)(0x80 | (active ? 8 : 0) | min(DEFINTENSITY, intensity)));

  // necessary for the TM1640
  digitalWrite(strobePin, LOW);
  digitalWrite(clockPin, LOW);
  #ifdef ARDUINO_ARCH_ESP32
	delayMicroseconds(2);
  #endif
  digitalWrite(clockPin, HIGH);
  digitalWrite(strobePin, HIGH);
}

void TM16XX::setDisplayDigit(byte digit, byte pos, boolean dot, const byte numberFont[])
{
  sendChar(pos, numberFont[digit & 0xF], dot);
}

void TM16XX::setDisplayToError()
{
    setDisplay(ERROR_DATA, 8);

	for (int i = 8; i < displays; i++) {
	    clearDisplayDigit(i, 0);
	}
}

void TM16XX::clearDisplayDigit(byte pos, boolean dot)
{
  sendChar(pos, 0, dot);
}

void TM16XX::setDisplay(const byte values[], unsigned int size)
{
  for (int i = 0; i < size; i++) {
    sendChar(i, values[i], 0);
  }
}

void TM16XX::clearDisplay()
{
  for (int i = 0; i < displays; i++) {
    sendData(i << 1, 0);
  }
}

void TM16XX::setDisplayToString(const char* string, const word dots, const byte pos, const byte font[])
{
  for (int i = 0; i < displays - pos; i++) {
  	if (string[i] != '\0') {
	  sendChar(i + pos, font[string[i] - 32], (dots & (1 << (displays - i - 1))) != 0);
	} else {
	  break;
	}
  }
}

void TM16XX::setDisplayToString(const String string, const word dots, const byte pos, const byte font[])
{
  int stringLength = string.length();

  for (int i = 0; i < displays - pos; i++) {
    if (i < stringLength) {
      sendChar(i + pos, font[string.charAt(i) - 32], (dots & (1 << (displays - i - 1))) != 0);
    } else {
      break;
    }
  }
}

void TM16XX::sendCommand(byte cmd)
{
  digitalWrite(strobePin, LOW);
  #ifdef ARDUINO_ARCH_ESP32
	delayMicroseconds(2);
  #endif
  send(cmd);
  digitalWrite(strobePin, HIGH);
}

void TM16XX::sendData(byte address, byte data)
{
  sendCommand(0x44);
  digitalWrite(strobePin, LOW);
  send(0xC0 | address);
  send(data);
  digitalWrite(strobePin, HIGH);
}

void TM16XX::send(byte data)
{
  for (int i = 0; i < 8; i++) {
    digitalWrite(clockPin, LOW);
    #ifdef ARDUINO_ARCH_ESP32
		delayMicroseconds(2);
    #endif
    digitalWrite(dataPin, data & 1 ? HIGH : LOW);
    #ifdef ARDUINO_ARCH_ESP32
		delayMicroseconds(2);
    #endif
    data >>= 1;
    digitalWrite(clockPin, HIGH);
    #ifdef ARDUINO_ARCH_ESP32
		delayMicroseconds(2);
    #endif
  }
}

byte TM16XX::receive()
{
  byte temp = 0;

  // Pull-up on
  pinMode(dataPin, INPUT);
  digitalWrite(dataPin, HIGH);

  for (int i = 0; i < 8; i++) {
    temp >>= 1;

    digitalWrite(clockPin, LOW);
	#ifdef ARDUINO_ARCH_ESP32
		delayMicroseconds(2);
	#endif
    if (digitalRead(dataPin)) {
      temp |= 0x80;
    }

    digitalWrite(clockPin, HIGH);
  }

  // Pull-up off
  pinMode(dataPin, OUTPUT);
  digitalWrite(dataPin, LOW);
  #ifdef ARDUINO_ARCH_ESP32
	delayMicroseconds(2);
  #endif
  
  return temp;
}
void TM16XX::setText(String &str)
{
	byte pos = 0;
        for (int i = 0; i < 8; i++) {
          if (str[i] != '\0' && str[i] != '.') {
            if (str[i] == 0xb0) {
              sendChar(pos, (byte)0b01100011,false);
            } else {
               sendChar(pos, FONT_DEFAULT[str[i] - 32], str[i+1] == '.');
            }
           ++pos;
          }
          if (str[i] == '\0') break;
         }
}

#if !defined(ARDUINO) || ARDUINO < 100
// empty implementation instead of pure virtual for older Arduino IDE
void TM16XX::sendChar(byte pos, byte data, boolean dot) {}
#endif

