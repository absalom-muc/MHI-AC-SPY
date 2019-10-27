# MHI-SPY
The intention is to share a program for reading data (e.g. room temperature, fan status etc.) from
a Mitsubishi Heavy Industries (MHI) air conditioner (AC) and send the data via MQTT.

My air conditioner:
  Indoor unit: SRK 35 ZS-S
  Outdoor unit: SRC 35 ZS-S
  
Used Hardware: ESP8266 (I use a LOLIN(WEMOS) D1 R2 & mini with 80 MHz)

The data are read via SPI from the AC. The AC is the SPI master and the ESP8266 is the SPI slave.
So far only reading from the AC is supported

Attention: You have to open the indoor unit to have access to the SPI. Opening of the indoor unit should be done by 
a qualified professional because faulty handling may cause leakage of water, electric shock or fire!
