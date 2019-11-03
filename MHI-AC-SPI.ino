// MHI-AC-SPI v1.1 by absalom-muc
// read data via SPI and send via MQTT

#include "esp8266_peri.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // see https://github.com/knolleary/pubsubclient

const char* ssid = "xxx";
const char* password = "xxx";

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

#define SYNC_PIN 4
volatile bool valid_datapacket_received = false;
volatile bool new_datapacket_received = false;
unsigned long last_sync_isrT = 0;
volatile bool sync = false;
volatile bool sync_changed = true;

unsigned long runtimeMillis = 0;
uint8_t rx_payload[19];

#define SIGNBYTE1 0
#define SIGNBYTE2 1
#define DATABYTE0 SIGNBYTE2 + 1
#define DATABYTE1 SIGNBYTE2 + 2
#define DATABYTE2 SIGNBYTE2 + 3
#define DATABYTE3 SIGNBYTE2 + 4
#define DATABYTE6 SIGNBYTE2 + 7

void MQTTreconnect() {
  while (!MQTTclient.connected()) { // Loop until we're reconnected
    SPI1S |= SPISSRES;   //reset SPI
    update_sync(0);
    Serial.print("Attempting MQTT connection...");
    if (MQTTclient.connect ("MHI-AC-SPI", "MHI/connected", 0, true, "0")) {
      Serial.println("connected");
      MQTTclient.publish("MHI/connected", "1", true);
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
          uint8_t *p = rx_payload;
          uint32_t dword;
          new_datapacket_received = 0;
          uint16_t sum = 0x6c;
          for(int i=0; i<19;i++) {
            if (i%4 == 0)
              dword = SPI1W(i/4);
            if(*p != (uint8_t)dword)
              new_datapacket_received = 1;
            *p = (uint8_t)dword;
            if(i<17)
              sum += *p;
            p++;
            dword = dword>>8;
          }
          if((rx_payload[0] != 0x80 | rx_payload[1] != 0x04) | (rx_payload[17] != highByte(sum) | rx_payload[18] != lowByte(sum)))
            digitalWrite(SYNC_PIN, 0);  // for debug only
          else
            valid_datapacket_received = true;
          digitalWrite(LED_BUILTIN, LOW);  // for debug only
        }
    } else if(istatus & (1 << SPII0)) { //SPI0 ISR
        SPI0S &= ~(0x3ff);//clear SPI ISR
    } else if(istatus & (1 << SPII2)) {} //I2S ISR
}

void update_sync(bool sync_new) {
  if(sync_new != sync) {
    sync = sync_new;
    sync_changed = true;
    digitalWrite(SYNC_PIN, sync_new);
  }
}

void hspi_slave_begin() {
    pinMode(SCK, SPECIAL);                  // Both inputs in slave mode
    pinMode(MOSI, SPECIAL);
    // Take care, the register descriptions might be wrong! Couldn't find a ESP8266 SPI register description
    SPI1C = 0x0628A000;                     // SPI_CTRL_REG LSB first, single bit rx_payload mode.
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
    ETS_SPI_INTR_ATTACH(_hspi_slave_isr_handler, 0);
    ETS_SPI_INTR_ENABLE();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.printf("%lu:MHI-AC_SPI starting\n", millis());
  pinMode(LED_BUILTIN, OUTPUT);  // indicates that a frame was received, active low
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(SYNC_PIN, OUTPUT);
  digitalWrite(SYNC_PIN, LOW);

  WiFi.hostname("MHI-AC-SPI");
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
}

void loop() {
  uint fan_old = 99;
  uint power_old = 99;
  uint mode_old = 99;
  uint databyte3_old = 99;
  int troom_old = 99;
  uint tsetpoint_old = 99;
  char strtmp[10]; // for the MQTT strings to send
  
  sync_isr();  
  while(1){
    if(valid_datapacket_received) {  // valid frame received
      valid_datapacket_received = false;
      last_sync_isrT = millis();
      sync_isr();        
      update_sync(true);
      digitalWrite(LED_BUILTIN, HIGH);
      if(new_datapacket_received) {  // new frame received
        new_datapacket_received = false;
        MQTTclient.publish("MHI/raw", rx_payload, 19, true);

        if((rx_payload[DATABYTE0] & 0x01) != power_old) {  // Power
          power_old = rx_payload[DATABYTE0] & 0x01;
          if(power_old == 0)
            MQTTclient.publish("MHI/Power", "off", true);
          else
            MQTTclient.publish("MHI/Power", "on", true);
        }
       
        if((rx_payload[DATABYTE0] & 0x1c) != mode_old) {  // Mode
          mode_old = rx_payload[DATABYTE0] & 0x1c;
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
        if((rx_payload[DATABYTE6] & 0x40) > 0) // Fan status
          fantmp = 4;
        else
          fantmp = (rx_payload[DATABYTE1] & 0x03) + 1;
        if(fantmp != fan_old){
          fan_old = fantmp;
          itoa(fan_old, strtmp, 10);
          MQTTclient.publish("MHI/fan", strtmp, true);
        }
          
        if(abs(rx_payload[DATABYTE3] - databyte3_old) > 1) {  // Room temperature delta > 0.25°C
          int troom = (rx_payload[DATABYTE3] - 61) / 4;
          if((rx_payload[DATABYTE3] - 61) % 4 >= 2)
            troom += 1;
          databyte3_old = rx_payload[DATABYTE3];
          if(troom != troom_old) {
            itoa(troom, strtmp, 10);
            MQTTclient.publish("MHI/Troom", strtmp, true);
            troom_old = troom;
          }
        }

        if((rx_payload[DATABYTE2] & 0x7f) >> 1 != tsetpoint_old) {  // Temperature setpoint
          tsetpoint_old = (rx_payload[DATABYTE2] & 0x7f) >> 1;
          itoa(tsetpoint_old, strtmp, 10);
          MQTTclient.publish("MHI/Tsetpoint", strtmp, true);
        }
      }
    }

    if(millis() - last_sync_isrT > 53) {
      last_sync_isrT = millis();
      sync_isr();        
      update_sync(false);
    }
    
    if(sync_changed) {
      if(sync)
        MQTTclient.publish ("MHI/synced", "1", true);
      else
        MQTTclient.publish ("MHI/synced", "0", true);
      sync_changed = 0;
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
