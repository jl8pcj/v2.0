#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <hardware/watchdog.h>

// =====================================================
// GPIO割り当て
// =====================================================

// D-Pad
#define PIN_UP     0
#define PIN_DOWN   1
#define PIN_RIGHT  2
#define PIN_LEFT   3

// Digital buttons
#define PIN_HOME   4
#define PIN_A      5
#define PIN_B      6
#define PIN_X      7
#define PIN_Y_BTN  8
#define PIN_L_BTN  9
#define PIN_ZL     10
#define PIN_R_BTN  11
#define PIN_ZR     12

// Piezo ADC
#define PIEZO_L    26
#define PIEZO_HR   27
#define PIEZO_Y    28
#define PIEZO_R    29

// =====================================================
// HORIPAD Button Bits
// =====================================================
#define BTN_Y       0x0001
#define BTN_B       0x0002
#define BTN_A       0x0004
#define BTN_X       0x0008
#define BTN_L       0x0010
#define BTN_R       0x0020
#define BTN_ZL      0x0040
#define BTN_ZR      0x0080
#define BTN_MINUS   0x0100
#define BTN_PLUS    0x0200
#define BTN_L3      0x0400
#define BTN_R3      0x0800
#define BTN_HOME    0x1000
#define BTN_CAPTURE 0x2000

// =====================================================
// HID
// =====================================================
Adafruit_USBD_HID usb_gamepad;

uint8_t const gamepad_report_desc[] = {
  0x05,0x01,0x09,0x05,0xA1,0x01,
  0x15,0x00,0x25,0x01,
  0x75,0x01,0x95,0x10,
  0x05,0x09,
  0x19,0x01,0x29,0x10,
  0x81,0x02,

  0x05,0x01,
  0x25,0x07,
  0x46,0x3B,0x01,
  0x75,0x04,
  0x95,0x01,
  0x65,0x14,
  0x09,0x39,
  0x81,0x42,

  0x65,0x00,
  0x95,0x01,
  0x81,0x01,

  0x26,0xFF,0x00,
  0x46,0xFF,0x00,
  0x09,0x30,
  0x09,0x31,
  0x09,0x32,
  0x09,0x35,
  0x75,0x08,
  0x95,0x04,
  0x81,0x02,

  0x06,0x00,0xFF,
  0x09,0x20,
  0x95,0x01,
  0x81,0x02,
  0xC0
};

// =====================================================
// Report構造
// =====================================================
typedef struct __attribute__((packed)) {
  uint16_t buttons;
  uint8_t hat;
  uint8_t lx, ly, rx, ry;
  uint8_t vendor;
} GamepadReport;

GamepadReport gp_report = {0,8,128,128,128,128,0};

// =====================================================
// Piezo
// =====================================================
int piezoBaseline[4];
int piezoThreshold[4];

// Piezoごとのデバウンス時間（ms）
uint32_t piezoDebounce[4] = {
  40,  // L → 軽打対応で短め
  50,  // HR
  40,  // Y → 軽打対応
  70   // R → 誤反応しやすいので長め
};

uint32_t lastHit[4] = {0,0,0,0};

bool calibrated=false;
uint32_t calibStart=0;

// =====================================================
// Helper
// =====================================================
void setButton(uint16_t mask,bool pressed){
  if(pressed) gp_report.buttons|=mask;
  else gp_report.buttons&=~mask;
}

// =====================================================
// Dpad → HAT
// =====================================================
void updateHat(){

  bool up=!digitalRead(PIN_UP);
  bool down=!digitalRead(PIN_DOWN);
  bool right=!digitalRead(PIN_RIGHT);
  bool left=!digitalRead(PIN_LEFT);

  if(up&&!left&&!right) gp_report.hat=0;
  else if(up&&right) gp_report.hat=1;
  else if(right&&!up) gp_report.hat=2;
  else if(down&&right) gp_report.hat=3;
  else if(down&&!right) gp_report.hat=4;
  else if(down&&left) gp_report.hat=5;
  else if(left&&!down) gp_report.hat=6;
  else if(up&&left) gp_report.hat=7;
  else gp_report.hat=8;
}

