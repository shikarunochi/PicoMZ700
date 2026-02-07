//----------------------------------------------------------------------------
// File:main.cpp
// MZ-700 Emulator PicoMZ700 for PicoCalc(with RaspberryPi Pico2)
// m5z700:Main Program Module based on mz80rpi
// modified by @shikarunochi 2025
//
// mz80rpi by Nibble Lab./Oh!Ishi, based on MZ700WIN
// (c) Nibbles Lab./Oh!Ishi 2017
//
// 'mz700win' by Takeshi Maruyama, based on Russell Marks's 'mz700em'.
// Z80 emulation from 'Z80em' Copyright (C) Marcel de Kogel 1996,1997
//----------------------------------------------------------------------------
#include <SD.h>

#include "m5z700.h"
#include "mzmain.h"
#include "mz700lgfx.h"

LGFX m5lcd;

int xorKey = 0x80;

int lcdRotate = 0;

void setup() {
  //シリアルモニタ出力はGPIO 4/5 で Serial2 を使っています
	Serial2.setTX(4);
	Serial2.setRX(5);
	Serial2.begin(115200);
	delay(100);
  Serial2.println("START!");
  
  m5lcd.init();

  m5lcd.println("LCD SETUP OK!");

  SPI.setTX(19);
  SPI.setSCK(18);
  SPI.setRX(16);
  SPI.setCS(17);
  while (false == SD.begin(17, SPI))
   {
      Serial2.println("SD Wait ...");
      delay(500);
   }
  m5lcd.println("SD CARD OK!");
  m5lcd.println("MAIN START...");
  Serial2.println("MAIN_START");
  delay(1000);
  m5lcd.clearDisplay(TFT_BLACK);
  mz80c_main();   
  exit(0);  
}

void loop() {

}

void setup1(){

}
void loop1(){
  //画面更新は Core1 で処理
  update_scrn_thread();
}