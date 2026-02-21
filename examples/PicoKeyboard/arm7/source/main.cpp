#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include "tusb.h"

rtos_mutex_t gCardMutex;

#define FIFO_KEYBOARD  FIFO_USER_01

// HID report: [modifier, reserved, key0..key5]
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

    // Sende HID Report wenn USB bereit
    if (tud_hid_ready())
    {
        tud_hid_report(0, sHidReport, sizeof(sHidReport));
    }
}

int main()
{
    // Standard ARM7 Initialisierung
    irqInit();
    fifoInit();
    installSoundFIFO();
    installSystemFIFO();

    irqEnable(IRQ_VBLANK);

    // Card Mutex initialisieren
    rtos_createMutex(&gCardMutex);

    // FIFO Handler für Keyboard-Reports vom ARM9
    fifoSetValue32Handler(FIFO_KEYBOARD, keyboard_fifo_handler, nullptr);

    // TinyUSB initialisieren
    tud_init(BOARD_TUD_RHPORT);

    // IRQs aktivieren
    rtos_enableIrqs();

    while (true)
    {
        tud_task();
        swiWaitForVBlank();
    }

    return 0;
}