// =====================================================
// 自動キャリブ（USB後実行）
// =====================================================
void calibratePiezo(){

  int sum[4]={0};

  for(int i=0;i<200;i++){
    sum[0]+=analogRead(PIEZO_L);
    sum[1]+=analogRead(PIEZO_HR);
    sum[2]+=analogRead(PIEZO_Y);
    sum[3]+=analogRead(PIEZO_R);
    delay(2);
  }

  for(int i=0;i<4;i++){
    piezoBaseline[i]=sum[i]/200;
  }

  // --- Piezoごとの感度調整 ---
  piezoThreshold[0] = piezoBaseline[0] + 80;   // L → 軽打対応
  piezoThreshold[1] = piezoBaseline[1] + 90;   // HR
  piezoThreshold[2] = piezoBaseline[2] + 80;   // Y → 軽打対応
  piezoThreshold[3] = piezoBaseline[3] + 120;  // R → 誤反応しやすいので高め
}

// =====================================================
// setup
// =====================================================
void setup(){

  watchdog_enable(10000,1);

  int pins[]={
    PIN_UP,PIN_DOWN,PIN_RIGHT,PIN_LEFT,
    PIN_HOME,PIN_A,PIN_B,PIN_X,
    PIN_Y_BTN,PIN_L_BTN,PIN_ZL,
    PIN_R_BTN,PIN_ZR
  };

  for(int i=0;i<13;i++)
    pinMode(pins[i],INPUT_PULLUP);

  analogReadResolution(12);

  usb_gamepad.setReportDescriptor(
    gamepad_report_desc,
    sizeof(gamepad_report_desc)
  );

  usb_gamepad.setPollInterval(1);
  usb_gamepad.begin();

  TinyUSBDevice.setID(0x0F0D,0x00C1);
  TinyUSBDevice.setManufacturerDescriptor("HORI");
  TinyUSBDevice.setProductDescriptor("HORIPAD");
  TinyUSBDevice.attach();

  calibStart=millis();
}

// =====================================================
// loop
// =====================================================
void loop(){

  watchdog_update();

  if(!TinyUSBDevice.mounted()){
    delay(10);
    return;
  }

  // ---- 自動キャリブ（USB後2秒）----
  if(!calibrated && millis()-calibStart>2000){
    calibratePiezo();
    calibrated=true;
  }

  updateHat();

  // digital buttons
  setButton(BTN_HOME,!digitalRead(PIN_HOME));
  setButton(BTN_A,!digitalRead(PIN_A));
  setButton(BTN_B,!digitalRead(PIN_B));
  setButton(BTN_X,!digitalRead(PIN_X));
  setButton(BTN_Y,!digitalRead(PIN_Y_BTN));
  setButton(BTN_L,!digitalRead(PIN_L_BTN));
  setButton(BTN_R,!digitalRead(PIN_R_BTN));
  setButton(BTN_ZL,!digitalRead(PIN_ZL));
  setButton(BTN_ZR,!digitalRead(PIN_ZR));

  // ---- Piezo ADC ----
  int raw[4];
  raw[0] = analogRead(PIEZO_L);
  raw[1] = analogRead(PIEZO_HR);
  raw[2] = analogRead(PIEZO_Y);
  raw[3] = analogRead(PIEZO_R);

  // 最大値方式で誤反応防止
  int maxIndex = -1;
  int maxValue = 0;

  for (int i = 0; i < 4; i++) {
    if (raw[i] > piezoThreshold[i] && raw[i] > maxValue) {
      maxValue = raw[i];
      maxIndex = i;
    }
  }

  // Piezo ボタンは一旦すべて OFF
  setButton(BTN_L, false);
  setButton(BTN_Y, false);
  setButton(BTN_R, false);

  // ---- Piezoごとのデバウンス ----
  if (maxIndex >= 0) {
    uint32_t now = millis();

    if (now - lastHit[maxIndex] > piezoDebounce[maxIndex]) {
      lastHit[maxIndex] = now;

      if (maxIndex == 0) {
        setButton(BTN_L, true);
      }
      else if (maxIndex == 1) {
        gp_report.hat = 2;  // 右
      }
      else if (maxIndex == 2) {
        setButton(BTN_Y, true);
      }
      else if (maxIndex == 3) {
        setButton(BTN_R, true);
      }
    }
  }

  // send
  if(usb_gamepad.ready()){
    usb_gamepad.sendReport(0,&gp_report,sizeof(gp_report));
  }

  delay(5);
}
