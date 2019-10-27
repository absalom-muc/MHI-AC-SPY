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
The signature bytes indicate the start of a frame with the 3 bytes 0x6d, 0x80, 0x08.
## Data
The following clauses describe the decoding for power, mode, fan status, temperature setpoint and the room temperature
### Power
Power status is coded in data byte 0 (bit 0).

db0	| Function
---- | -----
Bit 0| Power
0 | off
1 | on

### Mode
The mode is coded in data byte 0 (bit 4 ... 2).

db0 | db0 | db0 | Function
----| --- | --- | ---
Bit 4	| Bit 3 |	Bit 2 | Mode
0 |	0 |	0 |	Auto
0 |	0 |	1 |	Dry
0 |	1 |	0 |	Cool
0 |	1 |	1 |	Fan
1 |	0 |	0 |	Heat

### Fan
The fan level is coded in data byte 1 bit [1:0] and in data byte 6 (bit 6).
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

### Room temperature
The room temperature is coded in data byte 3, bit [7:0] according to the  formula T[°C]=(db3[7:0]-61)/4

### Temperature setpoint
The temperature setpoint is coded in data byte 2, bit [6:1] according to the formula T[°C]=db2[6:1]>>2

## Checksum
The checksum is calculated by the sum of the signature bytes plus the databytes. The low byte of the checksum is stored at byte position 18 and the low byte of the checksum is stored at byte position 19. Maximum value of the checksum is 0x0fe1. Therefore bit [7:4] of the checksum high byte is always 0.



 





