/*----------------------------------------------
 *  Wrist-EMG Monitor M5StickC/Plus Amp DEMO Program
 *    #2021/01/09 - #2021/02/06 - #2021/04/12  - #2022/02/16   #2022/10/27
 *    
 *    Board Select -> M5Stick-C-Plus
 *        CPU Clock 40MHz
 *    デモ用なので、SLEEPTIMEの時間でDeepSleepに入ります。
 *       機能をOFFにするにはSLEEPTIME=0に指定してください。
 *
  -----------------------------------------------*/
#include <M5StickCPlus.h>
#include <WiFi.h>
//#include "esp_deep_sleep.h"   最新のM5ライブラリでは必要ない

#define HUMFREQ     50        // デフォルトの商用周波数 (50 or 60)
#define SLEEPTIME   120     // Deep Sleep Time(sec)  0の場合は無効 #2022/10/27

#define BOTTONPIN   GPIO_NUM_37
#define LEDPIN      GPIO_NUM_10
#define ADPIN       36           // GPIO36

#define LCDWIDTH        240     // M5Stick-Plus  240
#define LCDHEIGHT       135     // M5Stick-Plus  135
#define EMGBARHEIGHT     16     // 
#define WAVEHEIGHT      (LCDHEIGHT-EMGBARHEIGHT-36)
#define IRANGEMIN       14
#define IRANGEMAX       (IRANGEMIN+WAVEHEIGHT)
#define WAVECLRWIDTH    20     // 波形の消去幅

#define IMAXBANKN   50
#define IMAXWAVEN   100
#define SAMPLERATE  1000      // 1KHz固定

hw_timer_t * timer = NULL;

void IRAM_ATTR TimerIsr();

void  TimerIsr();
void  SubGainMag(); 
byte  SubKeyCheck(int16_t* duration); 
int   SubWaveFilter (int addata);
void  SubCaliculateFilter(float fs, float fc, float fn);

volatile  int     nBankWave[IMAXBANKN][IMAXWAVEN];
volatile  int     nBankPtr;
volatile  int     nWriteBank;
volatile  int     nReadBank;
volatile  int     nBankCount;
volatile  int     nBankError;

static    int     nNotchFreq;       // 0:OFF   50:   60
static    int     nLFF;          
static    int     nMag;
static    int     nBufSpeedPnt;  

static    int     nSweepPtr;    // X Position
static    int     nCALEU;
static    int     nCALVU;
static    int     nBaseY;  
static    int     nLastY;  
static    int     nBaseLevel;    // 基線レベル(固定2048)
static    float   fConv;
static    float   fSum;
static    int     nAbsCount;  

static  uint32_t sectime;
static  uint32_t lasttime;

void setup() {
int   i;
uint8_t macBT[6];
char    btName[32];

    M5.begin();
    WiFi.mode(WIFI_OFF);                  // 省電力でWifiOFFする場合に下記入れる
    pinMode(ADPIN, ANALOG);
    gpio_pulldown_dis(GPIO_NUM_25);
    gpio_pullup_dis(GPIO_NUM_25);
    M5.Axp.ScreenBreath(10);         // LCD輝度を抑える 7-15
    M5.Lcd.setRotation(1);          // 回転をDefaultに戻す
    M5.Lcd.fillScreen(BLACK);
  //--
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setTextSize(1);    
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("Wrist-EMG Monitor (V1.20-40MHz)");
  //--
    nNotchFreq  = HUMFREQ;       // 0:OFF   50:   60
    nLFF        = 15;            // 15Hz
    nMag        = 1;
    nSweepPtr   = 0;
    nBufSpeedPnt= 8;
    nBaseLevel  = 2048;               // 1Vオフセット値
    nLastY      = 0;  
  //--
    analogSetAttenuation(ADC_6db);    // ADレンジを2Vにする #2020/11/17
    nBankError  = 0;
    nBankPtr    = 0;
    nWriteBank  = 0;
    nReadBank   = 0;
    nBankCount  = 0;
    fSum        = (float)0;
    nAbsCount   = 0;  
  //--
//    SubKeyInitial(1, 2, 100);
//    SubKeyInitial(2, 2, 100);
 //--
    SubGainMag();  
    SubCaliculateFilter((float)SAMPLERATE, (float)nLFF, (float)nNotchFreq);
    delay(200);      
//-- 割込み処理設定
    timer = timerBegin(0, 80, true);    // 1MHz
    timerAttachInterrupt(timer, &TimerIsr, true);
    timerAlarmWrite(timer, 1000, true);       // 1ms (1KHz)
    timerAlarmEnable(timer);    
  //--
    sectime = millis()/1000;
    lasttime = sectime;  
}


