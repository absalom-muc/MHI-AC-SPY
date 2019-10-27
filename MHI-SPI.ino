// MHI-SPI
// read data via SPI and send via MQTT


#include "esp8266_peri.h"
//#include "ets_sys.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // see https://github.com/knolleary/pubsubclient

const char* ssid = "xxx";
const char* password = "xxx";

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

const int SYNC_STATUS=4; // Pin indicating the sync status
volatile int frame_sync = 0;
      // 0: not synchronized and armed
      // 1: synchronized, armed
      // 2: synchronized, valid frame received, but unchanged
      // 3: synchronized, new frame received
volatile int frame_sync_changed = 0;

uint frame_cnt=0;

unsigned long previousMillis = 0;
unsigned long asyncMillis = 0;
unsigned long runtimeMillis = 0;
const int length = 20;        // length of packet, so 19 should be enough
uint8_t rx_SPIframe[length];

#define SIGNBYTE1 0
#define SIGNBYTE2 1
#define DATABYTE0 SIGNBYTE2 + 1
#define DATABYTE1 SIGNBYTE2 + 2
#define DATABYTE2 SIGNBYTE2 + 3
#define DATABYTE3 SIGNBYTE2 + 4
#define DATABYTE6 SIGNBYTE2 + 7

void MQTTreconnect() {
  // Loop until we're reconnected
  while (!MQTTclient.connected()) {
    SPI1S |= SPISSRES;   //reset SPI
    update_frame_sync(0);
    Serial.print("Attempting MQTT connection...");
    if (MQTTclient.connect ("ESP8266Client-MHI-SPI", "MHI/connected", 0, true, "0")) {
      Serial.println("connected");
      MQTTclient.publish("MHI/connected", "1", true);
      MQTTclient.publish("MHI/synced", "0", true);  // notwendig???
    } else {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void ICACHE_RAM_ATTR sync_isr() {     // Sync pin interrupt on falling edge
  SPI1S |= SPISSRES;                  // SPISSRES=bit31, Reset HSPI slave (SPI_SYNC_RESET )
  SPI1S &= ~SPISSRES;
  SPI1CMD = SPICMDUSR;                // Start HSPI slave with enable user-defined commands
}

void ICACHE_RAM_ATTR _hspi_slave_isr_handler(void *) {
    uint32_t istatus = SPIIR;
    if(istatus & (1 << SPII1)) {      //SPI1 ISR (SPI1 Interrupt)
        uint32_t status = SPI1S;
        SPI1S &= ~(0x3E0);            //disable interrupts
        SPI1S |= SPISSRES;            //reset
        SPI1S &= ~(0x1F);             //clear interrupts
        SPI1S |= (0x3E0);             //enable interrupts

        if(status & SPISWBIS) {       // was it a SPI_SLV_WR_BUF_INT
          uint8_t *p = rx_SPIframe;
          int j=0;
          uint32_t dword;
          int diff=0;
          uint16_t sum = 0x6c;
          for(int i=0; i<19;i++) {
            if (i%4 == 0)
              dword = SPI1W(j++);
            if(*p != (uint8_t)dword)
              diff=1;
            *p = dword;
            if(i<17)
              sum += *p;
            p++;
            dword = dword>>8;
          }
          if(rx_SPIframe[0] != 0x80 | rx_SPIframe[1] != 0x04)
            update_frame_sync(0);
          else if(rx_SPIframe[17] != highByte(sum) | rx_SPIframe[18] != lowByte(sum))
            update_frame_sync(0);
          else if(diff == 0)
            update_frame_sync(2);
          else
            update_frame_sync(3);
          digitalWrite(LED_BUILTIN, LOW);
        }
    } else if(istatus & (1 << SPII0)) { //SPI0 ISR
        SPI0S &= ~(0x3ff);//clear SPI ISR
    } else if(istatus & (1 << SPII2)) {} //I2S ISR
}

void hspi_slave_begin() {
    pinMode(SCK, SPECIAL);                  // Both inputs in slave mode
    pinMode(MOSI, SPECIAL);
    // Take care, the register descriptions might be wrong!
    // Couldn't find a ESP8266 SPI register description
    SPI1C = 0x0628A000;                     // SPI_CTRL_REG LSB first, single bit rx_SPIframe mode.
                                            // bit 26 = 1 SPI_WR_BIT_ORDER =>  sends LSB first
                                            // bit 25 = 1 SPI_RD_BIT_ORDER =>  sends LSB first
                                            // bit 24 = 0 SPI_FREAD_QIO  =>  no
                                            // bit 23 = 0 SPI_FREAD_DIO => disable
                                            // bit 21 = 1 SPI_WP =>  output high
                                            // bit 20 = 0 SPI_FREAD_QUAD => disable
                                            // die 8 von 0x0628A000 zeigt auf reserved bits
                                            // das oberste bit von A in 0x0628A000 zeigt auf reserved bits
                                            // bit 14 = 0 SPI_FREAD_DUAL => disable
                                            // bit 13 = 1 SPI_FASTRD_MODE => enable
                                            
    SPI1S = SPISE | SPISBE | SPISCD | 0x3E0;// SPI_SLAVE_REG, set slave mode, WR/RD BUF enable, CMD define, enable interrupts
                                            // bit 30 = 1 (SPISE) SPI_SLAVE_MODE => slave mode
                                            // bit 29 = 1 (SPISBE) SPI_SLV_WR_RD_BUF_EN => enables write and read buffer commands in slave mode
                                            // bit 27 = 1 (SPISCD) SPI_SLV_CMD_DEFINE =>  slave mode commands are defined in SPI_SLAVE3
    SPI1U=0x00000040;                       // evtl. SPI_CK_I_EDGE In slave mode, the bit is the same as SPI_CK_OUT_EDGE in master mode. It is combined with SPI_MISO_DELAY_MODE
    SPI1CLK = 0;                            // SPI_CLOCK_REG
    SPI1U1 = 7 << SPILADDR;                 // SPI_USER1_REG, set address length to 8 bits - needed, but meaning is not clear
    SPI1U2 = 7 << SPILCOMMAND;              // SPI_USER2_REG, set command length to 8 bits
    SPI1S1 = (19 * 8 - 1) << SPIS1LBUF;     // SPI_SLAVE1_REG, SPI_SLV_BUF_BITLEN - doesn't really match SPI_SLAVE1_REG description
    SPI1S3 = 0xF1F26CF3;                    // SPI_SLAVE3_REG, SPI_SLV_RDSTA_CMD_VALUE  = 0x6c
    SPI1P = 0x20080000;                     // SPI_PIN_REG, Clock idle high, seems to cause contension on the clock pin if set to idle low.
    frame_sync=0;
    ETS_SPI_INTR_ATTACH(_hspi_slave_isr_handler, 0);
    ETS_SPI_INTR_ENABLE();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.printf("%lu:Klima SPI Sniffer starting\n", millis());
  pinMode(LED_BUILTIN, OUTPUT);  // used to indicate that a frame was received, active low
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(SYNC_STATUS, OUTPUT);
  digitalWrite(SYNC_STATUS, LOW);

  //WiFi.disconnect();
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" connected to %s, IP address: %s\n", ssid, WiFi.localIP().toString().c_str());
  
  hspi_slave_begin();
  MQTTclient.setServer("ds218p", 1883);
  MQTTreconnect();
  //MQTTclient.setCallback(callback);
}

void update_frame_sync(int new_value) {
  if((frame_sync != new_value) & (frame_sync == 0 | new_value == 0)){
    Serial.printf("%lu:sync %i -> %i\n", millis(), frame_sync, new_value);
    frame_sync_changed = 1;
    if(new_value > 0)
      digitalWrite(SYNC_STATUS, HIGH); // synced
    else
      digitalWrite(SYNC_STATUS, LOW);  // not synced
  }
  frame_sync = new_value;
}

void loop() {
  uint fan_old = 99;
  uint power_old = 99;
  uint mode_old = 99;
  uint troom_old = 99;
  uint tsetpoint_old = 99;
  char databyte3_old = 0xff;  // temporary use for evaluation room temperature
  char strtmp[10]; // for the MQTT strings to send
  
  Serial.println("start loop");
  previousMillis = millis();
  sync_isr();  
  while(1){
    if(frame_sync >= 2) {  // valid frame received
        frame_cnt++;
        digitalWrite(LED_BUILTIN, HIGH);
        if(frame_sync == 3) {  // new frame received
          MQTTclient.publish("MHI/raw", rx_SPIframe, 19, true);

          if((rx_SPIframe[DATABYTE0] & 0x01) != power_old) {  // Power
            power_old = rx_SPIframe[DATABYTE0] & 0x01;
            if(power_old == 0)
              MQTTclient.publish("MHI/Power", "off", true);
            else
              MQTTclient.publish("MHI/Power", "on", true);
          }
         
          if(rx_SPIframe[DATABYTE0] & 0x1c != mode_old) {  // Mode
            mode_old = rx_SPIframe[DATABYTE0] & 0x1c;
            switch (mode_old) {
              case 0x00:
                  MQTTclient.publish("MHI/Mode", "Auto", true);
                  break;
              case 0x04:
                  MQTTclient.publish("MHI/Mode", "Dry", true);
                  break;
              case 0x08:
                  MQTTclient.publish("MHI/Mode", "Cool", true);
                  break;
              case 0x0c:
                  MQTTclient.publish("MHI/Mode", "Fan", true);
                  break;
              case 0x10:
                  MQTTclient.publish("MHI/Mode", "Heat", true);
                  break;
              default:
                  MQTTclient.publish("MHI/Mode", "invalid", true);
                  break;
            }
          }

          uint fantmp;
          if((rx_SPIframe[DATABYTE6] & 0x40) > 0) // Fan status
            fantmp = 4;
          else
            fantmp = (rx_SPIframe[DATABYTE1] & 0x03) + 1;
          if(fantmp != fan_old){
            fan_old = fantmp;
            itoa(fan_old, strtmp, 10);
            MQTTclient.publish("MHI/fan", strtmp, true);
          }

          if((rx_SPIframe[DATABYTE3] - 61) / 4 != troom_old) {  // Room temperature
            troom_old = (rx_SPIframe[DATABYTE3] - 61) >> 2;
            itoa(troom_old, strtmp, 10);
            MQTTclient.publish("MHI/Troom", strtmp, true);
          }

          if(rx_SPIframe[DATABYTE3] != databyte3_old) { // temporary use for room temperature evaluation
            String tmp = String(rx_SPIframe[DATABYTE3], HEX);
            MQTTclient.publish("MHI/TroomHex", tmp.c_str(), true);
            databyte3_old = rx_SPIframe[DATABYTE3];
          }
            
          if((rx_SPIframe[DATABYTE2] & 0x7f) >> 1 != tsetpoint_old) {  // Temperature setpoint
            tsetpoint_old = (rx_SPIframe[DATABYTE2] & 0x7f) >> 1;
            itoa(tsetpoint_old, strtmp, 10);
            MQTTclient.publish("MHI/Tsetpoint", strtmp, true);
          }
        }
        update_frame_sync(1);           //synced, armed
        previousMillis = millis();
        sync_isr();        
      }
    else { // armed, frame_sync <= 1
      if (millis() - previousMillis > 53) {  // latest after 50ms a valid frame ws expected
        if(frame_sync == 1) {
          asyncMillis = millis();
          update_frame_sync(0);           //not synced, armed
        }
        else if (millis() - asyncMillis > 10000) {  // 10 seconds after async state -> reset
          ESP.restart();
        }
        previousMillis = millis();
        sync_isr();
      }
    }
    if(frame_sync_changed == 1) {
      if(frame_sync > 0)
        MQTTclient.publish ("MHI/synced", "1", true);
      else
        MQTTclient.publish ("MHI/synced", "0", true);
      frame_sync_changed = 0;
    }

    if (millis() - runtimeMillis > 1000) {
      runtimeMillis+=1000;
      itoa(millis()/1000, strtmp, 10);
      MQTTclient.publish("MHI/runtime", strtmp, true);
    }
    
    if (!MQTTclient.connected())
      MQTTreconnect();
    MQTTclient.loop();
    delay(0);
  }
}
