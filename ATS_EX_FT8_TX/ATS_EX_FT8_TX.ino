/*
 * This code implements the FT8 transmit section of a software-controlled 
 * radio using an Arduino and an Si5351 clock generator.
 * The Arduino receives frequency control commands via UART in a simple 
 * XML-like format (e.g. <Freq>7074</Freq>). 
 * These commands set the RF base frequency used for transmission.
 *
 * An audio-frequency FSK signal (FT8 tones) is fed into the Arduino’s 
 * analog comparator. Using Timer1 input capture, the code measures the 
 * period of this signal and calculates the corresponding audio frequency.
 * 
 * This audio tone frequency is then added as an offset to the RF base 
 * frequency, and the Si5351 output is updated accordingly. 
 * When a valid FT8 tone is detected, the transmitter is automatically enabled; 
 * if no valid tone is present, transmission is disabled.
 * 
 * Digital output pins control external TX/RX switching hardware and a status LED, 
 * while the Si5351 generates the final RF signal required for FT8 transmission.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "si5351.h"  // Library for controlling the Si5351 clock generator
#include "Wire.h"    // I2C library

Si5351 si5351;

unsigned long freq = 7074000; // Start frequency: 7.074 MHz (FT8 on 40m)
bool tx_on = false;           // Indicates whether TX is currently active

#define INTERNAL_TX_LED 13
#define FT8_INPUT       7
#define TX_RELAIS_OUTPUT 4
#define RED_TX_LED 5
#define GREEN_RX_LED 9

// ------------------------------------------------------------
// UART MESSAGE HANDLING
// Parses incoming UART messages (e.g. frequency commands)
// ------------------------------------------------------------
void OnMessage(char input[100])
{
  char command[20] = {0};
  char parameter[20] = {0};
  char output[20] = {0};

  // Find XML-like tags: <Command>Value</Command>
  char *start = strchr(input, '<');
  char *mid   = strchr(input, '>');
  char *end   = strstr(input, "</");

  // Abort if message format is invalid
  if (!start || !mid || !end) 
    return;
  if (!(start < mid && mid < end)) 
    return;

  // Extract command name
  int cmdLen = mid - start - 1;
  if (cmdLen <= 0 || cmdLen >= sizeof(command)) 
    return;
  strncpy(command, start + 1, cmdLen);
  command[cmdLen] = '\0';

  // Extract parameter value
  int parLen = end - mid - 1;
  if (parLen <= 0 || parLen >= sizeof(parameter)) 
    return;
  strncpy(parameter, mid + 1, parLen);
  parameter[parLen] = '\0';

  // Only accept "Freq" command
  if (strcmp(command, "Freq") != 0) 
    return;

  // Ignore commands starting with '/'
  if (parameter[0] == '/') 
    return;

  // Remove decimal point from frequency string
  int j = 0;
  for (int i = 0; parameter[i] && j < sizeof(output) - 1; i++)
  {
    if (parameter[i] != '.')
      output[j++] = parameter[i];
  }
  output[j] = '\0';

  if (j == 0) 
    return;

  // Convert frequency to Hz (kHz → Hz)
  freq = atol(output) * 1000;
}

// ------------------------------------------------------------
// Read serial data and build a line buffer
// ------------------------------------------------------------
void readSerial()
{
  static char buffer[100];
  static uint8_t index = 0;

  while (Serial.available())
  {
    char c = Serial.read();

    // End of line → process message
    if (c == '\n')
    {
      buffer[index] = '\0';
      if (index > 0) 
        OnMessage(buffer);
      index = 0;
    }
    // Ignore carriage return
    else if (c != '\r')
    {
      if (index < sizeof(buffer) - 1)
        buffer[index++] = c;
      else
        index = 0; // Buffer overflow protection
    }
  }
}

// ------------------------------------------------------------
// TX CONTROL
// Enables or disables the transmitter
// ------------------------------------------------------------
void startTX()
{
  if (tx_on) return;

  tx_on = true;
  si5351.output_enable(SI5351_CLK2, 1); // Enable RF output
  digitalWrite(INTERNAL_TX_LED, HIGH);               // TX LED on
  digitalWrite(TX_RELAIS_OUTPUT, LOW);
  digitalWrite(RED_TX_LED, HIGH);
  digitalWrite(GREEN_RX_LED, LOW);
  Serial.println("<TX>1</TX>");
}

void stopTX()
{
  if (!tx_on) return;

  tx_on = false;
  si5351.output_enable(SI5351_CLK2, 0); // Disable RF output
  digitalWrite(INTERNAL_TX_LED, LOW);                // TX LED off
  digitalWrite(TX_RELAIS_OUTPUT, HIGH);
  digitalWrite(RED_TX_LED, LOW);
  digitalWrite(GREEN_RX_LED, HIGH);
  Serial.println("<TX>0</TX>");
}

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------
void setup()
{
  Serial.begin(9600);

  // Initialize Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_correction(0, SI5351_PLL_INPUT_XO);
  si5351.set_freq(freq * 100ULL, SI5351_CLK2);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
  si5351.output_enable(SI5351_CLK2, 0);

  // Timer1 configuration (used for frequency measurement)
  TCCR1A = 0x00;
  TCCR1B = 0x81;        // No prescaler, input capture enabled
  ACSR  |= (1 << ACIC); // Analog comparator triggers input capture

  // Pin configuration
  pinMode(FT8_INPUT, INPUT);    // Comparator input
  pinMode(TX_RELAIS_OUTPUT, OUTPUT);
  pinMode(RED_TX_LED, OUTPUT);
  pinMode(GREEN_RX_LED, OUTPUT);
  pinMode(INTERNAL_TX_LED, OUTPUT);

  // Default output states
  digitalWrite(TX_RELAIS_OUTPUT, HIGH);
  digitalWrite(RED_TX_LED, LOW);
  digitalWrite(GREEN_RX_LED, HIGH);

  stopTX(); // Ensure TX is off at startup
}

// ------------------------------------------------------------
// MAIN LOOP
// Detects FSK tones and controls FT8 transmission
// ------------------------------------------------------------
void loop()
{
  readSerial();

  unsigned int d1, d2;
  int FSK = 10;            // Number of retries for detecting FSK
  bool fskActive = false; // Indicates whether valid FSK was detected

  while (FSK > 0)
  {
    readSerial();
    TCNT1 = 0;

    // Wait for high → low transition
    while (ACSR & (1 << ACO))
    {
      readSerial();
      if (TCNT1 > 65000) 
        break;
    }

    // Wait for low → high transition
    while (!(ACSR & (1 << ACO)))
    {
      readSerial();
      if (TCNT1 > 65000) 
        break;
    }

    TCNT1 = 0;

    // Measure high period
    while (ACSR & (1 << ACO))
    {
      readSerial();
      if (TCNT1 > 65000) 
        break;
    }

    d1 = ICR1;

    // Measure low period
    while (!(ACSR & (1 << ACO)))
    {
      readSerial();
      if (TCNT1 > 65000) 
        break;
    }

    // Measure next high period
    while (ACSR & (1 << ACO))
    {
      readSerial();
      if (TCNT1 > 65000) 
        break;
    }

    d2 = ICR1;

    // Calculate frequency from measured periods
    if (TCNT1 < 65000 && d2 > d1)
    {
      unsigned long codefreq = 1600000000UL / (d2 - d1);

      // Valid FT8 audio tone range
      if (codefreq < 350000)
      {
        // Set RF frequency + audio tone offset
        si5351.set_freq((freq * 100ULL) + codefreq, SI5351_CLK2);
        startTX();
        fskActive = true;
      }
    }
    else
    {
      FSK--; // Retry if invalid measurement
    }
  }

  // Stop TX if no valid FSK signal is detected
  if (!fskActive)
  {
    stopTX();
  }
}
