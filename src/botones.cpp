
/**
 * @file botones.cpp
 * @brief This file contains functions for controlling LEDs and reading button inputs using shift registers.
 * 
 * The functions in this file are responsible for:
 * - Turning LEDs on and off.
 * - Initializing the LED and button control hardware.
 * - Reading the state of buttons.
 * - Handling debouncing of button inputs.
 * 
 * The file makes use of the following hardware components:
 * - 74HC595 shift register for controlling LEDs.
 * - CD4021B shift register for reading button inputs.
 * 
 * The main functions include:
 * - apagaLeds(): Turns off all LEDs.
 * - enciendeLeds(): Turns on all LEDs.
 * - loadDefaultSignal(): Displays a default signal pattern on LEDs.
 * - wifiClearSignal(): Displays a WiFi clear signal pattern on LEDs.
 * - initLeds(): Initializes the LED hardware and displays an initial pattern.
 * - initHC595(): Initializes the 74HC595 shift register.
 * - ledRGB(): Controls the RGB LED.
 * - led(): Controls individual LEDs.
 * - ledStatusId(): Checks the status of a specific LED.
 * - initCD4021B(): Initializes the CD4021B shift register.
 * - shiftInCD4021B(): Reads a byte of data from the CD4021B shift register.
 * - readInputs(): Reads the state of all buttons.
 * - testButton(): Tests the state of a specific button.
 * - parseInputs(): Parses the button inputs and handles debouncing.
 * - bID_bIndex(): Gets the index of a button based on its ID.
 * - bID_zIndex(): Gets the index of a zone based on its ID.
 * 
 * @note The file uses conditional compilation for debugging and tracing.
 * 
 * @version 2.5
 * @date 2023-10
 * 
 * @author 
 * Tomas
 * 
 */
#include <Control.h>

unsigned long lastMillis;
#define DEBOUNCEMILLIS 20
volatile uint16_t ledStatus = 0;

void apagaLeds()
{
  digitalWrite(HC595_LATCH, LOW);
  shiftOut(HC595_DATA, HC595_CLOCK, MSBFIRST, 0);
  shiftOut(HC595_DATA, HC595_CLOCK, MSBFIRST, 0);
  digitalWrite(HC595_LATCH, HIGH);
  ledStatus = 0;
  delay(200);
}

void enciendeLeds()
{
  digitalWrite(HC595_LATCH, LOW);
  shiftOut(HC595_DATA, HC595_CLOCK, MSBFIRST, 0xFF);
  shiftOut(HC595_DATA, HC595_CLOCK, MSBFIRST, 0xFF);
  digitalWrite(HC595_LATCH, HIGH);
  ledStatus = 0xFFFF;
  delay(200);
}

void loadDefaultSignal(uint veces)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in loadDefaultSignal"));
  #endif
  uint i;
  for(i=0;i<veces;i++) {
    led(LEDR,ON);
    led(LEDG,ON);
    delay(300);
    led(LEDR,OFF);
    led(LEDG,OFF);
    delay(300);
  }
}

void wifiClearSignal(uint veces)
{
  #ifdef TRACE
    Serial.println(F("TRACE: in wifiClearSignal"));
  #endif
  uint i;
  for(i=0;i<veces;i++) {
    led(LEDR,ON);
    led(LEDB,ON);
    delay(300);
    led(LEDR,OFF);
    led(LEDB,OFF);
    delay(300);
  }
}

void initLeds()
{
  int i;
  uint ledOrder[] = { lZONA1 , lZONA2 , lZONA3 , lZONA4 , lZONA5 , lZONA6 , lZONA7 ,
                      LEDR , LEDG , lGRUPO1 , lGRUPO2 , lGRUPO3 };
  size_t numLeds = sizeof(ledOrder)/sizeof(ledOrder[0]);
  apagaLeds();
  delay(200);
  for(i=0;i<numLeds;i++) {
    led(ledOrder[i],ON);
    delay(300);
    led(ledOrder[i],OFF);
  }
  delay(200);
  enciendeLeds();
  delay(200);
  apagaLeds();
  delay(200);
  led(LEDR,ON);
}

