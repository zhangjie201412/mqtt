#include <unistd.h>
#include <signal.h>
#include "ThinMqtt.h"

using namespace iot;

static volatile sig_atomic_t g_interrupt = false;

static void sig_int_handler(int sig)
{
    g_interrupt = true;
}

static void wait_For_sig_int(void)
{
    while(g_interrupt == false) {
        ::usleep(100000);
    }
}

int main(int args, char **argv)
{
    ::signal(SIGINT, sig_int_handler);
    ThinMqtt *mqtt = new ThinMqtt();

    mqtt->setup("127.0.0.1", 1884);

    wait_For_sig_int();
    mqtt->release();
    delete mqtt;

    return 0;
}
