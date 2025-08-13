//----------------------------------------------------------------------------
// File:MZmain.cpp
// 
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

#include "FS.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <linux/input.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
//#include <sys/un.h>
//#include <sys/socket.h>
#include <ctype.h>
#include <errno.h>
//#include <sys/ipc.h>
//#include <sys/msg.h>
#include <libgen.h>
#include "m5z700.h"
#include "mz700lgfx.h"
#include <Keyboard.h>

#define ROM_START  0
#define L_TMPEX  0x0FFE

//M5AtomDisplay m5lcd;                 // LGFXのインスタンスを作成。
//lgfx::LGFX_SPI<EXLCD_LGFX_Config> exLcd;
//lgfx::Panel_ST7789 panel;

extern "C" {
#include "z80.h"
#include "Z80Codes.h"
}

#include "defkey.h"

#include "mzmain.h"
#include "MZhw.h"
#include "mzscrn.h"
//#include "mzbeep.h"
//#include <WebServer.h>
//#include <WiFiMulti.h>

static bool intFlag = false;

void scrn_draw();
void keyCheck();

void systemMenu();

void checkJoyPad();

//int buttonApin = 26; //赤ボタン
//int buttonBpin = 36; //青ボタン

#define CARDKB_ADDR 0x5F
#define FACES_KB_ADDR 0x08
//int checkI2cKeyboard();
int checkPicoCalcKey();
int checkSerialKey();

int keyCheckCount;
int preKeyCode;
bool inputStringMode;

void sortList(String fileList[], int fileListCount);
static void keyboard(const uint8_t* d, int len);

static bool saveMZTImage();

bool joyPadPushed_U;
bool joyPadPushed_D;
bool joyPadPushed_L;
bool joyPadPushed_R;
bool joyPadPushed_A;
bool joyPadPushed_B;
bool joyPadPushed_Press;
bool enableJoyPadButton;

SYS_STATUS sysst;
int xferFlag = 0;

bool wonderHouseMode;
int wonderHouseKeyIndex;
int wonderHouseKey();

KeyBoard *_keyboard;

#define SyncTime	17									/* 1/60 sec.(milsecond) */

#define MAX_FILES 128 // this affects memory

String selectMzt();
bool firstLoadFlag;

int q_kbd;
typedef struct KBDMSG_t {
  long mtype;
  char len;
  unsigned char msg[80];
} KBDMSG;

#define SHIFT_KEY_700 0x80
#define SFT7 0x80

//左シフト0x85,右シフト0x80
#define SHIFT_KEY_80C 0x80
#define SFT8 0x80

//PicoCalcから入力された文字のコードを、MZ-80Cのキー入力に変換する。
//SHIFT併用が必要な文字の場合、ak_tbl_80は SFT8 を指定して、押すキーを ak_tbl_s_80cに記載する。
//MZ-80K/C キーマトリクス参考：http://www43.tok2.com/home/cmpslv/Mz80k/EnrMzk.htm
unsigned char ak_tbl_80c[] =
{
  0xff, 0xff, 0xff, SFT8, 0xff, 0xff, 0xff, 0xff, 0xff, 0x65, 0xff, SFT8, 0xff, 0x84, 0xff, 0xff,
  0xff, 0x92, SFT8, 0x83, SFT8, 0x90, SFT8, 0x81, SFT8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

  // " "    !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /
  0x91, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, 0x73, 0x05, 0x64, 0x74,
  //  0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
  0x14, 0x00, 0x10, 0x01, 0x11, 0x02, 0x12, 0x03, 0x13, 0x04, SFT8, 0x54, SFT8, 0x25, SFT8, SFT8,
  //  @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
  SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8,
  //  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
  SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, SFT8, 0xff, 0xff,
  //  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
  0xff, 0x40, 0x62, 0x61, 0x41, 0x21, 0x51, 0x42, 0x52, 0x33, 0x43, 0x53, 0x44, 0x63, 0x72, 0x24,
  //  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~
  0x34, 0x20, 0x31, 0x50, 0x22, 0x23, 0x71, 0x30, 0x70, 0x32, 0x60, 0xff, 0xff, 0xff, 0xff, 0xff,
  //      F1    F2    F3    F4    F5    F6    F7    F8    F9
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff ,0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  //F10 
  0xff,

};
unsigned char ak_tbl_s_80c[] =
{
  0xff, 0xff, 0xff, 0x93, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x65, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0x92, 0xff, 0x83, 0xff, 0x90, 0xff, 0x81, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0x00, 0x10, 0x01, 0x11, 0x02, 0x12, 0x03, 0x13, 0x04, 0x25, 0x05, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x24, 0xff, 0x20, 0xff, 0x30, 0x33,
  0x23, 0x40, 0x62, 0x61, 0x41, 0x21, 0x51, 0x42, 0x52, 0x33, 0x43, 0x53, 0x44, 0x63, 0x72, 0x24,
  0x34, 0x20, 0x31, 0x50, 0x22, 0x23, 0x71, 0x30, 0x70, 0x32, 0x60, 0x31, 0x32, 0x22, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff ,0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff,
};

//MZ-700 キーマトリクス参考：http://www.maroon.dti.ne.jp/youkan/mz700/mzioframe.html
unsigned char ak_tbl_700[] =
{
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x04, 0x06, 0x07, 0xff, 0x00, 0xff, 0xff,
  0xff, 0x74, 0x75, 0x73, 0x72, SFT7, SFT7, 0x76, 0x77, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  // " "    !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /
  0x64, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, 0x14, 0x13, 0x67, 0x66, 0x61, 0x65, 0x60, 0x70,
  //  0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
  0x63, 0x57, 0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50, 0x62, 0x01, 0x02, SFT7, 0x05, SFT7, 0x71,
  //  @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
  0x15, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7,
  //  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
  SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, SFT7, 0xff,
  //  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
  0xff, 0x47, 0x46, 0x45, 0x44, 0x43, 0x42, 0x41, 0x40, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31,
  //  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~
  0x30, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x17, 0x16, SFT7, SFT7, SFT7, SFT7, 0xff,
  //      F1    F2    F3    F4    F5    F6    F7    F8    F9  
  0xff, 0x97, 0x96, 0x95, 0x94, 0x93 ,SFT7, SFT7, SFT7, SFT7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  //F10
  SFT7,
};

