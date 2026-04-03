#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <hardware/watchdog.h>

// =====================================================
// GPIO割り当て
// =====================================================

// D-Pad (digital)
#define PIN_UP     0
#define PIN_DOWN   1
#define PIN_RIGHT  2
#define PIN_LEFT   3

// buttons (digital)
#define PIN_HOME   4
#define PIN_A      5
#define PIN_B      6
#define PIN_X      7
#define PIN_Y_BTN  8
#define PIN_L_BTN  9
#define PIN_ZL     10
#define PIN_R_BTN  11
#define PIN_ZR     12

// Piezo ADC (RP2040 ADC0〜3)
#define PIEZO_L     26
#define PIEZO_RIGHT 27
#define PIEZO_Y     28
#define PIEZO_R     29

// =====================================================
// HORIPAD Button Bits（実機検証配置）
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
  0x35,0x00,0x45,0x01,
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

GamepadReport gp_report = {0,0x08,128,128,128,128,0};

// =====================================================
// Piezo設定（自己キャリブ）
// =====================================================
#define PIEZO_COUNT 4
uint8_t piezoPins[PIEZO_COUNT] = {
  PIEZO_L,
  PIEZO_RIGHT,
  PIEZO_Y,
  PIEZO_R
};

uint16_t piezoBaseline[PIEZO_COUNT];
uint16_t piezoThreshold[PIEZO_COUNT];

#define THRESHOLD_OFFSET 120   // 感度調整

// =====================================================
void setButton(uint16_t mask, bool pressed){
  if(pressed) gp_report.buttons |= mask;
  else gp_report.buttons &= ~mask;
}

// =====================================================
// D-Pad → HAT
// =====================================================
void updateHat(){

  bool up    = !digitalRead(PIN_UP);
  bool down  = !digitalRead(PIN_DOWN);
  bool right = !digitalRead(PIN_RIGHT);
  bool left  = !digitalRead(PIN_LEFT);

  if(up && !left && !right) gp_report.hat=0;
  else if(up && right)      gp_report.hat=1;
  else if(right && !up)     gp_report.hat=2;
  else if(down && right)    gp_report.hat=3;
  else if(down && !right)   gp_report.hat=4;
  else if(down && left)     gp_report.hat=5;
  else if(left && !down)    gp_report.hat=6;
  else if(up && left)       gp_report.hat=7;
  else                      gp_report.hat=8;
}

// =====================================================
// 起動時 自動感度キャリブレーション
// =====================================================
void calibratePiezo(){

  for(int i=0;i<PIEZO_COUNT;i++){
    uint32_t sum=0;

    for(int n=0;n<200;n++){
      sum += analogRead(piezoPins[i]);
      delay(2);
    }

    piezoBaseline[i]=sum/200;
    piezoThreshold[i]=piezoBaseline[i]+THRESHOLD_OFFSET;
  }
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

  calibratePiezo();   // ← 電源ON自己調整

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

  // =================================================
  // PIEZO ADC入力（リトリガー無し）
  // =================================================
  for(int i=0;i<PIEZO_COUNT;i++){

    uint16_t v = analogRead(piezoPins[i]);
    bool hit = (v > piezoThreshold[i]);

    switch(i){
      case 0: setButton(BTN_L, hit); break;
      case 1: if(hit) gp_report.hat=2; break; // →右
      case 2: setButton(BTN_Y, hit); break;
      case 3: setButton(BTN_R, hit); break;
    }
  }

  if(usb_gamepad.ready()){
    usb_gamepad.sendReport(0,&gp_report,sizeof(gp_report));
  }

  delay(5);
}