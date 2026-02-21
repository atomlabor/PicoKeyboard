#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include "tusb.h"

rtos_mutex_t gCardMutex;

// FIFO-Kanal muss identisch mit ARM9 sein
#define FIFO_KEYBOARD  FIFO_USER_01

// Wird vom ARM9 aufgerufen wenn ein Keyboard-Report ankommt
static void keyboard_fifo_handler(u32 value, void* userdata)
{
    (void)userdata;

    uint8_t modifier = (value >> 24) & 0xFF;
    uint8_t keycode  = (value >> 16) & 0xFF;

    if (tud_hid_ready())
    {
        uint8_t keycodes[6] = { keycode, 0, 0, 0, 0, 0 };
        tud_hid_keyboard_report(0, modifier, keycodes);
    }
}

int main()
{
    rtos_createMutex(&gCardMutex);

    // FIFO-Handler registrieren bevor IRQs aktiviert werden
    fifoSetValue32Handler(FIFO_KEYBOARD, keyboard_fifo_handler, NULL);

    tud_init(BOARD_TUD_RHPORT);

    rtos_enableIrqs();

    while (true)
    {
        tud_task();
    }

    return 0;
}