//-------------------
//  Timer Interrupt 割込み処理
//     1msec
void TimerIsr()
{
  nBankWave[nWriteBank][nBankPtr] = analogRead(ADPIN)-nBaseLevel;
  nBankPtr++;
  if (nBankPtr>=nBufSpeedPnt){
    nBankPtr = 0;
    nWriteBank++;
    if (nWriteBank>=IMAXBANKN) nWriteBank=0;
    nBankCount++;
    if (nBankCount>=IMAXBANKN){
      nBankCount  = 0;
      nWriteBank  = 0;
      nReadBank   = 0;
      nBankError++;
    }
  }
}


//--------------
//
void loop() {
unsigned long nowtime;  
int16_t  dur;
byte    key;
int     val,nYPos,nYPos0,nXPos,nMin,nMax;
int     sum;
int     nWave[IMAXWAVEN];
int     i;

  if (nBankCount>0){
    for (i=0; i<nBufSpeedPnt; i++){
      nWave[i] = SubWaveFilter(nBankWave[nReadBank][i]);
    }
    nReadBank++;
    if (nReadBank>=IMAXBANKN) nReadBank=0;
    nBankCount--;
  //-- Apple Watch風
    nXPos = nSweepPtr+2; 
    M5.Beep.tone(1000+nXPos*10, 10);
    if (nXPos>=LCDWIDTH) nXPos-=LCDWIDTH;
    if (nXPos>=(LCDWIDTH-WAVECLRWIDTH)){
      M5.Lcd.fillRect(nXPos, IRANGEMIN, LCDWIDTH-nXPos, (IRANGEMAX-IRANGEMIN), BLACK);      
      M5.Lcd.fillRect(0,         IRANGEMIN, WAVECLRWIDTH-(LCDWIDTH-nXPos), (IRANGEMAX-IRANGEMIN), BLACK);      
    }else{
      M5.Lcd.fillRect(nXPos, IRANGEMIN, WAVECLRWIDTH, (IRANGEMAX-IRANGEMIN), BLACK);      
    }
    for (i=0; i<nBufSpeedPnt; i++){
      val = nWave[i];
      nYPos = nBaseY - (int)((float)val*fConv);
      if (nYPos<(IRANGEMIN+2)) nYPos=IRANGEMIN+2;
      if (nYPos>(IRANGEMAX-2)) nYPos=IRANGEMAX-2;
      if (i==0){
        nYPos0 = nYPos;
        nMin   = nYPos;
        nMax   = nYPos;
      }else{
        if (nMin>nYPos) nMin=nYPos;
        if (nMax<nYPos) nMax=nYPos;
      }
    }
    M5.Lcd.drawLine(nSweepPtr, nLastY, nSweepPtr, nYPos0, RED);
    M5.Lcd.drawLine(nSweepPtr, nMin,   nSweepPtr, nMax,   RED);
    if (nXPos<(LCDWIDTH-5)) M5.Lcd.fillRect(nXPos+1, nYPos-2, 4, 4, WHITE);      
    nLastY = nYPos;
    nSweepPtr++;
    if (nSweepPtr >= LCDWIDTH) nSweepPtr=0;   
  //-- Graph Bar
    for (i=0; i<nBufSpeedPnt; i++){
      fSum = fSum + (float)abs(nWave[i]);
      nAbsCount++;
      if (nAbsCount>=(SAMPLERATE/10)){
        fSum = fSum/(float)(SAMPLERATE/10);
        M5.Lcd.fillRect(0, IRANGEMAX+1, LCDWIDTH, LCDHEIGHT-IRANGEMAX-1, BLACK);      
        nXPos = (int)((float)LCDWIDTH*fSum/(float)2048);
        M5.Lcd.fillRect(0, IRANGEMAX+5, nXPos, EMGBARHEIGHT, GREEN);      
        fSum = (float)0;
        nAbsCount = 0;
      }
    }
  }
  // Serial.println(nXPos);    //追加

//--
  nowtime =  millis()/1000;
  if (sectime!=nowtime){
    sectime = nowtime;
  //-- DEEP SLEEP チェック
    if (SLEEPTIME>0){       // #2022/10/27
      if ((sectime-lasttime)>=SLEEPTIME){
        M5.Lcd.fillScreen(BLACK);
        M5.Axp.ScreenBreath(0);   // LCD OFF
        pinMode(BOTTONPIN, INPUT_PULLUP);
        pinMode(LEDPIN,    INPUT);
        esp_sleep_enable_ext0_wakeup(BOTTONPIN, LOW);           // GPIO37(M5StickCのHOMEボタン)がLOWになったら起動
        esp_deep_sleep_start();    // ディープスリープ
      }
    }
  }  
//
  M5.update();
  key  = SubKeyCheck(&dur);
  switch(key){
    case 1:       // A Button
      lasttime = nowtime;  
      if (dur<=300){    // GAIN-UP
        nMag++;
        if (nMag>=10) nMag=1;
      }else{
        if (dur<=800){    // GAIN-DN
          nMag--;
          if (nMag==0) nMag=10;
        }else{          // Save  
          
        }
      }
      SubGainMag();
      break;
    case 2:       // B Button
      lasttime = nowtime;  
      if (dur<=300){
        nBufSpeedPnt-=2;
        if (nBufSpeedPnt<2) nBufSpeedPnt=20;
      }else{
        if (dur<=800){
          nBufSpeedPnt+=2;
          if (nBufSpeedPnt>20) nBufSpeedPnt=2;
        }else{    

        }
      }
      SubGainMag();
      break;
  }
}