unsigned char ak_tbl_s_700[] =
{
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0x76, 0x77, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0x57, 0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x61, 0xff, 0x60, 0xff,
  0xff, 0x47, 0x46, 0x45, 0x44, 0x43, 0x42, 0x41, 0x40, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31,
  0x30, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x17, 0x16, 0x50, 0x67, 0x62, 0x14, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x70, 0x13, 0x71, 0x66, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x97, 0x96, 0x95, 0x94, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x93,
};

unsigned char *ak_tbl;
unsigned char *ak_tbl_s;

unsigned char shift_code;

#define MAX_PATH 256
char PROGRAM_PATH[MAX_PATH];
char FDROM_PATH[MAX_PATH];
char MONROM_PATH[MAX_PATH];
char CGROM_PATH[MAX_PATH];

extern uint16_t c_bright;

char serialKeyCode;

void selectTape();
void selectRom();
void sendCommand();
void sendBreakCommand();
//void setWiFi();
//void deleteWiFi();
void soundSetting();
void PCGSetting();
void doSendCommand(String inputString);
int set_mztype(void);

boolean sendBreakFlag;

String inputStringEx;

bool suspendScrnThreadFlag;

MZ_CONFIG mzConfig;

bool btKeyboardConnect = false;

//------------------------------------------------
// Memory Allocation for MZ
//------------------------------------------------
int mz_alloc_mem(void)
{
  int result = 0;

  /* Main Memory */
  //mem = (UINT8*)ps_malloc(64*1024);
  //mem = (UINT8*)ps_malloc((4 + 6 + 4 + 64) * 1024);
  mem = (UINT8*)malloc((4 + 6 + 4 + 64) * 1024);
  //mem = (UINT8*)pmalloc((4 + 6 + 4 + 64) * 1024);
  
  if (mem == NULL)
  {
    return -1;
  }

  /* Junk(Dummy) Memory */
  //junk = (UINT8*)ps_malloc(4096);
  junk = (UINT8*)malloc(4096);
  //junk = (UINT8*)pmalloc(4096);

  if (junk == NULL)
  {
    return -1;
  }

  /* MZT file Memory */
  //mzt_buf = (UINT32*)ps_malloc(4*64*1024);
  //mzt_buf = (UINT32*)ps_malloc(16 * 64 * 1024);
  //mzt_buf = (UINT32*)malloc(4 * 64 * 1024);
  //if (mzt_buf == NULL)
  //{
  //    return -1;
  //}

  /* ROM FONT */
  //mz_font = (uint8_t*)ps_malloc(ROMFONT_SIZE);
  #if defined (USE_EXT_LCD)||defined(_M5STICKCPLUS)||defined(_M5CARDPUTER)
  #else
  mz_font = (uint8_t*)malloc(ROMFONT_SIZE);
  if (mz_font == NULL)
  {
    result = -1;
  }

  /* PCG-700 FONT */
  //pcg700_font = (uint8_t*)ps_malloc(PCG700_SIZE);
  pcg700_font = (uint8_t*)malloc(PCG700_SIZE);
  if (pcg700_font == NULL)
  {
    result = -1;
  }
  #endif

  return result;
}

//------------------------------------------------
// Release Memory for MZ
//------------------------------------------------
void mz_free_mem(void)
{
  if (pcg700_font)
  {
    free(pcg700_font);
  }

  if (mz_font)
  {
    free(mz_font);
  }

  //if (mzt_buf)
  //{
//    free(mzt_buf);
//  }

  if (junk)
  {
    free(junk);
  }

  if (mem)
  {
    free(mem);
  }
}

//--------------------------------------------------------------
// ＲＯＭモニタを読み込む
//--------------------------------------------------------------
void monrom_load(void)
{
  Serial1.println("mzConfig.romFile");
  Serial1.println(mzConfig.romFile);
  String romFile = mzConfig.romFile;
  if (romFile.length() == 0) {
    romFile = DEFAULT_ROM_FILE;
  }
  String romPath = String(ROM_DIRECTORY) + "/" + romFile;
  Serial1.println("ROM PATH:" + romPath);

  if (SD.exists(romPath) == false) 
  {
    romFile = DEFAULT_ROM_FILE;
    romPath = String(ROM_DIRECTORY) + "/" + romFile;
    if (SD.exists(romPath) == false) 
    {
      m5lcd.println("ROM FILE NOT EXIST");
      m5lcd.printf("[%s]", romPath);
    }
  }
  File dataFile = SD.open(romPath, FILE_READ);
  int offset = 0;
  if (dataFile) {
    while (dataFile.available()) {
      *(mem + offset) = dataFile.read();
      offset++;
    }
    dataFile.close();
    m5lcd.print("ROM FILE:");
    m5lcd.println(romPath);
    Serial1.println(":END READ ROM:" + romPath);
  } else {
    m5lcd.print("ROM FILE FAIL:");
    m5lcd.println(romPath);
    Serial1.println("FAIL END READ ROM:" + romPath);
  }
  mzConfig.mzMode = set_mztype();
  Serial1.print("MZ TYPE=");
  Serial1.println(mzConfig.mzMode);

  if (mzConfig.mzMode == MZMODE_80) {
    ak_tbl = ak_tbl_80c;
    ak_tbl_s = ak_tbl_s_80c;
    
    shift_code = SHIFT_KEY_80C;
  } else {
    ak_tbl = ak_tbl_700;
    ak_tbl_s = ak_tbl_s_700;
    
    shift_code = SHIFT_KEY_700;
  }
}

