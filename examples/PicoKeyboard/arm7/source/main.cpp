#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include "tusb.h"

rtos_mutex_t gCardMutex;

#define FIFO_KEYBOARD FIFO_USER_01

static uint8_t sHidReport[8] = {0};

static void keyboard_fifo_handler(u32 value, void* userdata)
{
    (void)userdata;
    uint8_t modifier = (value >> 24) & 0xFF;
    uint8_t keycode  = (value >> 16) & 0xFF;

    sHidReport[0] = modifier;
    sHidReport[1] = 0;
    sHidReport[2] = keycode;
    sHidReport[3] = 0;
    sHidReport[4] = 0;
    sHidReport[5] = 0;
    sHidReport[6] = 0;
    sHidReport[7] = 0;

    if (tud_hid_ready())
    {
        tud_hid_report(0, sHidReport, sizeof(sHidReport));
    }
}

int main()
{
    // Standard libnds ARM7 Init - Reihenfolge ist wichtig!
    irqInit();
    fifoInit();
    installSoundFIFO();
    installSystemFIFO();
    irqEnable(IRQ_VBLANK);

    // Card Mutex für TinyUSB/DSPico
    rtos_createMutex(&gCardMutex);

    // FIFO Handler für Keyboard vom ARM9
    fifoSetValue32Handler(FIFO_KEYBOARD, keyboard_fifo_handler, NULL);

    // TinyUSB - NACH irqInit() initialisieren
    tud_init(BOARD_TUD_RHPORT);

    while (true)
    {
        tud_task();
        swiWaitForVBlank();
    }

    return 0;
}
