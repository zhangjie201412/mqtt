#include <unistd.h>
#include "ThinMqtt.h"

using namespace iot;

int main(int args, char **argv)
{
    ThinMqtt *mqtt = new ThinMqtt();

    mqtt->setup("127.0.0.1", 1884);

    while(true) {
        ::usleep(1000000);
    }
    delete mqtt;

    return 0;
}