int set_mztype(void) {
  if (!strncmp((const char*)(mem + 0x6F3), "1Z-009", 5))
  {
    return MZMODE_700;
  }
  else if (!strncmp((const char*)(mem + 0x6F3), "1Z-013", 5))
  {
    return MZMODE_700;
  }
  else if (!strncmp((const char*)(mem + 0x142), "MZ\x90\x37\x30\x30", 6))
  {
    return MZMODE_700;
  }
  return MZMODE_80;
}

//--------------------------------------------------------------
// ＭＺのモニタＲＯＭのセットアップ
//--------------------------------------------------------------
void mz_mon_setup(void)
{

  //memset(mem, 0xFF, 64*1024);
  memset(mem + VID_START, 0, 4 * 1024);
  memset(mem + RAM_START, 0, 64 * 1024);

}

//--------------------------------------------------------------
// シグナル関連
//--------------------------------------------------------------
void sighandler(int act)
{
  intFlag = true;
}

bool intByUser(void)
{
  return intFlag;
}

void mz_exit(int arg)
{
  sighandler(0);
}

//--------------------------------------------------------------
// メイン部
//--------------------------------------------------------------
int mz80c_main()
{
  delay(500);

  Serial1.println("M5Z-700 START");
  m5lcd.println("M5Z-700 START");
 
  _keyboard = new PicoCalcKeyBoard;

  btKeyboardConnect = false;

  suspendScrnThreadFlag = false;

  c_bright = TFT_GREEN;
  //c_bright = WHITE;

  serialKeyCode = 0;

  sendBreakFlag = false;
    
  //pinMode(buttonApin, INPUT_PULLUP);
  //pinMode(buttonBpin, INPUT_PULLUP);
  joyPadPushed_U = false;
  joyPadPushed_D = false;
  joyPadPushed_L = false;
  joyPadPushed_R = false;
  joyPadPushed_A = false;
  joyPadPushed_B = false;
  joyPadPushed_Press = false;
  enableJoyPadButton = false;

  keyCheckCount = 0;
  preKeyCode = -1;
  inputStringMode = false;

  firstLoadFlag = true;
  
  wonderHouseMode = false;

  Serial1.println("Screen init:Start");
  mz_screen_init();
  Serial1.println("Screen init:OK");
  mz_alloc_mem();
  Serial1.println("Alloc mem:OK");

  makePWM();
  Serial1.println("MakePWM:OK");
  // ＭＺのモニタのセットアップ
  mz_mon_setup();
  Serial1.println("Monitor:OK");
  // メインループ実行
  mainloop();

  delay(1000);

  mz_free_mem();
  mz_screen_finish();

  return 0;
}

////-------------------------------------------------------------
////  mz700win MAINLOOP
////-------------------------------------------------------------
void mainloop(void)
{
  long synctime = SyncTime;
  long timeTmp;
  long syncTmp;
  long startMills = millis();
  bool demoLoadFlag = false;
  //mzbeep_init(44100);
  monrom_load();
  font_load("");
  mz_reset();
  Serial1.println("mz_reset:OK");
  setup_cpuspeed(1);
   Serial1.println("setup_cpuspeed:OK");
  Z80_IRQ = 0;
  //	Z80_Trap = 0x0556;

  // start the CPU emulation

  updateStatusArea("");
  
  m5lcd.fillScreen(TFT_BLACK);
  
  delay(100);

  while (!intByUser())
  {
    timeTmp = millis();
    if (!Z80_Execute()) break;

    scrn_draw(); 

    keyCheck();

    syncTmp = millis();
    if (synctime - (syncTmp - timeTmp) > 0) {
      delay(synctime - (syncTmp - timeTmp));
    } else {
      delay(1);
    }

#if defined(AUTO_LOAD_ROPOKO) || defined( AUTO_LOAD_SP5030) || defined( AUTO_LOAD_SBASIC)
    if(demoLoadFlag == false && syncTmp - startMills > 1000 * 5){ //起動から5秒後にROPOKOトライアルを読み込み
      set_scren_update_valid_flag(false);
      suspendScrnThreadFlag = true;
      delay(100);
  #ifdef AUTO_LOAD_ROPOKO
      setMztToMemory("ROPOKO-T.mzt"); 
  #elif defined( AUTO_LOAD_SP5030)
      setMztToMemory("SP-5030.mzt"); 
  #else
      setMztToMemory("S-BASIC.mzt"); 
  #endif
      delay(100);
      m5lcd.fillScreen(TFT_BLACK);
      suspendScrnThreadFlag = false;
      set_scren_update_valid_flag(true); 
      demoLoadFlag = true;
    }
#endif

  }

}

//------------------------------------------------------------
// CPU速度を設定 (10-100)
//------------------------------------------------------------
void setup_cpuspeed(int mul) {
  int _iperiod;

  _iperiod = (CPU_SPEED * CpuSpeed * mul) / (100 * IFreq);

  _iperiod *= 256;
  _iperiod >>= 8;

  Z80_IPeriod = _iperiod;
  Z80_ICount = _iperiod;

}

//--------------------------------------------------------------
// スレッドの準備
//--------------------------------------------------------------
int create_thread(void)
{
  return 0;
}

//--------------------------------------------------------------
// スレッドの開始
//--------------------------------------------------------------
void start_thread(void)
{
}

//--------------------------------------------------------------
// スレッドの後始末
//--------------------------------------------------------------
int end_thread(void)
{
  return 0;
}

void suspendScrnThread(bool flag) {
  suspendScrnThreadFlag = flag;
  //delay(100);
}

//--------------------------------------------------------------
// 画面描画
//--------------------------------------------------------------
void scrn_draw()
{
  long synctime = SyncTime;
  long timeTmp;
  long vsyncTmp;
  if (suspendScrnThreadFlag == false) {
    // 画面更新処理
    hw700.retrace = 1;											/* retrace = 0 : in v-blnk */
    vblnk_start(); 
    timeTmp = millis();

    //前回から時間が経っていれば描画
    if (synctime - (timeTmp - vsyncTmp) < 0) {
      update_scrn();												/* 画面描画 */
      vsyncTmp = millis();
    }
  }
  return;
}

