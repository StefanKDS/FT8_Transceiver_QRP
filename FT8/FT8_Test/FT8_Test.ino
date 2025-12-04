// FT8QRP – Nano R4 Version ohne Analog Comparator
// Signal-Eingang jetzt an A2 (digitaler Interrupt)

#include "si5351.h"
#include "Wire.h"

Si5351 si5351;

const uint8_t SIG_PIN = A2;     // Eingang für FT8-Signal
const uint8_t LED_PIN = 13;     // Status-LED

unsigned long freq = 7074000UL;

// Variablen für Interrupt-basierte Periodenmessung
volatile uint32_t lastMicros = 0;
volatile uint32_t periodMicros = 0;
volatile bool newPeriod = false;


// ------------------------------------------------------
// ISR – misst die Zeit zwischen zwei steigenden Flanken
// ------------------------------------------------------
void IRAM_ATTR isr_edge() {
  uint32_t now = micros();
  if (lastMicros != 0) {
    periodMicros = now - lastMicros;
    newPeriod = true;
  }
  lastMicros = now;
}


// ------------------------------------------------------
// SETUP
// ------------------------------------------------------
void setup() {
  Wire.begin();

  // Si5351 initialisieren
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_correction(0, SI5351_PLL_INPUT_XO);

  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);

  // RX (CLK1) aktiv
  si5351.set_freq(freq * 100ULL, SI5351_CLK1);
  si5351.output_enable(SI5351_CLK1, 1);

  // TX (CLK0) vorbereiten
  si5351.set_freq(freq * 100ULL, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  si5351.output_enable(SI5351_CLK0, 0);

  // Signal-Eingang
  pinMode(SIG_PIN, INPUT);

  // Interrupt auf steigende Flanke
  attachInterrupt(digitalPinToInterrupt(SIG_PIN), isr_edge, RISING);

  pinMode(LED_PIN, OUTPUT);
}


// ------------------------------------------------------
// LOOP – ersetzt die komplette Comparator/Timer1-Logik
// ------------------------------------------------------
void loop() {

  int FSK = 10;
  bool FSKtx = false;

  while (FSK > 0) {

    // Gibt es eine neue Periodenmessung?
    if (newPeriod) {
      noInterrupts();
      uint32_t p = periodMicros;
      newPeriod = false;
      interrupts();

      // Schutz gegen ungültige Werte
      if (p == 0 || p > 50000) {  
        FSK--;
        continue;
      }

      // Frequenz berechnen (Hz)
      unsigned long codefreq = 1000000UL / p;

      // Schutzbereich wie im Original (<350k)
      if (codefreq < 350000UL) {

        // TX einschalten, wenn noch nicht aktiv
        if (!FSKtx) {
          digitalWrite(LED_PIN, HIGH);
          si5351.output_enable(SI5351_CLK1, 0);   // RX aus
          si5351.output_enable(SI5351_CLK0, 1);   // TX an
          FSKtx = true;
        }

        // Frequenz verschieben: Basis + FT8-Ton
        si5351.set_freq((freq * 100ULL + codefreq), SI5351_CLK0);
      }
    } 
    else {
      // Wenn längere Zeit keine Flanke -> Schleife verlassen
      FSK--;
      delay(2);
    }
  }

  // TX aus, RX an
  digitalWrite(LED_PIN, LOW);
  si5351.output_enable(SI5351_CLK0, 0);
  si5351.output_enable(SI5351_CLK1, 1);
}
