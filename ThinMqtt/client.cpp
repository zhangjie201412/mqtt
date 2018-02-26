#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include "common.h"

#define CLIENT_IP       "127.0.0.1"
#define CLIENT_PORT     1884

void thin_mqtt_connect(int fd)
{
    uint8_t u_data = THIN_CONNECT << 4 | 0x00;
    ::write(fd, &u_data, 1);
    LOGD("write head: %02x\n", u_data);
    int rc = ::read(fd, &u_data, 1);
    if(rc != 1) {
        LOGE("Failed to get connack\n");
        return;
    }
    if(u_data & 0xf0 == THIN_CONNACK << 4) {
        LOGD("Get connack!\n");
    }
    return;
}

void thin_mqtt_publish(int fd, char *topic, char *message)
{
    uint8_t u_data = THIN_PUBLISH << 4 | 0x00;
    ::write(fd, &u_data, 1);
    LOGD("write head: %02x\n", u_data);
    int rc = ::read(fd, &u_data, 1);
    if(rc != 1) {
        LOGE("Failed to get connack\n");
        return;
    }
    if(u_data & 0xf0 == THIN_PUBACK << 4) {
        LOGD("Get puback!\n");
    }
    //write topic and message
    return;
}

int main(void)
{
    int sockfd;
    int len;
    struct sockaddr_in addr;
    int rc;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    addr.sin_port = htons(CLIENT_PORT);

    sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        LOGE("Failed to create socket fd\n");
        return -1;
    }

    rc = ::connect(sockfd, (struct sockaddr *)&addr,
            sizeof(struct sockaddr));
    if(rc < 0) {
        LOGE("Failed to connect\n");
        ::close(sockfd);
        return -1;
    }
#if 0
    rc = ::write(sockfd, "Hello\n", 6);
    if(rc < 0) {
        LOGE("Failed to write to server\n");
        ::close(sockfd);
        return -1;
    }
#endif
    thin_mqtt_connect(sockfd);

    usleep(1000000);
    ::close(sockfd);

    return 0;
}