//--------------------------------------------------------------
// キーボード入力チェック
//--------------------------------------------------------------
int delayInputKeyCode = 0;  //SHIFT併用時にkeyDownを少し遅らせる
void keyCheck() {
  //入力がある場合、5回は入力のまま。
  if (preKeyCode != -1) {
    keyCheckCount++;
    if(delayInputKeyCode != 0){
       mz_keydown_sub(delayInputKeyCode);
       delayInputKeyCode  = 0;
    }
    if (keyCheckCount < 5) {
      return;
    }
    mz_keyup_sub(preKeyCode);
    mz_keyup_sub(shift_code);
    preKeyCode = -1;
    keyCheckCount = 0;
  }
  if (sendBreakFlag == true) { //SHIFT+BREAK送信
    mz_keydown_sub(shift_code);
    if (mzConfig.mzMode == MZMODE_700) {
      mz_keydown_sub(0x87);
      preKeyCode = 0x87;
    } else {
      mz_keydown_sub(ak_tbl_s[3]);
      preKeyCode = ak_tbl_s[3];
    }
    sendBreakFlag = false;
    return;
  }

  char inKeyCode = 0;

  //特別扱いのキー（ジョイパッドボタン用）
  bool ctrlFlag = false;
  bool shiftFlag = false;

  //inputStringExが入っている場合は、１文字ずつ入力処理
  if (inputStringEx.length() > 0) {
    inKeyCode = inputStringEx.charAt(0);
    if (inKeyCode == 'C') { //CTRL
      ctrlFlag = true;
    }
    if (inKeyCode == 'S') { //SHIFT
      shiftFlag = true;
    }
    inputStringEx = inputStringEx.substring(1);
  } else {
    if (inputStringMode == true) {
      inKeyCode = 0x0D;
      inputStringMode = false;
    }
  }

  if (ctrlFlag == false && shiftFlag == false) {
    //キー入力
    if (inKeyCode == 0) {
      inKeyCode = checkSerialKey();
    }

    if (inKeyCode == 0) {
      inKeyCode = checkPicoCalcKey();
      if(inKeyCode == 0xD6){ //SHIFT + ↑ :ファイル選択
        set_scren_update_valid_flag(false);
        suspendScrnThreadFlag = true;
        delay(500);
        selectMzt();
        delay(500);
        m5lcd.fillScreen(TFT_BLACK);
        suspendScrnThreadFlag = false;
        set_scren_update_valid_flag(true);
        return;
      }else if(inKeyCode == 0xD7) {//SHIFT + ↓ :システムメニュー
        set_scren_update_valid_flag(false);
        suspendScrnThreadFlag = true;
        delay(500);
        systemMenu();
        delay(500);
        m5lcd.fillScreen(TFT_BLACK);
        suspendScrnThreadFlag = false;
        set_scren_update_valid_flag(true);
        return;
      }
    }


    if (inKeyCode == 0) {
      return;
    }
    //Serial1.printf("inkeyCode:%#x:%#x:%#x\n", inKeyCode,ak_tbl[inKeyCode],ak_tbl_s[inKeyCode]);

    if (ak_tbl[inKeyCode] == 0xff)
    {
      //何もしない
    }
    else if (ak_tbl[inKeyCode] == shift_code)
    {
      mz_keydown_sub(shift_code);
      //mz_keydown_sub(ak_tbl_s[inKeyCode]);
      delayInputKeyCode = ak_tbl_s[inKeyCode];
      preKeyCode = ak_tbl_s[inKeyCode];
    }
    else
    {
      mz_keydown_sub(ak_tbl[inKeyCode]);
      preKeyCode = ak_tbl[inKeyCode];
    }
    
  } else {
    if (ctrlFlag == true) {
      mz_keydown_sub(0x86);
      preKeyCode = 0x86;
      ctrlFlag = false;
    }
    if (shiftFlag == true) {
      mz_keydown_sub(shift_code);
      preKeyCode = shift_code;
      shiftFlag = false;

    }
  }
}

//--------------------------------------------------------------
// シリアル入力
//--------------------------------------------------------------
int checkSerialKey()
{
  if (Serial1.available() > 0) {
    serialKeyCode = Serial1.read();
    if ( serialKeyCode == 27 ) { //ESC
      serialKeyCode = Serial1.read();
      if (serialKeyCode == 91) {
        serialKeyCode = Serial1.read();
        switch (serialKeyCode) {
          case 65:
            serialKeyCode = 0x12;  //UP
            break;
          case 66:
            serialKeyCode = 0x11;  //DOWN
            break;
          case 67:
            serialKeyCode = 0x13;  //RIGHT
            break;
          case 68:
            serialKeyCode = 0x14;  //LEFT
            break;
          case 49:
            serialKeyCode = 0x15;  //HOME
            break;
          case 52:
            serialKeyCode = 0x16;  //END -> CLR
            break;
          case 50:
            serialKeyCode = 0x18;  //INST
            break;
          default:
            serialKeyCode = 0;
        }
      } else if (serialKeyCode == 255)
      {
        //Serial1.println("ESC");
        //serialKeyCode = 0x03;  //ESC -> SHIFT + BREAK
        sendBreakFlag = true; //うまくキーコード処理できなかったのでWebからのBreak送信扱いにします。
        serialKeyCode = 0;
      }
    }
    if (serialKeyCode == 127) { //BackSpace
      serialKeyCode = 0x17;
    }
    while (Serial1.available() > 0 && Serial1.read() != -1);
  }
  return serialKeyCode;
}

//--------------------------------------------------------------
// JoyPadLogic
//--------------------------------------------------------------

void checkJoyPad() {
  return;
}
//--------------------------------------------------------------
// PicoCalc Keyboard Logic
//--------------------------------------------------------------

