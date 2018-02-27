#ifndef __COMMON_H__
#define __COMMON_H__

#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef USE_SYSLOG
#include <syslog.h>
#define LOGE(format, ...)       syslog(LOG_ERR, format, ## __VA_ARGS__)
#define LOGI(format, ...)       syslog(LOG_INFO, format, ## __VA_ARGS__)
#define LOGD(format, ...)       syslog(LOG_DEBUG, format, ## __VA_ARGS__)
#else
#include <stdio.h>
#define LOGE(format, ...)       printf(format, ## __VA_ARGS__)
#define LOGI(format, ...)       printf(format, ## __VA_ARGS__)
#define LOGD(format, ...)       printf(format, ## __VA_ARGS__)
#endif

#define THREAD_NAME_SIZE        16
#define CLIENT_BUFFER_SIZE      1024 
enum ThinMsgType {
    THIN_CONNECT = 1,
    THIN_CONNACK,
    THIN_PUBLISH,
    THIN_PUBACK,
    THIN_PUBCOMP,
    THIN_SUBSCRIBE,
    THIN_SUBACK,
    THIN_UNSUBSCRIBE,
    THIN_UNSUBACK,
    THIN_PINGREQ,
    THIN_PINGRESP,
    THIN_DISCONNECT,
    THIN_DISCONNACK,
};

#define STATUS_CONNECTED        1 << 0
#define STATUS_SUBSCRIBED       1 << 1
#define STATUS_DISCONNECT       1 << 2
#define STATUS_DESTORY          1 << 3

typedef union {
    unsigned char byte;
    struct {
        unsigned int flag : 4;
        unsigned int type : 4;
    } bits;
} ThinMQTTHeader;

typedef struct {
    ThinMQTTHeader header;
    unsigned int length;
    char data[0];
} ThinMQTTBody;

typedef struct {
    int fd;
    unsigned int status;
    bool available;
    char *id;
    char *topic;
    pthread_t pid;
    pthread_mutex_t mutex;
    pthread_cond_t cond_v;
    char threadName[THREAD_NAME_SIZE];
    uint8_t buffer[CLIENT_BUFFER_SIZE];
    uint32_t offset;
} ThinMQTTClient;

typedef struct {
    int sync_id;
    char client_id[32];
    int length;
    uint8_t data[0];
} ThinMQTTMessage;

#endif
