#include <nds.h>
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

    // Standard HID Tastatur-Report (8 Bytes)
    sHidReport[0] = modifier;
    sHidReport[1] = 0; // Reserviert (immer 0)
    sHidReport[2] = keycode;
    sHidReport[3] = 0;
    sHidReport[4] = 0;
    sHidReport[5] = 0;
    sHidReport[6] = 0;
    sHidReport[7] = 0;

    // Nur senden, wenn das USB-Kabel verbunden und der PC bereit ist
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

    // Card Mutex für TinyUSB/DSPico Hardware-Sperre
    rtos_createMutex(&gCardMutex);

    // FIFO Handler für Keyboard vom ARM9
    fifoSetValue32Handler(FIFO_KEYBOARD, keyboard_fifo_handler, NULL);

    // TinyUSB - NACH irqInit() initialisieren
    tusb_init(); // tusb_init() ist der sicherste Standard-Aufruf für TinyUSB

    // Die USB-Endlosschleife
    while (true)
    {
        // tud_task() muss im Dauerfeuer laufen, um USB-Timeouts 
        // auf der Host-Seite (Mac/PC) zu verhindern!
        tud_task();
    }

    return 0;
}
