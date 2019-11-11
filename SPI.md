# SPI Bus
## Interface
The AC is the SPI master and the ESP8266 is the SPI slave. MHI uses signals: SCK, MOSI, MISO.  A slave select signal is not supported.

Name | Function |input/output
------------ | ------------- |--------------
SCK | serial clock | Clock output for AC, input for ESP8266
MOSI | Master Out, Slave In | Data output for MHI, input for ESP8266
MISO | Master In, Slave Out | Input for MHI, output for ESP8266

## Protocol
Clock polarity: CPOL=1 => clock idles at 1, and each cycle consists of a pulse of 0
Clock timing: CPHA=1 => data is captures with the rising clock edge, data changes with the falling edge
## Timing
A byte consists of 8 bits. SCK has a frequency of 32kHz. One bit takes 31.25µs, so one byte takes 8x31.25µs=250µs. There is a pause of 250µs between two bytes.
A frame consists of 20 bytes. A frame consumes 20x2x250µs=10ms. Between 2 frames is a pause of 40ms. So 20 frames per second will be transmitted. The following oscilloscope screenshot shows 3 bytes:
![SPI timing](/images/ScreenImg-11-cut.png)

Yellow: SCK; Purple: MOSI
# SPI Frame
A frame starts with three signature bytes, followed by 15 data bytes and 2 bytes for a checksum. The following table shows the content of a frame.
## Signature
The MOSI signature bytes indicate the start of a frame with the 3 bytes 0x6c, 0x80, 0x04.
For MISO seems to be more than one signature, usually it is 0xa9, 0x00, 0x07.

## Data
The following clauses describe the MOSI decoding for power, mode, fan, vanes, temperature setpoint and the room temperature
### Power
Power status is coded in MOSI data byte 0 (bit 0).

data byte 0	| Function
---- | -----
Bit 0| Power
0 | off
1 | on

### Mode
The mode is coded in MOSI data byte 0 (bit 4 ... 2).
<table style="width: 273px; height: 68px;">
<thead>
<tr>
<td style="width: 66.9667px;" colspan="3"><strong>data byte 0</strong></td>
<td style="width: 66.9667px;"><strong>Function</strong></td>
</tr>
</thead>
<tbody>
<tr>
<td style="width: 66.9667px;">bit 4</td>
<td style="width: 71.4333px;">bit 3</td>
<td style="width: 66.9667px;">bit 2</td>
<td style="width: 66.9667px;">Mode</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">0</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">Auto</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">0</td>
<td style="width: 66.9667px;">1</td>
<td style="width: 66.9667px;">Dry</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">1</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">Cool</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">1</td>
<td style="width: 66.9667px;">1</td>
<td style="width: 66.9667px;">Fan</td>
</tr>
<td style="width: 66.9667px;">1</td>
<td style="width: 71.4333px;">0</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">Heat</td>
</tr>
</tbody>
</table>

### Fan
The fan level is coded in MOSI data byte 1 bit [1:0] and in data byte 6 (bit 6).
<table style="width: 273px; height: 68px;">
<thead>
<tr>
<td style="width: 66.9667px;" colspan="2"><strong>data byte 1</strong></td>
<td style="width: 66.9667px;"><strong>data byte 6</strong></td>
<td style="width: 66.9667px;"><strong>Function</strong></td>
</tr>
</thead>
<tbody>
<tr>
<td style="width: 66.9667px;">bit 1</td>
<td style="width: 71.4333px;">bit 0</td>
<td style="width: 66.9667px;">bit 6</td>
<td style="width: 66.9667px;">Fan</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">0</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">1</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">1</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">2</td>
</tr>
<tr>
<td style="width: 66.9667px;">1</td>
<td style="width: 71.4333px;">0</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">3</td>
</tr>
<tr>
<td style="width: 66.9667px;">x</td>
<td style="width: 71.4333px;">x</td>
<td style="width: 66.9667px;">1</td>
<td style="width: 66.9667px;">4</td>
</tr>
</tbody>
</table>

### Vanes
The vanes upd/down level is coded in MOSI data byte 1 bit [5:4] and in data byte 0 (bit 6).
<table style="width: 273px; height: 68px;">
<thead>
<tr>
<td style="width: 66.9667px;" colspan="2"><strong>data byte 1</strong></td>
<td style="width: 66.9667px;"><strong>data byte 0</strong></td>
<td style="width: 66.9667px;"><strong>Function</strong></td>
</tr>
</thead>
<tbody>
<tr>
<td style="width: 66.9667px;">bit 5</td>
<td style="width: 71.4333px;">bit 4</td>
<td style="width: 66.9667px;">bit 6</td>
<td style="width: 66.9667px;">Vanes (up/down)</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">0</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">1</td>
</tr>
<tr>
<td style="width: 66.9667px;">0</td>
<td style="width: 71.4333px;">1</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">2</td>
</tr>
<tr>
<td style="width: 66.9667px;">1</td>
<td style="width: 71.4333px;">0</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">3</td>
</tr>
<tr>
<td style="width: 66.9667px;">1</td>
<td style="width: 71.4333px;">1</td>
<td style="width: 66.9667px;">0</td>
<td style="width: 66.9667px;">4</td>
</tr>
<tr>
<td style="width: 66.9667px;">x</td>
<td style="width: 71.4333px;">x</td>
<td style="width: 66.9667px;">1</td>
<td style="width: 66.9667px;">swing</td>
</tr>
</tbody>
</table>
Vanes status is not updated when using wireless IR remote control!

### Room temperature
The room temperature is coded in MOSI data byte 3, bit [7:0] according to the  formula T[°C]=(db3[7:0]-61)/4
The resolution is 0.25°C. But for displaying the room temperature you shouldn't use this resolution to prevent displaying the toggling of the least significant bit.

### Temperature setpoint
The temperature setpoint is coded in MOSI data byte 2, bit [6:0] according to the formula T[°C]=db2[6:1]/2
The resolution of 0.5°C is supported by the wired remote control [RC-E5](https://www.mhi-mth.co.jp/en/products/pdf/pjz012a087b_german.pdf). The wireless IR remote control supports a resolution of 1°C.

## Checksum
The checksum is calculated by the sum of the signature bytes plus the databytes. The low byte of the checksum is stored at byte position 18 and the low byte of the checksum is stored at byte position 19. Maximum value of the checksum is 0x0fe1. Therefore bit [7:4] of the checksum high byte is always 0.

## Variants
**The following chapter is in draft status!**
Different variants are possible when a remote control is connected via SPI. The MISO frame (data from RC to MHI-AC) requests data. The variant of the data is identified via MISO-db9. MHI-AC answers with the same value in MOSI-db9 (but only when bit2 of MISO-db14 is set).

The following screenshot shows some SPI traffic:
![MISO MOSI traffic](/images/MISO-MOSI_1.JPG)

In the marked row MISO-db9=0x80 and bit2 of MISO-db14 is set, so variant 0 is selected. MOSI-db9=0x80 and MOSI-db11 represents the outdoor temperature

data byte 9	| Variant | description
---- | ----- | -----
0xff| default | default when no SPI RC is connected or no 'special' data are requested
0x80| 0 | db10=?, Outdoor temperature: T[°C]=(db11[7:0]-94)/4 (formula is not finally confirmed)
0x32| 1 | db10=?, db11=?
0xf1| 2 | db10=?, db11=?
0xd2| 3 | db10=?, db11=?, only seen once during heating

note: the numbering of the variants 0..2 is reused from [rjdekker's code here](https://raw.githubusercontent.com/rjdekker/MHI2MQTT/master/src/MHI-SPI2ESP.ino)
