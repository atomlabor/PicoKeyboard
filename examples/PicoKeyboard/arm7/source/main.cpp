#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include "tusb.h"

// Global card mutex (declared extern in common.h)
rtos_mutex_t gCardMutex;

int main()
{
    rtos_createMutex(&gCardMutex);

    tud_init(BOARD_TUD_RHPORT);

    rtos_enableIrqs();

    while (true)
    {
        tud_task();
    }

    return 0;
}
