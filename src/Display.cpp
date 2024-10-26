
/**
 * @file Display.cpp
 * @brief Implementation of the Display class for controlling a 7-segment LED display.
 * 
 * This file contains the implementation of the Display class, which provides methods
 * to initialize, control, and display data on a 7-segment LED display. The class 
 * supports displaying characters, numbers, and time, as well as providing functions 
 * for blinking and clearing the display.
 * 
 * @author Tomas
 * @version 2.5
 * @date 2023
 * 
 * @note This file is part of the ControlRiego-2.5 project.
 * 
 * @see Display.h
 */
#include "Display.h"

static char CharTab[] = { '0','1','2','3',
                          '4','5','6','7',
                          '8','9','A','b',
                          'C','d','E','F',
                          '-',' ','*','c',  // "-"," ",degree º,r,
                          '[',']','?','#',  // [,],?,triple -,
                          'H','G','L','Y',
                          'J','O','q','u',
                          'h','n','r','U',
                          'S','t','o','P'};

Display::Display(uint8_t clk,uint8_t data) : ledDisp(clk,data)
{
  #ifdef EXTRADEBUG
   Serial.println(F("DISPLAY: set brigth"));
  #endif
  ledDisp.set(BRIGHT_TYPICAL); 
  #ifdef EXTRADEBUG
   Serial.println(F("DISPLAY: init"));
  #endif
  ledDisp.init();
  #ifdef EXTRADEBUG
   Serial.println(F("DISPLAY: point"));
  #endif
  ledDisp.point(POINT_ON);
  #ifdef EXTRADEBUG
   Serial.println(F("DISPLAY: exit constructor"));
  #endif
}  

void Display::check(int veces)
{
  uint8_t t[5];
  t[4]=0;

  for(int repeat=0;repeat<veces;repeat++)
  {
    clearDisplay();
    for(int i=0;i<10;i++)
    {
      for(int j=0;j<4;j++)
        t[j]=i;
      printRaw(t);
      delay(300);
    }
    print("----");
  }
}

void Display::print(const char *str)
{
  uint8_t t[5];
  for(int i=0;i<4;i++){
    for(unsigned int j=0;j<sizeof(CharTab);j++){
      if(*(str+i) == CharTab[j]) {
        t[i] = j;
        break;
      }
      t[i] = 17;
    }
  }
  t[4] = 0;
  printRaw(t);
}

void Display::print(int n)
{
  ledDisp.point(POINT_OFF);
  ledDisp.display((int16_t) n);
}

void Display::printRaw(uint8_t text[])
{
  memcpy(actual,text,5);
  if (text[4] == 1) ledDisp.point(POINT_ON);
  else ledDisp.point(POINT_OFF);
  ledDisp.display((int8_t *)text);
}

void Display::blink(int veces)
{
  for (int i=0; i<veces; i++) {
    clearDisplay();
    delay(500);
    printRaw(actual);
    delay(500);
  }
}

void Display::clearDisplay()
{
    ledDisp.point(POINT_OFF);
    ledDisp.clearDisplay();
    ledDisp.point(POINT_ON);
}

void Display::printTime(int m,int s)
{
  uint8_t t[5];
  t[4] = 1;
  t[2] = s / 10;
  t[3] = s % 10;
  t[0] = m / 10;
  t[1] = m % 10;
  printRaw(t);
}

void Display::refreshDisplay(void)
{
  printRaw(actual);
}
