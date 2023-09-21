#ifndef _DEF_H_
#define _DEF_H_

// #define DEBUG_PRINT_MAC_ADDRESS

////////// Task Interval Config /////////////////////////
#define CONTROL_INTERVAL_MS 20
#define ESPNOW_INTERVAL_MS 50

////////// ESPNOW /////////////////////////
#define ESPNOW_MAX_RETRY 3
#define ESPNOW_MSG_LENGTH 8

#define ESPNOW_HEADER_BOOT          0x42    //'B'
#define ESPNOW_HEADER_MAC           0x4D    //'M'
#define ESPNOW_HEADER_STATUS        0x55    //'U'

#define ESPNOW_HEADER_RESET         0x62    //'b'
#define ESPNOW_HEADER_MAC_QUERY     0x6D    //'m'
#define ESPNOW_HEADER_RUN           0x72    // r
#define ESPNOW_HEADER_STATUS_QUERY  0x75    //'u'

#define BRIDGE_MAC_ADDRESS {0x48, 0xE7, 0x29, 0x24, 0x81, 0x29}

#define DEVICE_MAC_ADDRESS_LIST {{0x50, 0x02, 0x91, 0x9F, 0xCF, 0x9C}}

#define DEVICE_COUNT    1
#define DEV_NO_BROADCAST 0
#define MESAGE_DUPLICATION_REJECT_TIME_ms 100

#endif // _DEF_H_