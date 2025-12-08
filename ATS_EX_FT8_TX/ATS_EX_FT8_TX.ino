#include "si5351.h"
#include "Wire.h"

Si5351 si5351;
unsigned long  freq;

void OnMessage(String message) 
{
  char input[message.length() + 1]; 
  message.toCharArray(input, sizeof(input)); 

  char command[20] = {0};  // Vorinitialisierte Arrays vermeiden Fragmentierung
  char parameter[20] = {0};
  char output[20] = {0};

  char *start = strchr(input, '<');
  char *mid = strchr(input, '>'); 
  char *end = strrchr(input, '<'); 

  if (start && mid && end && start < mid && mid < end) 
  { 
    start++;

    strncpy(command, start, mid - start);
    command[mid - start] = '\0';

    strncpy(parameter, mid + 1, end - mid - 1);
    parameter[end - mid - 1] = '\0';

    if (strcmp(command, "Freq") == 0) 
    {
      if(parameter[0] == '/')
      {
        //Serial.println("Wrong mode!");
        return;
      }

      int j = 0;

      for (int i = 0; i < strlen(parameter); i++) 
      {
          if (parameter[i] != '.') 
          {  
              output[j++] = parameter[i];
          }
      }
      
      output[j] = '\0';  // Nullterminator setzen!

      freq = atof(output);  // Umwandlung zu float und setzen der Frequenz
    }
  } 
  else 
  {
    //Serial.println("Error: Wrong format!");
  }
}

void readSerial() 
{
  static char buffer[100];
  static int index = 0;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || index == sizeof(buffer) - 1) { // Nachricht beendet oder Puffer voll
      buffer[index] = '\0'; // Null-Terminierung
      OnMessage(buffer);
      index = 0;  // Puffer zurücksetzen
    } else {
      buffer[index++] = c;
    }
  }
}

void setup(void)
{
  Serial.begin(9600);
  freq= 7074000;
  word cal_factor = 0;
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0); 
  si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);
 
  si5351.set_freq(freq*100ULL, SI5351_CLK2);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
  si5351.output_enable(SI5351_CLK2, 0);
  TCCR1A = 0x00;
  TCCR1B = 0x01; // Timer1 Timer 16 MHz
  TCCR1B = 0x81; // Timer1 Input Capture Noise Canceler
  ACSR |= (1<<ACIC);  // Analog Comparator Capture Input
  pinMode(7, INPUT); //PD7 = AN1 = HiZ, PD6 = AN0 = 0
  pinMode(13, OUTPUT);
}


void loop(void)
{
  readSerial();
 // Modulationsfrequenz messen über Analog Comparator Pin7 = AN1

 unsigned int d1,d2;
 int FSK = 10;
 int FSKtx = 0;
 while (FSK>0){
  TCNT1 = 0;
  while (ACSR &(1<<ACO)){
    if (TCNT1>65000) {break;
  }
  }  while ((ACSR &(1<<ACO))==0){
    if (TCNT1>65000) {break;}
  }
  TCNT1 = 0;
  while (ACSR &(1<<ACO)){
    if (TCNT1>65000) {break;}
  }
  d1 = ICR1;  
  while ((ACSR &(1<<ACO))==0){
    if (TCNT1>65000) {break;}
  } 
  while (ACSR &(1<<ACO)){
    if (TCNT1>65000) {break;}
  }
  d2 = ICR1;
  if (TCNT1 < 65000){
  unsigned long codefreq = 1600000000/(d2-d1);
    if (codefreq < 350000){
      if (FSKtx == 0){
          digitalWrite(13,1);
          // TX on
          si5351.output_enable(SI5351_CLK2, 1);   // TX on
          Serial.println("<TX>1</TX>");
      }
      si5351.set_freq((freq * 100 + codefreq), SI5351_CLK2);  
      FSKtx = 1;
    }
  }
  else{
    FSK--;
  }
 }
  digitalWrite(13,0);
  si5351.output_enable(SI5351_CLK2, 0);   //TX off
  Serial.println("<TX>0</TX>");
  FSKtx = 0;
}