int checkPicoCalcKey() {
  
  uint8_t picoCalcKeyCode = 0;

  if( _keyboard->fetch_key(picoCalcKeyCode)){
  }else{  return 0;  }
  
  if (picoCalcKeyCode == 0) {
    return 0;
  }

  Serial1.println(picoCalcKeyCode, HEX);

  //特殊キー
  switch (picoCalcKeyCode) {
    case 0x0A:
      picoCalcKeyCode = 0x0D;  //CR
      break;
    case 0xB5:
      picoCalcKeyCode = 0x12;  //UP
      break;
    case 0xB6:
      picoCalcKeyCode = 0x11;  //DOWN
      break;
    case 0xB7:
      picoCalcKeyCode = 0x13;  //RIGHT
      break;
    case 0xB4:
      picoCalcKeyCode = 0x14;  //LEFT
      break;
    case 0xD2: //HOME
      picoCalcKeyCode = 0x15;  //HOME
      break;

    case 0xB1: //ESC->CLR
      picoCalcKeyCode = 0x16;
      break;
    case 0x09:  //TAB->INST
      picoCalcKeyCode = 0x18;
      break;
    case 0x08: //BS
      picoCalcKeyCode = 0x17;
      break;
    case 0xD0: //BREAK->SHIT+BREAK
      sendBreakFlag = true; //うまくキーコード処理できなかったのでWebからのBreak送信扱いにします。
      return 0;

      //入力モード切替
    case 0xC1: //Caps Lock->英数（PicoCalcは、CapsLock + SHIFT は取れない…）
      picoCalcKeyCode = 0x09;
      break;
    case 0xD4:  //DEL->かな
      picoCalcKeyCode = 0x0B;
      break;
    case 0xD5:  //END->記号
      picoCalcKeyCode = 0x0A;
      break;
    default:
      break;
  }
  return picoCalcKeyCode;
}