void SubGainMag(){
char    strbuff[64];
char    strbuff1[32];

    nCALEU  = WAVEHEIGHT;
    nCALVU  = 4096;
    fConv   = (float)nMag*(float)nCALEU/(float)nCALVU;
    nBaseY  = 18+WAVEHEIGHT/2;  
//--
    M5.Lcd.fillRect(0, LCDHEIGHT-16, LCDWIDTH, 16, BLACK);      
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setTextSize(2);    
//--
    M5.Lcd.setCursor(2, LCDHEIGHT-16);
    sprintf(strbuff,"Gain:%i Sp:%i ",nMag,nBufSpeedPnt);
}


//------------------------------------------------
//  Digital Filter Caliculate Subroutine
//    #2020/06/16 - #2021/01/10
//    
#define   VAL1FLOAT (float)1.0         // 1.0
#define   VAL2FLOAT (float)2.0         // 2.0
#define   PAI       (float)3.141592654     // PAI.

//-- LFF Table
static    float   fLFF_a;      // a.
static    float   fLFF_b0;     // b0.
static    float   fLFF_b1;     // b1.
static    float   fLFF_z;      // z  Delay element for PC Filter.
//-- Notch Table
static    float   Notch_a0;  
static    float   Notch_a1;  
static    float   Notch_b0;  
static    float   Notch_b1;  
static    float   Notch_b2;  
static    float   Notch_z0;   
static    float   Notch_z1;      

//-----------------------------------------
//   複合フィルタを計算する(int)
int  SubWaveFilter (int addata)
{
float    fData;
float    sum;
int      nRet;

  fData = (float)addata;
//-- LFF 
  sum    = fData + fLFF_z*fLFF_a;
  fData  = (sum - fLFF_z)*fLFF_b0;
  fLFF_z = sum;               // Delay Element Shift2.
//-- Notch
  sum   = Notch_a0*Notch_z0+Notch_a1*Notch_z1+fData;
  fData = Notch_b0*sum+Notch_b1*Notch_z0+Notch_b2*Notch_z1;
  Notch_z1=Notch_z0;    // Delay Element Shift1.
  Notch_z0=sum;         // Delay Element Shift2.
//---
  nRet = (int)fData;
  if (nRet>=32767) nRet = 32767;
  if (nRet<-32768) nRet = -32768;
  return nRet;
}

