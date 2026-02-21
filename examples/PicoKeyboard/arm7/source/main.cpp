#include "common.h"
#include <libtwl/card/card.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include "tusb.h"
#include "DldiIpcService.h"
#include "Arm7State.h"
#include "ExitMode.h"

// Global card mutex (declared extern in common.h)
rtos_mutex_t gCardMutex;

static DldiIpcService sDldiIpcService;

int main()
{
    // Initialize the card mutex
    rtos_createMutex(&gCardMutex);

    // Initialize card hardware
    card_init();

    // Initialize TinyUSB device stack
    tud_init(BOARD_TUD_RHPORT);

    // Start IPC service threads
    sDldiIpcService.Start();

    // Enable interrupts
    rtos_enableIrqs();

    // Main loop: keep the USB device stack running
    while (true)
    {
        tud_task();
    }

    return 0;
}