String selectMzt() {
  //音を鳴らしているなら止める
	tone(BUZZER_PIN,0);

  File fileRoot;
  String fileList[MAX_FILES];

  m5lcd.fillScreen(TFT_BLACK);
  m5lcd.setCursor(0, 0);
  m5lcd.setTextSize(2);
  String fileDir = TAPE_DIRECTORY; 

  fileRoot = SD.open(fileDir);
  int fileListCount = 0;

  while (1)
  {
    File entry = fileRoot.openNextFile();
    if (!entry)
    { // no more files
      break;
    }
    //ファイルのみ取得
    if (!entry.isDirectory())
    {
      String fullFileName = entry.name();
      String fileName = fullFileName.substring(fullFileName.lastIndexOf("/") + 1);
      String ext = fileName.substring(fileName.lastIndexOf(".") + 1);
      ext.toUpperCase();
      if(ext.equals("MZT")==false && ext.equals("MZF")==false && ext.equals("M12")==false){
        continue;
      }

      fileList[fileListCount] = fileName;
      fileListCount++;
      Serial1.println(fileName);
    }
    entry.close();
  }
  fileRoot.close();

  int startIndex = 0;
  int endIndex = startIndex + 10;
  if (endIndex > fileListCount)
  {
    endIndex = fileListCount;
  }

  sortList(fileList, fileListCount);

  boolean needRedraw = true;
  int selectIndex = 0;
  int preStartIndex = 0;
  int dispfileCount = 18;
  uint8_t picoCalcKeyCode = 0;
  while (true)
  {
    if (needRedraw == true)
    {
      m5lcd.setCursor(0, 0);
      startIndex = selectIndex - 15;
      if (startIndex < 0)
      {
        startIndex = 0;
      }
      endIndex = startIndex + dispfileCount;
      if (endIndex > fileListCount)
      {
        endIndex = fileListCount;
        startIndex = endIndex - dispfileCount;
        if (startIndex < 0) {
          startIndex = 0;
        }
      }
      if (preStartIndex != startIndex) {
        m5lcd.fillRect(0, 0, 320, 320, 0);
        preStartIndex = startIndex;
      }
      for (int index = startIndex; index < endIndex; index++)
      {
        if (index == selectIndex)
        {
            m5lcd.setTextColor(TFT_GREEN);
        }
        else
        {
          m5lcd.setTextColor(TFT_WHITE);
        }
        m5lcd.println(fileList[index].substring(0, 26));
      }
      m5lcd.setTextColor(TFT_WHITE);
      needRedraw = false;
    }

    if(_keyboard->fetch_key(picoCalcKeyCode)){

      if(picoCalcKeyCode != 0){
        Serial1.println(picoCalcKeyCode, HEX);
        switch(picoCalcKeyCode){
          case 0xB5://上
            selectIndex--;
            if (selectIndex < 0)
            {
              selectIndex = fileListCount -1;
            }
            needRedraw = true;
          break;
          case 0xB6: //下
            selectIndex++;
            if (selectIndex >= fileListCount)
            {
              selectIndex = 0;
            }
            needRedraw = true;
          break; 
          case 0xD1: //SHIFT+Enter->強制MZTメモリ読み込み
            firstLoadFlag = true;
            mz_reset();
          case 0x0A: //Enter
          case 0x20: //Space
            m5lcd.fillScreen(TFT_BLACK);
            if(firstLoadFlag == true){ //初回はMZTの目盛りコピー方式で読み込み、2回目以降はMZTを展開してエミュレータロジックで読み込み
              firstLoadFlag = false;
              m5lcd.setCursor(0, 0);
              m5lcd.print("SET MZT FILE:");
              m5lcd.print(fileList[selectIndex]);
              delay(2000);
              m5lcd.fillScreen(TFT_BLACK);
              delay(10);
              //メモリに読み込む
              setMztToMemory(fileList[selectIndex]);
              return fileList[selectIndex];
            }else{
              m5lcd.setCursor(0, 0);
              m5lcd.print("SET MZT TO CMT IMAGE AND START:");
              m5lcd.print(fileList[selectIndex]);
              //delay(2000);読み込み遅いのでdelay無くても大丈夫。
              set_mztData(fileList[selectIndex]);
              ts700.cmt_play = 1;
              ts700.mzt_start = ts700.cpu_tstates;
              ts700.cmt_tstates = 1;
              setup_cpuspeed(5);
              sysst.motor = 1; 
              xferFlag |= SYST_MOTOR;
              return fileList[selectIndex];
            }
          break;
          case 0x08: //BS
          case 0xB1: //ESC
            //何もせず戻る
            m5lcd.fillScreen(TFT_BLACK);
            delay(10);
            return "";
          break;
        }
      }
    }
    delay(10);
  }
}
int setMztToMemory(String mztFile) {

  String filePath = TAPE_DIRECTORY;
  filePath += "/" + mztFile;
  /*
    File fd = SD.open(filePath, FILE_READ);
    Serial1.println("fileOpen");
    if(fd == NULL)
    {
    Serial1.print("can't open:");
    Serial1.println(filePath);
    delay(100);
    fd.close();
    return false;
    }
    uint8_t header[128];
    fd.read((byte*)header, sizeof(header)) ;
    int size = header[0x12] | (header[0x13] << 8);
    int offs = header[0x14] | (header[0x15] << 8);
    int addr = header[0x16] | (header[0x17] << 8);
    Serial1.printf("Read MZT to RAM[size:%X][offs:%X][addr:%X]\n",size,offs,addr);
    fd.read(mem + offs + RAM_START, size) ;
    fd.close();

    //まだうまくうごきません…。
    mz_reset();
    mem[ROM_START+L_TMPEX] = (addr & 0xFF);
    mem[ROM_START+L_TMPEX+1] = (addr >> 8);
    Z80_Reset();
  */
  File fd = SD.open(filePath, FILE_READ);
  Serial1.println("fileOpen");
  if (fd == NULL)
  {
    Serial1.print("can't open:");
    Serial1.println(filePath);
    delay(100);
    fd.close();
    return false;
  }

  fd.seek(0, SeekEnd);
  fd.flush();
  int remain = fd.position();
  fd.seek(0, SeekSet);

  uint8_t header[128];
  fd.read((byte*)header, 8);

  if (header[0] == 'm' && header[1] == 'z' && header[2] == '2' && header[3] == '0') {
    // this is mzf format
    remain -= 8;

    while (remain >= 128 + 2 + 2) {
      fd.read(header, sizeof(header));
      fd.seek(2, SeekCur); // skip check sum
      remain -= 128 + 2;

      int size = header[0x12] | (header[0x13] << 8);
      int offs = header[0x14] | (header[0x15] << 8);
      int addr = header[0x16] | (header[0x17] << 8);
      
      Serial1.printf("MZ20[size:%X][offs:%X][addr:%X]\n", size, offs, addr);

      if (remain >= size + 2) {
        fd.read(mem + offs + RAM_START, size);
        fd.seek(2, SeekCur); // skip check sum
      }
      remain -= size + 2;
      Z80_Regs StartRegs;
      StartRegs.PC.D = addr;
      Z80_SetRegs(&StartRegs);
    }
  } else {
    // this is mzt format
    fd.seek(0, SeekSet);
    while (remain >= 128) {
      fd.read(header, sizeof(header));
      remain -= 128;
      byte mode =  header[0]; //1:マシン語 2:BASIC
      int size = header[0x12] | (header[0x13] << 8);
      int offs = header[0x14] | (header[0x15] << 8);
      int addr = header[0x16] | (header[0x17] << 8);

      //mode = 1 以外、または、場合、CMTをイメージをセットする。
      if(mode != 1){
        fd.close();
        m5lcd.print("\n[SET TO CMT IMAGE]");
        set_mztData(mztFile);
        ts700.cmt_play = 1;
        
        //カセットのセットだけして、回転開始はエミュレータ側に任せるほうがいいかどうか…
        ts700.mzt_start = ts700.cpu_tstates;
        ts700.cmt_tstates = 1;
        setup_cpuspeed(5);
        sysst.motor = 1;
        xferFlag |= SYST_MOTOR;

        return true;
      }
      m5lcd.print("\n[SET TO MEMORY]");
      delay(500);
      
      Serial1.printf("[size:%X][offs:%X][addr:%X]\n", size, offs, addr);

      if (remain >= size) {
//         if(mode == 2){ //BASICの場合、1バイトずつ処理。SP-5030用 / mode=5 S-BASIC？ワークエリア違ってるから読めないです…。
//           //http://www43.tok2.com/home/cmpslv/Mz80k/Mzsp5030.htm
//           WORD address = offs;//0x4806; //BASICプログラムエリア
//           WORD nextAddress = 0;
//           uint8_t readBuffer[128];
//           while(true){
//             //次のアドレス
//             fd.read(readBuffer, 2);
//             remain -= 2;
//             WORD deltaAddress = readBuffer[0] | (readBuffer[1] << 8);
//             if(deltaAddress <= 0){
//               break;
//             }
//             nextAddress = address + deltaAddress;
//             Serial1.printf("%x:%x:%x\n" ,address,deltaAddress,nextAddress);
//             *(mem + RAM_START + address)= ((uint8_t*)&(nextAddress))[0];
//             *(mem +  RAM_START + address + 1)= ((uint8_t*)&(nextAddress))[1];
//             fd.read(mem + RAM_START + address + 2, deltaAddress - 2); 
//             address = nextAddress;
//             remain -= deltaAddress;
//           }
//           *(mem + RAM_START + address) = 0;
//           *(mem +  RAM_START + address + 1)= 0;
//           address = address + 2;
//             //BASICワークエリアを更新
//             //０４６３４Ｈ－０４６３５Ｈ：ＢＡＳＩＣプログラム最終アドレス
//             //０４６３６Ｈ－０４６３７Ｈ：２重添字つきストリング変数エリア　ポインタ  (ＢＡＳＩＣプログラム最終アドレス+2バイト)
//             //０４６３８Ｈ－０４６３９Ｈ：　　添字つきストリング変数エリア　ポインタ  (+2バイト)
//             //０４６３ＡＨ－０４６３ＢＨ：　　　　　　ストリング変数エリア　ポインタ  (+2バイト)
//             //０４６３ＥＨ－０４６３ＦＨ：　　　　　　　　　　　配列エリア　ポインタ  (+2バイト)
//             //０４６４０Ｈ－０４６４１Ｈ：　　　　　　　　　　　変数エリア　ポインタ  (+2バイト)
//             //０４６４２Ｈ－０４６４３Ｈ：　　　　　　　　　　　ストリング　ポインタ  (+2バイト)
//             //０４６４４Ｈ－０４６４５Ｈ：　　　　　　　変数へ代入する数値　ポインタ  (+2バイト)

//             for(int workAddress = 0x4634;workAddress<0x4645;workAddress=workAddress + 2){
//               *(mem + RAM_START + workAddress)= ((uint8_t*)&(address))[0];
//               *(mem + RAM_START + workAddress + 1)= ((uint8_t*)&(address))[1];
//   //           Serial1.printf("WORK:%x:%x\n" ,workAddress,address);
//               address = address + 2;
//           }
//           //これは必要ないかも？
//           //*(mem + RAM_START + 0x4801)= 0x5a;
//           //*(mem + RAM_START + 0x4802)= 0x44;

// /* TEST
//           for(int testAddress = 0x4634;testAddress < 0x4660;testAddress=testAddress+16){
//             Serial1.printf("%08x  " ,testAddress);
//             for(int j=0;j<16;j++){
//               Serial1.printf("%02x " ,*(mem + RAM_START + testAddress + j));
//             }
//             Serial1.println();
//           }
// */

//         }else{
          fd.read(mem + offs + RAM_START, size);
//        }
      }
      remain -= size;
      if(mode == 1){
        Z80_Regs StartRegs;
        Z80_GetRegs(&StartRegs);
        StartRegs.PC.D = addr;
        Z80_SetRegs(&StartRegs);
        //複数データがあっても最初のだけ読んで終了
        return true;
      }
      // if(mode == 5){ //S-BASICワークエリア
      //   WORD address = offs + size;
      //   Serial1.printf("address :%x " ,address);
      //   //WORD workAreaAddress = 0x6B09;
      //   WORD workAreaAddress =0x6B49; //状況によって場所が異なるかも？
      //   *(mem + RAM_START + workAreaAddress)= ((uint8_t*)&(address))[0];
      //   *(mem + RAM_START + workAreaAddress + 1)= ((uint8_t*)&(address))[1];
      //   address = address + 1;
      //   *(mem + RAM_START + workAreaAddress + 2)= ((uint8_t*)&(address))[0];
      //   *(mem + RAM_START + workAreaAddress + 3)= ((uint8_t*)&(address))[1];
      //   address = address + 0x222;
      //   *(mem + RAM_START + workAreaAddress + 4)= ((uint8_t*)&(address))[0];
      //   *(mem + RAM_START + workAreaAddress + 5)= ((uint8_t*)&(address))[1];
      //   //workAreaAddress = 0x6B57;
      //   //*(mem + RAM_START + workAreaAddress + 14)= 0xB4;
      //   for(int testAddress = 0x6B09;testAddress < 0x6B60;testAddress=testAddress+16){
      //       Serial1.printf("%08x  " ,testAddress);
      //       for(int j=0;j<16;j++){
      //         Serial1.printf("%02x " ,*(mem + RAM_START + testAddress + j));
      //       }
      //       Serial1.println();
      //   }
      // }
    }
  }

  fd.close();

  return true;
}