//-----------------------------------------
//   ﾌｨﾙﾀｰ係数を再計算する
void  SubCaliculateFilter(float fs, float fc, float fn)
{
float     a,b,c,d,Ta,Ts;    // 2 Coefficient.
float     fa,fQ,omega,alpha,beta; 

  if (fc>=(fs/VAL2FLOAT)) fc=0;      // Freq over.
  fLFF_a=0;
  fLFF_b0=VAL1FLOAT;
  fLFF_b1=0;
  if (fc!=0){
    Ta=VAL2FLOAT*PI*fc;            // 1/(2*n*fc)
    Ts=VAL1FLOAT/fs;                // 
    a=(VAL2FLOAT-Ta*Ts)/(VAL2FLOAT+Ta*Ts);      // Coefficient(a).
    b=VAL2FLOAT/(VAL2FLOAT+Ta*Ts);              // Coefficient(b).
    fLFF_a=a;
    fLFF_b0=b;
    fLFF_b1=(float)(-1.0);
  }
  fLFF_z =(float)0;
//--
  fQ = (float)2;
  if (fn!=0){
    Ts=VAL1FLOAT/fs;
    fa=fs*tan(fn*PAI/fs)/PAI;        // Analogue <-- Digital Vutoff frequency.
    omega=VAL2FLOAT*PAI*fa;          // 2*n*fa
    alpha=(float)4.0+omega*omega*Ts*Ts;
    beta =(float)4.0-omega*omega*Ts*Ts;
    a=alpha+VAL2FLOAT*(omega/fQ)*Ts;
    b=-VAL2FLOAT*beta;
    c=alpha-VAL2FLOAT*(omega/fQ)*Ts;
    d=alpha;
  //---
    Notch_a0=-b/a;
    Notch_a1=-c/a;
    Notch_b0= d/a;
    Notch_b1= b/a;
    Notch_b2= d/a;
  }else{
    Notch_a0=0;
    Notch_a1=0;
    Notch_b0=VAL1FLOAT;
    Notch_b1=0;
    Notch_b2=0;
  }
  Notch_z0 = (float)0;   
  Notch_z1 = (float)0;      
}


//---------------
static  byte  keyMode1=2;       // A-KEY1 MODE (0:OFF  1:Edge  2:Dur)
static  byte  keyMode2=2;       // B-KEY2 MODE (0:OFF  1:Edge  2:Dur)
static  int   defaultZeroDur1=100; // Duration=0時のDurデフォルト(0:OFF  1-N:ms)
static  int   defaultZeroDur2=100; // Duration=0時のDurデフォルト(0:OFF  1-N:ms)

static  byte  keyState=0;
static unsigned long ontime1;
static unsigned long ontime2;
static unsigned long offtime=0;
//--------------

//------------------
//  Key Response Check 
//    #2021/01/11-
//
byte  SubKeyCheck(int16_t* duration){
byte  sw;
byte  i;
int   dur;
unsigned long nowtime;

  sw  = 0;
  dur = 0;
  switch(keyState){
    case 0:         // Find Key
      nowtime = millis();
      if (nowtime>=offtime){
        if (keyMode1>0){
          if (M5.BtnA.isPressed()){
            ontime1 = nowtime;
            switch(keyMode1){
              case 1:
                delay(100);
                sw  = 0x01;
                dur=defaultZeroDur1;       // Edge 
                break;
              case 2:
                keyState  = 0x01;
                break;
            }
          }
        }
        if (keyMode2>0){
          if (M5.BtnB.isPressed()){
            ontime2 = nowtime;
            switch(keyMode2){
              case 1:
                delay(100);
                sw  = 0x02;
                dur=defaultZeroDur2;       // Edge 
                break;
              case 2:
                keyState = 0x02;
                break;
            }
          }
        }
      }
      break;
    case 1:
      if (!M5.BtnA.isPressed()){
          sw = keyState;
          keyState = 0;
          dur = (int)(millis()-ontime1);
      }
      break;
    case 2:
      if (!M5.BtnB.isPressed()){
          sw = keyState;
          keyState = 0;
          dur = (int)(millis()-ontime2);
      }
      break;
  }
  if (sw>0) offtime = millis()+dur;        // OFF時間
  *duration = dur;
  return sw;
}

//----------  END  ------
