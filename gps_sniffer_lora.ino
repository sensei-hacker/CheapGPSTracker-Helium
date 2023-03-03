
#include "LoRaWan_APP.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"

#include "gps_sniffer.h"

#define RXD2 37
#define UPDATE_EVERY_MS 3000
#define LORA_SPREADING_FACTOR 9



gps_sniffer *gsniff;
gps_sniffer::ubx_nav_posllh *pos;
int32_t speed = 0;
// Initialized high in order to send the first position
double distance_moved = 50;
RTC_DATA_ATTR int RTC_OLD_LAT = 0;
RTC_DATA_ATTR int RTC_OLD_LON = 0;
const int MIN_DISTANCE_MOVED = 3;
RTC_DATA_ATTR int RTC_BEEN_STATIONARY = 0;
const int SLEEP_AFTER_STATIONARY = 4;


/* You MUST replace these three with the values for YOUR board, in YOUR Helium account. */
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
RTC_DATA_ATTR uint32_t appTxDutyCycle = 20000;

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

uint8_t confirmedNbTrials = 4;


LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
extern SSD1306Wire display;


static void prepareTxFrame( uint8_t port )
{
  if (pos != NULL) {

    appDataSize = 20; //AppDataSize max value is 64

    appData[3] = pos->latitude >> 24;
    appData[2] = pos->latitude >> 16;
    appData[1] = pos->latitude >>  8;
    appData[0] = pos->latitude;

    appData[7] = pos->longitude >> 24;
    appData[6] = pos->longitude >> 16;
    appData[5] = pos->longitude >>  8;
    appData[4] = pos->longitude;

    appData[11] = pos->altitude_ellipsoid >> 24;
    appData[10] = pos->altitude_ellipsoid >> 16;
    appData[9] = pos->altitude_ellipsoid >> 8;
    appData[8] = pos->altitude_ellipsoid;

    appData[15] = speed >> 24;
    appData[14] = speed >> 16;
    appData[13] = speed >> 8;
    appData[12] = speed;

    int32_t accuracy = pos->horizontal_accuracy / 1000; // UBX reports it in mm.

    appData[19] = accuracy >> 24;
    appData[18] = accuracy >> 16;
    appData[17] = accuracy >>  8;
    appData[16] = accuracy;

  } else {
    appDataSize = 1;
    appData[0] = 0;
  }
}


double distanceBetween(double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795;
}



void setup()
{

  Mcu.begin();
  Serial.begin(115200);
  while (!Serial);
  SPI.begin(SCK,MISO,MOSI,SS);
  deviceState = DEVICE_STATE_INIT;

  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, -1);
  gsniff = new gps_sniffer(&Serial2);
}

void update_location() {

  static unsigned long updatetimer = 0;
  gps_sniffer::ubx_nav_posllh *newpos;

    // if (updatetimer + UPDATE_EVERY_MS < millis()) {
        updatetimer = millis();
        newpos = gsniff->GetLocation(&speed);
        while( (newpos == NULL) && millis() < updatetimer + 100) {
          newpos = gsniff->GetLocation(&speed);
        }


        if (newpos != NULL) {
          if (RTC_OLD_LAT != 0 && RTC_OLD_LON != 0) {
            distance_moved = distanceBetween( (double) newpos->latitude / 10000000, (double) newpos->longitude / 10000000, (double) RTC_OLD_LAT / 10000000,  (double) RTC_OLD_LON / 10000000);
            Serial.printf("distance_moved: %.2f\n", distance_moved);
            if (distance_moved < MIN_DISTANCE_MOVED) {
              if (RTC_BEEN_STATIONARY++ > SLEEP_AFTER_STATIONARY) {
                // Do not double past 6 hours.
                if( appTxDutyCycle < (6 * 60 * 60 * 1000) ) {
                  appTxDutyCycle = appTxDutyCycle * 2;
                  Serial.printf("Been stationary for %i cycles, doubling period to %i seconds\n", RTC_BEEN_STATIONARY - 1, (int) appTxDutyCycle / 1000);
                }
                RTC_BEEN_STATIONARY = 0;
              }
            } else {
              if (appTxDutyCycle > 60000) {
                appTxDutyCycle = 30000;
              }
              RTC_BEEN_STATIONARY = 0;
            }
          }


          int old_lat = RTC_OLD_LAT;
          int old_lon = RTC_OLD_LON;

          pos = newpos;

          printf("long: %.6f, latitude: %.6f aka 0x%08x, h_accuracy: %i, distance: %.2f\n", (float) pos->longitude / 10000000, (float) pos->latitude / 10000000, pos->latitude, pos->horizontal_accuracy / 1000, (float) distance_moved);
          RTC_OLD_LAT = pos->latitude;
          RTC_OLD_LON = pos->longitude;
        }
    // }
  printf("returning from update_location, pos is: %i\n", pos);
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
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
        update_location();
        if (distance_moved >= MIN_DISTANCE_MOVED) {
          LoRaWAN.displaySending();
          prepareTxFrame( appPort );
          LoRaWAN.send();
        }
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
      LoRaWAN.displayAck();
      // if (pos != NULL) {
        // pos = gsniff->GetLocation(&speed);
        LoRaWAN.sleep(loraWanClass);
        // pos = gsniff->GetLocation(&speed);
        // update_location();
      // }
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}