#define MZT_LOAD_MODE 4
//#define SAVE_IMAGE_INDEX 5
#define SOUND_INDEX 5
#define PCG_INDEX 6

void systemMenu()
{
  static String menuItem[] =
  {
    "RESET:NEW MONITOR7",
    "RESET:NEW MONITOR",
    "RESET:MZ-1Z009",
    "RESET:SP-1002",
    "MZT LOAD MODE",
    //"SAVE MZT Image",
    "SOUND",
    "PCG",
    ""
  };
 //音が鳴っていたら止める
 	tone(BUZZER_PIN,0);

  delay(10);
  m5lcd.fillScreen(TFT_BLACK);
  delay(10);
  m5lcd.setTextSize(2);

  bool needRedraw = true;

  int menuItemCount = 0;
  while (menuItem[menuItemCount] != "") {
    menuItemCount++;
  }

  int selectIndex = 0;
  uint8_t picoCalcKeyCode = 0;
  delay(100);

  while (true)
  {
    if (needRedraw == true)
    {
      m5lcd.setCursor(0, 0);
      for (int index = 0; index < menuItemCount; index++)
      {
        if (index == selectIndex)
        {
          m5lcd.setTextColor(TFT_GREEN,TFT_BLACK);
        }
        else
        {
          m5lcd.setTextColor(TFT_WHITE,TFT_BLACK);
        }
        String curItem = menuItem[index];
        if(index == MZT_LOAD_MODE){
          curItem = curItem + (firstLoadFlag?String(":MEMORY COPY"):(":CMT EMULATE"));
        }
        if (index == PCG_INDEX) {
          curItem = curItem + ((hw700.pcg700_mode == 1) ? String(": ON ") : String(": OFF"));
        }

        if (index == SOUND_INDEX) {
          curItem = curItem + (mzConfig.enableSound ? String(": ON ") : String(": OFF"));
        }
        m5lcd.println(curItem);
      }
      m5lcd.setTextColor(TFT_WHITE);
      needRedraw = false;
    }
    if(_keyboard->fetch_key(picoCalcKeyCode)){
      if(picoCalcKeyCode != 0){
          switch(picoCalcKeyCode){
          case 0xB5://上
            selectIndex--;
            if (selectIndex < 0)
            {
              selectIndex = menuItemCount -1;
            }
            needRedraw = true;
            break;
          case 0xB6: //下
            selectIndex++;
            if (selectIndex >= menuItemCount)
            {
              selectIndex = 0;
            }
            needRedraw = true;
            break;
          case 0x08: //BS
          case 0xB1: //ESC
            //何もせず戻る
            m5lcd.fillScreen(TFT_BLACK);
            delay(10);
            return ;
          case 0x0A: //Enter
          case 0x20: //Space
            bool returnFlag = false;
            switch (selectIndex)
            {
            case 0:
              strcpy(mzConfig.romFile, "NEWMON7.ROM");
              break;
            case 1:
              strcpy(mzConfig.romFile, "NEWMON.ROM");
              break;
            case 2:
              strcpy(mzConfig.romFile, "1Z009.ROM");
              break;
            case 3:
              strcpy(mzConfig.romFile, "SP-1002.ROM");
              break;
            case MZT_LOAD_MODE:
              firstLoadFlag = !firstLoadFlag;
              needRedraw = true;
              break;
            case SOUND_INDEX:
              mzConfig.enableSound = !mzConfig.enableSound;
              needRedraw = true;
              break;
            // case SAVE_IMAGE_INDEX:
            //   saveMZTImage();
            //   m5lcd.fillScreen(TFT_BLACK);
            //   needRedraw = true;
            //   break;
            case PCG_INDEX:
              hw700.pcg700_mode = (hw700.pcg700_mode == 1) ? 0 : 1;
              needRedraw = true;
              break;
            default:
              m5lcd.fillScreen(TFT_BLACK);
              delay(10);
              return;
            }
          
            if (selectIndex >= 0 && selectIndex <= 3) {
              m5lcd.fillScreen(TFT_BLACK);
              m5lcd.setCursor(0, 0);
              m5lcd.print("Reset MZ:");
              m5lcd.println(mzConfig.romFile);
              delay(2000);
              monrom_load();
              mz_reset();
              firstLoadFlag = true;
              return;
            }

            if(returnFlag == true){
              m5lcd.fillScreen(TFT_BLACK);
              delay(10);
              return;
            }
        }
      }  
    }
    delay(10);
   }
}

