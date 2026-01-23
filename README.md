# FT8 Transceiver

My QRP FT8 Transceiver is basded on a ATS20+ Receiver and on the FT8 project on the website https://www.elektronik-labor.de/HF/FT8QRP.html .

The new Firmware of the ATS20+ is based on the firmware from Goshante: https://github.com/goshante/ats20_ats_ex .  
The Firmware of the TX part is based on these project: https://www.elektronik-labor.de/HF/FT8QRP.html .

I made modifications on both source codes.

Please note that the Microcontroller of the TX part has to be a Nano, not a Nano R4, because I use the comperator input D6 / D7. The Nano R4 don't has these input.

The PA is a 2W PA from China. For this use it's totally OK.
In the end of these document you can find pictures of all parts I bought.

## Overview:
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/1000008323.jpg)

![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/003.jpg)

## Description:

On the ATS20+ Board we have to make some small modifications:

- Replace the Arduino Nano with a Arduino Nano R4. The "old" Nano you can use for the TX part.
- Remove the battery
- Remove U1 (TP4056).
- Upload the fimrware in the folder ATS_EX.



## Details:
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/004.jpg)

![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/005_1.jpg)

![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/006.jpg)

![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/007.jpg)

## Used parts:
ATS20+
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/ATS20p.jpg)

---------------------------------------------------------------------------------------------------

PowerUnit HW688
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/HW688.jpg)

---------------------------------------------------------------------------------------------------

PA
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/PA.jpg)

At 12V VCC and 7Mhz / 1mW ( 0dBm ) input it has around +33dBm ( 2W )

---------------------------------------------------------------------------------------------------

LPF
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/LPF.jpg)

---------------------------------------------------------------------------------------------------

Relais
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/Relais.jpg)

---------------------------------------------------------------------------------------------------

Si5351
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/Si5351.jpg)

---------------------------------------------------------------------------------------------------

Nano R4
![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/nanoR4.jpg)
