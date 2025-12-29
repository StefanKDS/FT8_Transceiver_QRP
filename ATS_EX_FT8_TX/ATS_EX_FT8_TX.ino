#include "si5351.h"  // Bibliothek für den SI5351-Frequenzgenerator
#include "Wire.h"    // I2C-Bibliothek (wird vom SI5351 verwendet)

Si5351 si5351;

unsigned long freq = 7074000; // Startfrequenz 7,074 MHz (typische FT8-Frequenz)
bool tx_on = false;           // Statusvariable, ob TX aktiv ist

// ------------------------------------------------------------
// UART MESSAGE HANDLING (Verarbeitung eingehender UART-Nachrichten)
// ------------------------------------------------------------
void OnMessage(char input[100])
{
  char command[20] = {0};
  char parameter[20] = {0};
  char output[20] = {0};

  char *start = strchr(input, '<');
  char *mid   = strchr(input, '>');
  char *end   = strstr(input, "</");

  if (!start || !mid || !end) return;
  if (!(start < mid && mid < end)) return;

  int cmdLen = mid - start - 1;
  if (cmdLen <= 0 || cmdLen >= sizeof(command)) return;
  strncpy(command, start + 1, cmdLen);
  command[cmdLen] = '\0';

  int parLen = end - mid - 1;
  if (parLen <= 0 || parLen >= sizeof(parameter)) return;
  strncpy(parameter, mid + 1, parLen);
  parameter[parLen] = '\0';

  if (strcmp(command, "Freq") != 0) return;
  if (parameter[0] == '/') return;

  int j = 0;
  for (int i = 0; parameter[i] && j < sizeof(output) - 1; i++)
  {
    if (parameter[i] != '.')
      output[j++] = parameter[i];
  }
  output[j] = '\0';

  if (j == 0) return;

  freq = atol(output) * 1000;
}

// ------------------------------------------------------------
// Serial-Daten einlesen und Puffer aufbauen
// ------------------------------------------------------------
void readSerial()
{
  static char buffer[100];
  static uint8_t index = 0;

  while (Serial.available())
  {
    char c = Serial.read();

    if (c == '\n')
    {
      buffer[index] = '\0';
      if (index > 0) OnMessage(buffer);
      index = 0;
    }
    else if (c != '\r')
    {
      if (index < sizeof(buffer) - 1)
        buffer[index++] = c;
      else
        index = 0;
    }
  }
}

// ------------------------------------------------------------
// TX Steuerung
// ------------------------------------------------------------
void startTX()
{
  if (tx_on) return;

  tx_on = true;
  si5351.output_enable(SI5351_CLK2, 1);
  digitalWrite(13, HIGH);
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);
  digitalWrite(9, LOW);
  Serial.println("<TX>1</TX>");
}

void stopTX()
{
  if (!tx_on) return;

  tx_on = false;
  si5351.output_enable(SI5351_CLK2, 0);
  digitalWrite(13, LOW);
  digitalWrite(4, HIGH);
  digitalWrite(5, LOW);
  digitalWrite(9, HIGH);
  Serial.println("<TX>0</TX>");
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup()
{
  Serial.begin(9600);

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_correction(0, SI5351_PLL_INPUT_XO);
  si5351.set_freq(freq * 100ULL, SI5351_CLK2);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
  si5351.output_enable(SI5351_CLK2, 0);

  TCCR1A = 0x00;
  TCCR1B = 0x81;
  ACSR  |= (1 << ACIC);

  pinMode(7, INPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(13, OUTPUT);

  digitalWrite(4, HIGH);
  digitalWrite(5, LOW);
  digitalWrite(9, HIGH);

  stopTX();
}

// ------------------------------------------------------------
// Hauptschleife
// ------------------------------------------------------------
void loop()
{
  readSerial();

  unsigned int d1, d2;
  int FSK = 10;
  bool fskActive = false;

  while (FSK > 0)
  {
    readSerial();
    TCNT1 = 0;

    // high -> low
    while (ACSR & (1 << ACO))
    {
      readSerial();
      if (TCNT1 > 65000) break;
    }

    // low -> high
    while (!(ACSR & (1 << ACO)))
    {
      readSerial();
      if (TCNT1 > 65000) break;
    }

    TCNT1 = 0;

    // high
    while (ACSR & (1 << ACO))
    {
      readSerial();
      if (TCNT1 > 65000) break;
    }

    d1 = ICR1;

    // low
    while (!(ACSR & (1 << ACO)))
    {
      readSerial();
      if (TCNT1 > 65000) break;
    }

    // high
    while (ACSR & (1 << ACO))
    {
      readSerial();
      if (TCNT1 > 65000) break;
    }

    d2 = ICR1;

    if (TCNT1 < 65000 && d2 > d1)
    {
      unsigned long codefreq = 1600000000UL / (d2 - d1);

      if (codefreq < 350000)
      {
        si5351.set_freq((freq * 100ULL) + codefreq, SI5351_CLK2);
        startTX();
        fskActive = true;
      }
    }
    else
    {
      FSK--;
    }
  }

  if (!fskActive)
  {
    stopTX();
  }
}
