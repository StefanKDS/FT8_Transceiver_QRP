#include "Arduino.h"
#include "Rotary.h"

#define R_START       0x0

#ifdef HALF_STEP
// (Dein Halb-Step table bleibt unverändert)
#else
#define R_CW_FINAL  0x1
#define R_CW_BEGIN  0x2
#define R_CW_NEXT   0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT  0x6

const unsigned char ttable[7][4] = 
{
  {R_START, R_CW_BEGIN, R_CCW_BEGIN, R_START},
  {R_CW_NEXT, R_START, R_CW_FINAL, R_START | DIR_CW},
  {R_CW_NEXT, R_CW_BEGIN, R_START, R_START},
  {R_CW_NEXT, R_CW_BEGIN, R_CW_FINAL, R_START},
  {R_CCW_NEXT, R_START, R_CCW_BEGIN, R_START},
  {R_CCW_NEXT, R_CCW_FINAL, R_START, R_START | DIR_CCW},
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif

// Constructor
Rotary::Rotary(char _pin1, char _pin2) 
{
  pin1 = _pin1;
  pin2 = _pin2;

  pinMode(pin1, INPUT);
  pinMode(pin2, INPUT);

#ifdef ENABLE_PULLUPS
  pinMode(pin1, INPUT_PULLUP);
  pinMode(pin2, INPUT_PULLUP);
#endif

  state = R_START;
}

unsigned char Rotary::process() 
{
    // Read pins via Arduino API (Nano R4 compatible)
    unsigned char s1 = digitalRead(pin1);
    unsigned char s2 = digitalRead(pin2);

    // Build pinstate exactly like old code (2-bit value)
    unsigned char pinstate = (s1 << 1) | s2;

    // State machine
    state = ttable[state & 0x0f][pinstate];

    return state & 0x30;
}