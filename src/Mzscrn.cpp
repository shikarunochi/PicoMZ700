//----------------------------------------------------------------------------
// File:MZscrn.cpp
// MZ-700 Emulator PicoMZ700 for PicoCalc(with RaspberryPi Pico2)
// m5z700:Main Program Module based on mz80rpi
// modified by @shikarunochi 2025
//
// mz80rpi by Nibble Lab./Oh!Ishi, based on MZ700WIN
// (c) Nibbles Lab./Oh!Ishi 2017
//----------------------------------------------------------------------------

#include "FS.h"
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
//#include <linux/fb.h>
//#include <linux/fs.h>
//#include <sys/mman.h>
//#include <sys/ioctl.h>
#include <stdbool.h>
#include "m5z700.h"

#include "mz700lgfx.h"

//
#include "z80.h"
//
#include "mzmain.h"
#include "MZhw.h"
#include "mzscrn.h"
#include <SD.h>

uint16_t c_bright;
#define c_dark TFT_BLACK

//フレームバッファ
//static LGFX_Sprite fb(&m5lcd); // スプライトを使う場合はLGFX_Spriteのインスタンスを作成。
//TFT_eSprite fb = TFT_eSprite(&M5.Lcd);

LGFX_Sprite canvas(&m5lcd);

int fontOffset = 0;
int fgColor = 0;
int bgColor = 0;
int fgColorIndex = 0;
int bgColorIndex = 0;
int MZ700_COLOR[] = {TFT_BLACK, TFT_BLUE, TFT_RED, TFT_MAGENTA, TFT_GREEN, TFT_CYAN, TFT_YELLOW, TFT_WHITE};

int lcdMode = 0;
//int lcdRotate = 0; move to m5z700AtomHDMI.ino

boolean needScreenUpdateFlag=false;
boolean screnUpdateValidFlag=false;

String statusAreaMessage;

void update_scrn_thread(void *pvParameters);

/*
   表示画面の初期化
*/
int mz_screen_init(void)
{
  needScreenUpdateFlag = false;
  screnUpdateValidFlag = false;

  canvas.setColorDepth(8);
  canvas.setTextSize(1);
  canvas.createSprite(320,40); //メモリ足りないので縦40ドット（=5行）に分割して5回に分けて描画する。

  statusAreaMessage = "";

  screnUpdateValidFlag = true;
  return 0;
}

/*
   画面関係の終了処理
*/
void mz_screen_finish(void)
{
}

void set_scren_update_valid_flag(boolean flag){
  screnUpdateValidFlag = flag;
}
/*
   フォントデータを描画用に展開
*/
int font_load(const char *fontfile)
{
  #if defined (USE_EXT_LCD)||defined(_M5STICKCPLUS)||defined(_M5ATOMS3)||defined(_M5CARDPUTER)
    Serial1.println("USE INTERNAL FONT DATA");
    return 0;
  #endif
  FILE *fdfont;
  int dcode, line, bit;
  UINT8 lineData;
  uint16_t color;

  String romDir = ROM_DIRECTORY;
  String fontFile = DEFAULT_FONT_FILE;
  File dataFile = SD.open(romDir + "/" + fontFile, FILE_READ);
  if (!dataFile) {
    Serial1.println("FONT FILE NOT FOUND");
    perror("Open font file");
    return -1;
  }

  for (dcode = 0; dcode < 256 * 2; dcode++) //拡張フォントも読み込む
  {
    for (line = 0; line < 8; line++)
    {
      if (dataFile.available()) {
        lineData = dataFile.read();
        mz_font[dcode * 8 + line] = lineData;
        if (dcode >= 128 && dcode < 256 ) { //通常フォント後半の場合PCG初期値として設定する
          pcg700_font[(dcode & 0x7F) * 8 + line] = lineData;
          pcg700_font[(dcode) * 8 + line] = lineData;
        }
      }
    }
    delay(10);
  }
  Serial1.println("END READ ROM");

  dataFile.close();
  return 0;
}
/**********************/
/* redraw the screen */
/*********************/
void update_scrn(void) {
    needScreenUpdateFlag = true;
}

void update_scrn_thread(){
    if (needScreenUpdateFlag == true && screnUpdateValidFlag == true) {
      needScreenUpdateFlag = false;
      UINT8 ch;
      UINT8 chAttr;
      UINT8 lineData;
      uint8_t *fontPtr;
      int ptr = 0;//YOFFSET;
      uint16_t blankdat[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      int color;
      hw700.retrace = 1;

      hw700.cursor_cou++;
      if (hw700.cursor_cou >= 40)													/* for japan.1/60 */
      {
        hw700.cursor_cou = 0;
      }
      fontOffset = 0;
      fgColor = 0;
      bgColor = 0;
      fgColorIndex = 0;
      bgColorIndex = 0;
      int drawIndex = 0;
      m5lcd.startWrite();
      for (int cy = 0; cy < 25; cy++) {
        for (int cx = 0; cx < 40; cx++) {
          ch = mem[VID_START + cx + cy * 40];
          if (mzConfig.mzMode == MZMODE_700) {
            chAttr = mem[VID_START + cx + cy * 40 + 0x800];
            if ((chAttr & 0x80) == 0x80) { //MZ-700キャラクタセット
              fontOffset = 0x100;
            } else {
              fontOffset = 0;
            }
            //Color
            fgColorIndex = ((chAttr >> 4) & 0x07) ;
            bgColorIndex = (chAttr & 0x07);

            if (fgColorIndex <= 7) {
              fgColor = MZ700_COLOR[fgColorIndex];
            } else {
              fgColor = MZ700_COLOR[7];
              Serial1.print("fgColorIndexError:");
              Serial1.print(fgColorIndex);
            }
            if (bgColorIndex <= 7) {
              bgColor = MZ700_COLOR[bgColorIndex];
            } else {
              fgColor = MZ700_COLOR[0];
              Serial1.print("bgColorIndexError:");
              Serial1.print(bgColorIndex);
            }
          } else {
            fgColor = c_bright;
            bgColor = c_dark;
          }
            canvas.fillRect(cx * 8, (cy * 8) % 40, 8, 8, bgColor);
          if (hw700.pcg700_mode == 0 || !(ch & 0x80)) {
              fontPtr = &mz_font[(ch + fontOffset) * 8];
          } else {
            if ((chAttr & 0x80) != 0x80) {
              fontPtr = &pcg700_font[(ch & 0x7F) * 8];
            } else {
              fontPtr = &pcg700_font[(ch) * 8];
            }
          }
          canvas.drawBitmap(cx * 8, (cy * 8) %40 , fontPtr, 8, 8, fgColor);
        }
          if((cy+1)%5 == 0){
          canvas.pushSprite(0, 40 * drawIndex); 
          drawIndex = drawIndex + 1;
          }
      }
      if(statusAreaMessage.equals("")==false){
          m5lcd.setTextColor(TFT_WHITE);
          m5lcd.setTextSize(1);
          m5lcd.fillRect(0, 200, 320, 10, TFT_BLACK);
          m5lcd.setCursor(0, 200);
          m5lcd.print(statusAreaMessage);
      }else{
          m5lcd.fillRect(0, 200, 320, 10, TFT_BLACK);
      }
      m5lcd.endWrite();
    }
    delay(10);
}

void updateStatusArea(const char* message) {
  statusAreaMessage = String(message);
}
