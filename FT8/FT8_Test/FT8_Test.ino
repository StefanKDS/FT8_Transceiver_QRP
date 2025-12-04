#include "si5351.h"
#include "Wire.h"

Si5351 si5351;
unsigned long freq = 7074000;

volatile unsigned long lastCaptureTime = 0;
volatile unsigned long period = 0;
volatile bool newPeriodAvailable = false;

const int inputPin = 16;  // Signal Eingang, A2 als digitaler eingang
const int ledPin = 13;

void IRAM_ATTR onSignalChange() {
  unsigned long now = micros();
  if (lastCaptureTime != 0) {
    period = now - lastCaptureTime;
    newPeriodAvailable = true;
  }
  lastCaptureTime = now;
}

void setup() {
  pinMode(inputPin, INPUT);
  pinMode(ledPin, OUTPUT);

  Wire.begin();
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_correction(0, SI5351_PLL_INPUT_XO);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.set_freq(freq * 100ULL, SI5351_CLK1);
  si5351.output_enable(SI5351_CLK1, 1);

  si5351.set_freq(freq * 100ULL, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  si5351.output_enable(SI5351_CLK0, 0);

  attachInterrupt(digitalPinToInterrupt(inputPin), onSignalChange, CHANGE);
}

void loop() {
  static int FSK = 10;
  static int FSKtx = 0;

  if (newPeriodAvailable) {
    noInterrupts();
    unsigned long measuredPeriod = period;
    newPeriodAvailable = false;
    interrupts();

    if (measuredPeriod > 0) {
      unsigned long codefreq = 1000000UL / measuredPeriod; // Frequenz aus Periodendauer in Mikrosekunden

      if (codefreq < 350000) {
        if (FSKtx == 0) {
          digitalWrite(ledPin, HIGH);
          si5351.output_enable(SI5351_CLK1, 0);   // RX off
          si5351.output_enable(SI5351_CLK0, 1);   // TX on
          FSKtx = 1;
        }
        si5351.set_freq((freq * 100ULL + codefreq), SI5351_CLK0);
        FSK = 10;
      } else {
        FSK--;
      }
    } else {
      FSK--;
    }
  }

  if (FSK <= 0) {
    digitalWrite(ledPin, LOW);
    si5351.output_enable(SI5351_CLK0, 0);   // TX off
    si5351.output_enable(SI5351_CLK1, 1);   // RX on
    FSKtx = 0;
    FSK = 10;
  }
}