void initHC595()
{
  pinMode(HC595_CLOCK, OUTPUT);
  pinMode(HC595_DATA, OUTPUT);
  pinMode(HC595_LATCH, OUTPUT);
  apagaLeds();
}

void ledRGB(int  R, int G, int B)
{
  led(LEDR,R);
  led(LEDG,G);
  led(LEDB,B);
}

void led(uint8_t id,int estado)
{
    if(id==0) return;
    if(estado == ON) ledStatus |= (1 << (id-1));
    else ledStatus &= ~(1 << (id-1));
    uint8_t bajo = (uint8_t)((ledStatus & 0x00FF));
    uint8_t alto = (uint8_t)((ledStatus & 0xFF00) >> 8);
    digitalWrite(HC595_LATCH, LOW);
    shiftOut(HC595_DATA, HC595_CLOCK, MSBFIRST, alto);
    shiftOut(HC595_DATA, HC595_CLOCK, MSBFIRST, alto);
    shiftOut(HC595_DATA, HC595_CLOCK, MSBFIRST, bajo);
    digitalWrite(HC595_LATCH, HIGH);
}

bool ledStatusId(int ledID)
{
  #ifdef EXTRADEBUG
    Serial.print(F("ledStatus : "));Serial.println(ledStatus,BIN);
    Serial.print(F("ledID : "));Serial.println(ledID,DEC);
  #endif
  return((ledStatus & (1 << (ledID-1))));
}

void initCD4021B()
{
  pinMode(CD4021B_LATCH, OUTPUT);
  pinMode(CD4021B_CLOCK, OUTPUT);
  pinMode(CD4021B_DATA, INPUT);
}

byte shiftInCD4021B(int myDataPin, int myClockPin)
{
  int i;
  int temp=0;
  int myDataIn = 0;
  for (i=7;i>=0;i--)
  {
    digitalWrite(myClockPin,0);
    delayMicroseconds(2);
    temp = digitalRead(myDataPin);
    if(temp) myDataIn = myDataIn | (1 << i);
    digitalWrite(myClockPin,1);
  }
  return myDataIn;
}

uint16_t readInputs()
{
  byte    switchVar1;
  byte    switchVar2;
  digitalWrite(CD4021B_LATCH,1);
  delayMicroseconds(20);
  digitalWrite(CD4021B_LATCH,0);
  switchVar1 = shiftInCD4021B(CD4021B_DATA, CD4021B_CLOCK);
  switchVar2 = shiftInCD4021B(CD4021B_DATA, CD4021B_CLOCK);
  return switchVar2 | (switchVar1 << 8);
}

bool testButton(uint16_t id,bool state)
{
  uint16_t buttons = readInputs();
  bool result = ((buttons & id) == 0)?0:1;
  if (result == state) return 1;
   else return 0;
}

S_BOTON *parseInputs(bool read)
{
  int i;
  unsigned long currentMillis = millis();
  if(currentMillis < (lastMillis + DEBOUNCEMILLIS)) return NULL;
  else lastMillis = currentMillis;
  uint16_t inputs = readInputs();
  for (i=0;i<NUM_S_BOTON;i++) {
    if (!Boton[i].flags.enabled) continue;
    Boton[i].estado = inputs & Boton[i].id;
    if ((Boton[i].estado != Boton[i].ultimo_estado) || (Boton[i].estado && Boton[i].flags.hold && !Boton[i].flags.holddisabled))
    {
      Boton[i].ultimo_estado = Boton[i].estado;
      if (Boton[i].estado || Boton[i].flags.dual) {
        #ifdef DEBUG
          bool bEstado = Boton[i].estado;
          if (!read) Serial.print(F("Cleared: "));
          Serial.printf("Boton: %s  idx: %d  id: %#X  Estado: %d \n", Boton[i].desc, Boton[i].idx, Boton[i].id, bEstado);
        #endif
        if (read) return &Boton[i]; 
      }
    }
  }
  return NULL;
}

int bID_bIndex(uint16_t id)
{
  for (int i=0;i<NUM_S_BOTON;i++) {
    if (Boton[i].id == id) return i;
  }
  return 999;
}

int bID_zIndex(uint16_t id)
{
  for (uint i=0;i<NUMZONAS;i++) {
    if(ZONAS[i] == id) return i;
  }
  return 999;
}

