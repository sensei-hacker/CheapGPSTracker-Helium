
#include "LoRaWan_APP.h"
#include <Wire.h>               
#include "HT_SSD1306Wire.h"

#include <Preferences.h>


uint8_t devEui[] = { 0x60, 0x81, 0xF9, 0x9A, 0xBF, 0x1C, 0xF8, 0x43 };
uint8_t appEui[] = { 0x60, 0x81, 0xF9, 0xF9, 0x9D, 0x37, 0x56, 0x18 };
uint8_t appKey[] = { 0xDE, 0xD5, 0x9B, 0xD7, 0x33, 0x7B, 0x36, 0x32, 0x0A, 0xB4, 0x78, 0x21, 0x95, 0xD5, 0x96, 0x80 };

/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 300000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = false;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = false;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };


/* Application port */
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 8;


LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
extern SSD1306Wire display;

static void prepareTxFrame( uint8_t port )
{
    appDataSize = 4;//AppDataSize max value is 64
    appData[0] = 0x00;
    appData[1] = 0x01;
    appData[2] = 0x02;
    appData[3] = 0x03;
}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup()
{

  Mcu.begin();
  Serial.begin(115200);
  while (!Serial);
  SPI.begin(SCK,MISO,MOSI,SS);
  deviceState = DEVICE_STATE_INIT;
}

void loop()
{
  
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      // Ray TODO check this display stuff
      VextON();
      delay(100);
      LoRaWAN.displayJoining();
      /*
      display.init();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(10, 128, "Joining");
      display.display();
      */
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      LoRaWAN.displaySending();
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      // LoRaWAN.displayAck();
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
  

}
