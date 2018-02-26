#include "ThinMqtt.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "common.h"

//#define LOCAL_IP        "192.168.3.95"
////#define LOCAL_PORT      8989
//
#define MAXEVENTS       4096
#define MAX_LISTENER 2048

namespace iot {

ThinMqtt::ThinMqtt()
    :mPort(-1)
{
    mClients.clear();
    ::memset(mHost, 0x00, 64);
}

ThinMqtt::~ThinMqtt()
{
    //do something cleaner
}

bool ThinMqtt::setup(const char *host, int port)
{
    mPort = port;
    ::strcpy(mHost, host);
    LOGD("%s: setup %s:%d\n", __func__, mHost, mPort);
    int rc = ::pthread_create(&mMainThread, NULL, ThinMqtt::mainThread, this);
    if(rc < 0) {
        LOGE("%s: failed to create main thread\n", __func__);
        return false;
    }
    return true;
}

int ThinMqtt::makeNoneBlock(int fd)
{
    int flags, ret;

    flags = ::fcntl(fd, F_GETFL, 0);
    if(flags == -1) {
        LOGE("%s: failed to fcntl\n", __func__);
        return -1;
    }

    flags |= O_NONBLOCK;
    ret = ::fcntl(fd, F_SETFL, flags);
    if(flags == -1) {
        LOGE("%s: failed to fcntl\n", __func__);
        return -1;
    }

    return 0;

}


ThinMQTTClient *ThinMqtt::getClient(int fd)
{
    int idx = -1;

    for(int i = 0; i < mClients.size(); i++) {
        if(mClients[i]) {
            if(fd == mClients[i]->fd) {
                idx = i;
            }
        }
    }
    if(idx == -1)
        return NULL;
    return mClients[idx];
}


int ThinMqtt::readPackage(ThinMQTTClient *client)
{
    int rc = -1;

    //process the comming data

    return rc;
}

void *ThinMqtt::clientThread(void *ptr)
{
    ThinMQTTClient *client = (ThinMQTTClient *)ptr;
    int fd = client->fd;
    int rc;

    do {
        if(false == client->available) {
            pthread_cond_wait(&client->cond_v, &client->mutex);
            client->available = false;
        }

        char data;
        rc = ::read(client->fd, &data, 1);
        if(rc != 1) {
            LOGE("%s: failed to read head\n", __func__);
            break;
        }
        LOGD("%s: get head %02x\n", __func__, data);
        ThinMQTTHeader header  = {0};
        header.byte = data;
        uint8_t type = header.bits.type;
        LOGD("%s: type = %02x\n", __func__, type);

        switch(type) {
            case 0:
                LOGE("%s: invalid type\n", __func__);
                break;
            case THIN_CONNECT:
                data = THIN_CONNACK << 4 | 0x00;
                ::write(client->fd, &data, 1);
                client->status |= STATUS_CONNECTED;
                break;
            case THIN_PUBLISH:
                data = THIN_PUBACK << 4 | 0x00;
                ::write(client->fd, &data, 1);
                break;
            case THIN_PUBCOMP:
                data = THIN_PUBACK << 4 | 0x00;
                ::write(client->fd, &data, 1);
                break;
            case THIN_DISCONNECT:
                data = THIN_DISCONNACK << 4 | 0x00;
                ::write(client->fd, &data, 1);
                client->status |= STATUS_DISCONNECT;
                break;
            default:
                break;
        }
#if 0
        int bytes = ::read(client->fd, recv, MAX_SIZE);
        if(bytes < 0) {
            LOGE("%s: failed to read bytes: %s\n", __func__, ::strerror(errno));
            continue;
        } else if(bytes == 0) {
            LOGD("%s: maybe remote client closed\n", __func__);
            // destory the client
            client->status |= STATUS_DESTORY;
            LOGD("%s: status = %x\n", __func__, client->status);
        } else {
            LOGD("<<--- client %d recv %d bytes: %s\n", client->fd, bytes, recv);
        }
#endif
    } while((client != NULL) 
        && ((client->status & STATUS_DESTORY) == 0));
    LOGD("%s: maybe remote client closed\n", __func__);
    // destory the client
    client->status |= STATUS_DESTORY;
    LOGD("%s: status = %x\n", __func__, client->status);
    ::close(fd);
    LOGD("%s: client %d exit!!!!\n", __func__, fd);

    return NULL;
}

void *ThinMqtt::mainThread(void *ptr)
{
    int sock_id;
    int efd;
    struct epoll_event event;
    struct epoll_event *events;
    ThinMqtt *self = (ThinMqtt *)ptr;
    const int MAX_SIZE = 1024;
    uint8_t recv[MAX_SIZE];

    int rc;
    sock_id = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sock_id < 0) {
        LOGE("%s: failed to create socket\n", __func__);
        return NULL;
    }

    struct sockaddr_in local_addr;
    ::memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(self->mHost);
    local_addr.sin_port = htons(self->mPort);

