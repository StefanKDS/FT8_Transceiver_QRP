#include "Arduino.h"
#include "SimpleButton.h"

#define BUTTONSTATE_IDLE          0
#define BUTTONSTATE_DEBOUNCE      1
#define BUTTONSTATE_RELEASE       2
#define BUTTONSTATE_PRESSED       3
#define BUTTONSTATE_LONGPRESS     4
#define BUTTONSTATE_LONGRELEASE   5
#define BUTTONSTATE_SHORTRELEASE  6


SimpleButton::SimpleButton(uint8_t pin)
{
    // R4 / ARM-kompatibel
    pinMode(pin, INPUT_PULLUP);

    // Speicherung wie im Original: pin (oben), debounce (Mitte), state unten
    _PinDebounceState = ((uint16_t)pin << 10);
}


uint8_t SimpleButton::checkEvent(uint8_t (*_event)(uint8_t event, uint8_t pin))
{
    uint8_t ret = 0;
    uint16_t timeNow = millis() & 0x3f0;

    uint16_t state    = _PinDebounceState & 0x0f;
    uint16_t debounce = _PinDebounceState & 0x3f0;
    uint8_t  pin      = _PinDebounceState >> 10;

    uint8_t pinState = digitalRead(pin);     // <-- Nano R4 kompatibel

    // 16ms-Zeitticks zur Erkennung von Überlauf
    if (timeNow < debounce)
        timeNow += 0x400;

    uint16_t elapsed = timeNow - debounce;


    switch(state)
    {
    case BUTTONSTATE_IDLE:
        if (pinState == LOW)
        {
            state = BUTTONSTATE_DEBOUNCE;
            debounce = timeNow;
        }
        break;

    case BUTTONSTATE_DEBOUNCE:
        if (pinState == HIGH)
        {
            state = BUTTONSTATE_IDLE;
        }
        else if (elapsed >= BUTTONTIME_PRESSDEBOUNCE)
        {
            state = BUTTONSTATE_PRESSED;
        }
        break;

    case BUTTONSTATE_PRESSED:
        if (pinState == HIGH)
        {
            debounce = timeNow;
            state = BUTTONSTATE_SHORTRELEASE;
        }
        else if (elapsed >= BUTTONTIME_LONGPRESS1)
        {
            ret = BUTTONEVENT_FIRSTLONGPRESS;
            state = BUTTONSTATE_LONGPRESS;
            debounce = timeNow;
        }
        break;

    case BUTTONSTATE_LONGPRESS:
        if (pinState == HIGH)
        {
            state = BUTTONSTATE_LONGRELEASE;
        }
        else if (elapsed >= BUTTONTIME_LONGPRESSREPEAT)
        {
            debounce = timeNow;
            ret = BUTTONEVENT_LONGPRESS;
        }
        break;

    case BUTTONSTATE_LONGRELEASE:
        if (pinState == HIGH)
        {
            ret = BUTTONEVENT_LONGPRESSDONE;
            state = BUTTONSTATE_RELEASE;
            debounce = timeNow;
        }
        else
        {
            state = BUTTONSTATE_LONGPRESS;
        }
        break;

    case BUTTONSTATE_SHORTRELEASE:
        if (pinState == HIGH)
        {
            ret = BUTTONEVENT_SHORTPRESS;
            state = BUTTONSTATE_RELEASE;
        }
        else
        {
            state = BUTTONSTATE_PRESSED;
        }
        break;

    case BUTTONSTATE_RELEASE:
        if (pinState == HIGH)
        {
            if (elapsed >= BUTTONTIME_RELEASEDEBOUNCE)
                state = BUTTONSTATE_IDLE;
        }
        else
        {
            debounce = timeNow;
        }
        break;

    default:
        break;
    }


    // Neue Daten in den kompakten 16bit-Speicher schreiben
    _PinDebounceState = ( (pin << 10) | (debounce & 0x3f0) | state );


    // Event-CB aufrufen?
    if (ret)
    {
        if (_event)
            ret = _event(ret, pin);
    }
    else
    {
        if (state > BUTTONSTATE_RELEASE)
            ret = BUTTON_PRESSED;
    }

    return ret;
}
