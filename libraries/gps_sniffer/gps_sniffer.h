#ifndef gps_sniffer_h
#define gps_sniffer_h
#include "Arduino.h" 


class gps_sniffer {

  #define MSG_NMEA_RMC 0x4
  #define MSG_NMEA_VGS 0x5
  #define MSG_POSLLH 0x2
  #define MSG_STATUS 0x3
  #define MSG_SOL 0x6
  #define MSG_PVT 0x7
  #define MSG_VELNED 0x12

  public:
    typedef struct {
        uint32_t time;              // GPS msToW
        int32_t longitude;
        int32_t latitude;
        int32_t altitude_ellipsoid;
        int32_t altitude_msl;
        uint32_t horizontal_accuracy;
        uint32_t vertical_accuracy;
    } ubx_nav_posllh;

    typedef struct {
        uint32_t time; // GPS msToW
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
        uint8_t valid;
        uint32_t tAcc;
        int32_t nano;
        uint8_t fix_type;
        uint8_t fix_status;
        uint8_t reserved1;
        uint8_t satellites;
        int32_t longitude;
        int32_t latitude;
        int32_t altitude_ellipsoid;
        int32_t altitude_msl;
        uint32_t horizontal_accuracy;
        uint32_t vertical_accuracy;
        int32_t ned_north;
        int32_t ned_east;
        int32_t ned_down;
        int32_t speed_2d;
        int32_t heading_2d;
        uint32_t speed_accuracy;
        uint32_t heading_accuracy;
        uint16_t position_DOP;
        uint16_t reserved2;
        uint16_t reserved3;
    } ubx_nav_pvt;

    typedef struct {
        uint32_t time;              // GPS msToW
        int32_t ned_north;
        int32_t ned_east;
        int32_t ned_down;
        uint32_t speed_3d;
        uint32_t speed_2d;
        int32_t heading_2d;
        uint32_t speed_accuracy;
        uint32_t heading_accuracy;
    } ubx_nav_velned;

    ubx_nav_posllh pos;
    gps_sniffer(Stream *source);
    ubx_nav_posllh *GetLocation(int32_t *speed);

  private:
    float lat, lon, acc;
    uint8_t _step;
    uint8_t _msg_id;
    uint16_t gpsStats_errors;
    uint16_t _payload_length;
    uint8_t _class;
    uint8_t _ck_a;
    uint8_t _ck_b;
    uint8_t _buffer[256];
    uint16_t gpsStats_packetCount;
    uint8_t fix;
    int frameread;
    uint8_t _payload_counter;
    #define UART_TIMEOUT  60000
    Stream *_source;
    ubx_nav_pvt nav_pvt;
    ubx_nav_velned nav_velned;
    bool gpsNewFrameUBLOX(uint8_t data);
    

};



#endif
