# MHI-AC-SPY
The program reads data (e.g. room temperature, fan status etc.) from a Mitsubishi Heavy Industries (MHI)
air conditioner (AC) via SPI and sends the data via MQTT. The AC is the SPI master and the ESP8266 is the SPI slave.
So far only reading from the AC is supported. (but writing is planned for the future)
The program has a draft status and your input is welcome.
My target is to read and write data with an ESP8266.

# Attention:
You have to open the indoor unit to have access to the SPI. Opening of the indoor unit should be done by 
a qualified professional because faulty handling may cause leakage of water, electric shock or fire!

# Prerequisites:
For use of the program you have to connect your ESP8266 (I use a LOLIN(WEMOS) D1 R2 & mini with 80 MHz) via a
cable connector to your air conditioner. This has to be a split device (separated indoor and outdoor unit).
I assume that all indoor units of the type SRC xx ZS-S are supported. I use the indoor unit SRK 35 ZS-S and the outdoor unit SRC 35 ZS-S.
  
# Installing:

## Hardware:
The ESP8266 is powered from the AC via DC-DC (12V -> 5V) converter
The ESP8266 SPI signals SCL (SPI clock) and MOSI (Master Out Slave In) signals are connected via a voltage divider with the AC.
More details are described [here](/hardware.md)

## Software:
The program uses the [MQTT client library](https://github.com/knolleary/pubsubclient) from Nick O'Leary (knolleary). Thank you Nick - great work!
Please check his GitHub page to see how to install it.
Dwonload MHI-SPI.ino and adapt ssid, password and the MQTT server data in the row MQTTclient.setServer("ds218p", 1883);

# License
This project is licensed under the MIT License - see the LICENSE.md file for details

# Acknowledgments
The coding of the SPI protocol of the AC is a nightmare. Without [rjdekker's MHI2MQTT](https://github.com/rjdekker/MHI2MQTT) I had no chance to understand the protocol! Unfortunatelly rjdekker is not longer active on GitHub. He used an Arduino plus an ESP8266 for his project.
