#include "Arduino.h"
#include "gps_sniffer.h"




gps_sniffer::gps_sniffer(Stream *source) {
  _source = source;
  _step = 0;
  frameread = 0;
}

gps_sniffer::ubx_nav_posllh *gps_sniffer::GetLocation(int32_t *speed = NULL) {
    bool location_updated = false;
    while ( (_source->available() > 0) && frameread == 0) {
      uint8_t byte = _source->read();
      frameread = gpsNewFrameUBLOX(byte);
    }

    if ( frameread > 0 && _msg_id > 0) {
      frameread = 0;
      if (_msg_id == MSG_POSLLH && _payload_length == 28) {
        memcpy ((void *) &pos, (void *) _buffer, _payload_length );
        location_updated = true;
      } else if (_msg_id == MSG_PVT && _payload_length == 92) {
        memcpy ((void *) &nav_pvt, (void *) _buffer, _payload_length );
        pos.longitude = nav_pvt.longitude;
        pos.latitude = nav_pvt.latitude;
        pos.altitude_msl = nav_pvt.altitude_msl;
        pos.time = nav_pvt.time;
        pos.altitude_ellipsoid = nav_pvt.altitude_ellipsoid;
        pos.horizontal_accuracy = nav_pvt.horizontal_accuracy;
        *speed = nav_pvt.speed_2d;
        location_updated = true;
        // Serial.printf("in class, long: %i, lat: %i, alt: %i\, time: %i\n", pos.longitude, pos.latitude, pos.altitude_msl, pos.time);
      } else if (_msg_id == MSG_VELNED) {
        memcpy ( (void *) &nav_velned, (void *) _buffer, sizeof(nav_velned) );
        *speed = nav_pvt.speed_2d;
        location_updated = false;
      }

      if (location_updated && pos.longitude != 0) {
        return &pos;
      }

    }
    return NULL;
}


bool gps_sniffer::gpsNewFrameUBLOX(uint8_t data)
{
    
    bool parsed = false;
    bool _skip_packet = 0;
    switch (_step) {
        case 0: // Sync char 1 (0xB5)
            if (0xb5 == data) {
                _step++;
                // Serial.printf("read 0xb5\n");
            }
            break;
        case 1: // Sync char 2 (0x62)
            if (0x62 != data) {
                _step = 0;
                break;
            }
            // Serial.printf("read 0xb5, 0x62\n");
            _step++;
            break;
        case 2: // Class
            _step++;
            _class = data;
            _ck_b = _ck_a = data;
            break;
        case 3: // Id
            _step++;
            _ck_b += (_ck_a += data);       // checksum byte
            _msg_id = data;
            // Serial.printf("msg_id: %i\n", _msg_id);
            break;
        case 4: // Payload length (part 1)
            _step++;
            _ck_b += (_ck_a += data);       // checksum byte
            _payload_length = data; // payload length low byte
            break;
        case 5: // Payload length (part 2)
            _step++;
            _ck_b += (_ck_a += data);       // checksum byte
            _payload_length |= (uint16_t)(data << 8);
            // prepare to receive payload
            _payload_counter = 0;
            if (_payload_length == 0) {
                _step = 7;
            }
            // Serial.printf("payload_length: %i\n", _payload_length);
            break;
        case 6:
            _ck_b += (_ck_a += data);       // checksum byte
            if (_payload_counter < 256) {
                _buffer[_payload_counter] = data;
            }
            // NOTE: check counter BEFORE increasing so that a payload_size of 65535 is correctly handled.  This can happen if garbage data is received.
            if (_payload_counter ==  _payload_length - 1) {
                _step++;
            }
            _payload_counter++;
            // Serial.printf("_payload_counter: %i\n", _payload_counter);
            break;
        case 7:
            _step++;
            if (_ck_a != data) {
                _skip_packet = true;          // bad checksum
                gpsStats_errors++;
                _step = 0;
                Serial.println("bad checksum in case 7");
            }
            break;
        case 8:
            _step = 0;

            if (_ck_b != data) {
                gpsStats_errors++;
                Serial.println("bad checksum in case 8");
                break;              // bad checksum
            }


            if (_skip_packet) {
                break;
            }
            gpsStats_packetCount++;
            parsed = 1;


    }
    return parsed;
}