    rc = ::bind(sock_id, (struct sockaddr *)&local_addr,
            sizeof(struct sockaddr));
    if(rc < 0) {
        LOGE("%s: bind %s:%d failed: %s\n", __func__,
                self->mHost, self->mPort, ::strerror(errno));
        ::close(sock_id);
        return NULL;

    }
    if(self->makeNoneBlock(sock_id) < 0) {
        LOGE("%s: failed make none block\n", __func__);
        ::close(sock_id);
        return NULL;

    }
    rc = ::listen(sock_id, MAX_LISTENER);
    if(rc < 0) {
        LOGE("%s: listen %s:%d failed\n", __func__,
                self->mHost, self->mPort);
        ::close(sock_id);
        return NULL;

    }

    efd = ::epoll_create(MAX_LISTENER);
    if(efd < 0) {
        LOGE("%s: failed to create epoll: %s\n",
                __func__, ::strerror(errno));
        ::close(sock_id);
        return NULL;

    }

    event.data.fd = sock_id;
    event.events = EPOLLIN | EPOLLET;
    rc = ::epoll_ctl(efd, EPOLL_CTL_ADD, sock_id, &event);
    if(rc < 0) {
        LOGE("%s:%d failed to epoll ctl %d: %s\n",
                __func__, __LINE__, efd, ::strerror(errno));
        ::close(sock_id);
        return NULL;

    }

    events = (struct epoll_event *)::calloc(MAXEVENTS, sizeof(struct epoll_event));
    while(true) {
        int event_num, i;

        event_num = ::epoll_wait(efd, events, MAXEVENTS, -1);
        for(i = 0; i < event_num; i++) {
            LOGD("Check client list\n");
            //check the clients list
            int size = self->mClients.size();
            for(int j = 0; j < size; j++) {
                if((self->mClients[j]->status & STATUS_DESTORY) != 0) {
                    LOGD("%s: remove client!\n", __func__);
                    ::free(self->mClients[j]);
                    self->mClients[j] = NULL;
                    self->mClients.erase(self->mClients.begin() + j);
                }
            }

            if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                LOGE("%s: connection closed\n", __func__);
                ::close(events[i].data.fd);
                int idx = -1;
                for(int j = 0; j < self->mClients.size(); i++) {
                    if(events[i].data.fd == self->mClients[i]->fd) {
                        idx = i;
                        break;
                    }
                }
                if(idx >= 0) {
                    LOGD("%s: remove client %d\n", __func__, idx);
                    self->mClients.erase(self->mClients.begin() + idx);
                }

                continue;

            } else if(sock_id == events[i].data.fd) {
                //connection comming
                struct sockaddr_in remote_addr;
                int sin_size = sizeof(struct sockaddr_in);
                LOGD("%s: wait for connected...\n", __func__);
                int fd = ::accept(sock_id, (struct sockaddr *)&remote_addr,
                        (socklen_t *)&sin_size);
                if(fd < 0) {
                    LOGE("%s: failed to do accept\n", __func__);
                    continue;

                }
                LOGD("%s: connection done\n", __func__);
                if(self->makeNoneBlock(fd) < 0) {
                    LOGE("%s: failed make none block\n", __func__);
                    continue;

                }
                event.data.fd = fd;
                event.events = EPOLLIN | EPOLLET;
                rc = ::epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
                if(rc < 0) {
                    LOGE("%s:%d failed to epoll ctl\n", __func__, __LINE__);
                    continue;

                }
                ThinMQTTClient *client = (ThinMQTTClient *)malloc(sizeof(ThinMQTTClient));
                if(!client) {
                    LOGE("%s: failed to malloc for client", __func__);
                    return NULL;
                }
                ::memset(client, 0x00, sizeof(ThinMQTTClient));
                client->fd = fd;
                client->status = 0;
                client->available = false;

                pthread_mutex_init(&client->mutex, NULL);
                pthread_cond_init(&client->cond_v, NULL);
                //launch the thread
                pthread_mutex_lock(&client->mutex);
                rc = pthread_create(&client->pid, NULL, ThinMqtt::clientThread, (void *)client);
                if(rc < 0) {
                    LOGE("%s: failed to create client thread\n", __func__);
                    pthread_mutex_unlock(&client->mutex);
                    return NULL;
                }
                pthread_mutex_unlock(&client->mutex);
                self->mClients.push_back(client);
                LOGD("%s: add socket fd: %d, total %ld clients\n",
                        __func__, fd, self->mClients.size());
                //TODO: Need to check clients ??
            } else {
                //data available
                LOGD("%s: data available\n", __func__);
                int fd = events[i].data.fd;
                //find the client
                ThinMQTTClient *client = self->getClient(fd);
                if(client) {
                    //self->readPackage(client);
                    //notify the client thread
                    pthread_mutex_lock(&client->mutex);
                    client->available = true;
                    LOGD("%s: notify %d\n", __func__, fd);
                    pthread_cond_signal(&client->cond_v);
                    pthread_mutex_unlock(&client->mutex);
                } else {
                    LOGE("%s: cannot get client by fd = %d\n", __func__, fd);
                }
            }
        }
    }

    return NULL;
}

}
