# FT8 Transceiver

My QRP FT8 Transceiver is basded on a ATS20+ Receiver and on the FT8 project on the website https://www.elektronik-labor.de/HF/FT8QRP.html .

The new Firmware of the ATS20+ is based on the firmware from Goshante: https://github.com/goshante/ats20_ats_ex .
The Firmware of the TX part is based on these project: https://www.elektronik-labor.de/HF/FT8QRP.html .

I made modifications on both source codes.

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

![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/005.jpg)

![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/006.jpg)

![](https://github.com/StefanKDS/FT8_Transceiver_QRP/blob/main/img/007.jpg)
