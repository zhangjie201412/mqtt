#ifndef __THIN_MQTT_CLIENT_H__
#define __THIN_MQTT_CLIENT_H__

#include <pthread.h>
#include <vector>

#include "common.h"

namespace iot {

class ThinMqtt {
public:
    ThinMqtt();
    virtual ~ThinMqtt();

    static void *mainThread(void *ptr);
    static void *clientThread(void *ptr);
    bool setup(const char *addr, int port);
private:
    int makeNoneBlock(int fd);
    int readPackage(ThinMQTTClient *client);
    ThinMQTTClient *getClient(int fd);

private:
    pthread_t mMainThread;
    std::vector<ThinMQTTClient *> mClients;
    int mPort;
    char mHost[64];
};
};

#endif