//https://github.com/tobozo/M5Stack-SD-Updater/blob/master/examples/M5Stack-SD-Menu/M5Stack-SD-Menu.ino
void sortList(String fileList[], int fileListCount) {
  bool swapped;
  String temp;
  String name1, name2;
  do {
    swapped = false;
    for (int i = 0; i < fileListCount - 1; i++ ) {
      name1 = fileList[i];
      name1.toUpperCase();
      name2 = fileList[i + 1];
      name2.toUpperCase();
      if (name1.compareTo(name2) > 0) {
        temp = fileList[i];
        fileList[i] = fileList[i + 1];
        fileList[i + 1] = temp;
        swapped = true;
      }
    }
  } while (swapped);
}

void gui_msg(const char* msg)         // temporarily display a msg
{
    Serial1.println(msg);
    //if(btKeyboardConnect == false){
    //  updateStatusArea(msg);
    //}
}
void sys_msg(const char* msg) {
    Serial1.println(msg);
    //if(btKeyboardConnect == false){
    updateStatusArea(msg);
    //}
}

static bool saveMZTImage(){
  //上書きで保存
  //MZTファイル形式でメモリを保存する。
  //・ヘッダ (128 バイト)
  //0000h モード (01h バイナリ, 02h S-BASIC, C8h CMU-800 データファイル)
  //0001h ～ 0011h ファイル名 (0Dh で終わり / 0011h = 0Dh?)
  //0012h ～ 0013h ファイルサイズ
  //0014h ～ 0015h 読み込むアドレス
  ///0016h ～ 0017h 実行するアドレス
  //0018h ～ 007Fh 予約
  //・データ (可変長)
  //0080h ～ データがファイルサイズ分続く

  m5lcd.fillScreen(TFT_BLACK);
  m5lcd.setCursor(0,0);
  m5lcd.println("SAVE MZT IMAGE");
  File saveFile;
  String romDir = TAPE_DIRECTORY;
  String fileName = "";
  if (mzConfig.mzMode == MZMODE_80) {
      fileName =  "0_M5Z80MEM.MZT";
  }else{
      fileName =  "0_M5Z700MEM.MZT";
  }
  saveFile = SD.open(romDir + "/" + fileName , FILE_WRITE);
  if(saveFile == false){
    return false;
  }

  saveFile.write(0x01);
  saveFile.write('M');
  saveFile.write('5');
  saveFile.write('Z');
  saveFile.write('M');
  saveFile.write('E');
  saveFile.write('M');
  for(int i = 0;i < 11;i++){
    saveFile.write(0x0D);
  }
  //ファイルサイズ = (0xCFFF-0x1200) = BDFF
  saveFile.write(0xFF);
  saveFile.write(0xBD);

  //読み込みアドレス 0x1200
  saveFile.write((uint8_t)0x00);
  saveFile.write(0x12);
  
  //実行アドレス：現在のPC
  Z80_Regs regs;
  Z80_GetRegs(&regs);
  int addr = regs.PC.D;

  saveFile.write(((uint8_t*)&(addr))[1]);
  saveFile.write(((uint8_t*)&(addr))[0]);
  
  for(int i = 0x18;i <= 0x7f;i++){
    saveFile.write((uint8_t)0);
  }
  BYTE	*mainRamMem = mem + RAM_START;
  //RAM 1200h～CFFF を保存
  m5lcd.println();
  for(int index = 0x1200;index <= 0xCFFF;index++){
    saveFile.write(*(mainRamMem + index));
    if(index % 100 == 0){
      m5lcd.print("*");
    }
  }  
  saveFile.close();
  m5lcd.println();
  m5lcd.println("Save :" + fileName  +" COMPLTE!");
  delay(2000);
  return true;
}

