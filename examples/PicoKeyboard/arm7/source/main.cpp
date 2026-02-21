#include <nds.h>
#include <string.h>
#include "common.h"
#include "ExitMode.h"
#include "libtwl/rtos/rtosMutex.h"
#include "libtwl/rtos/rtosThread.h"
#include "tusb.h"

// Hardware-Sperre, die der dcd_dspico Treiber zwingend verlangt
rtos_mutex_t gCardMutex;

// Speicher und Thread für den USB-Hintergrundprozess
u8 gUsbThreadStack[4096] __attribute__((aligned(8)));
rtos_thread_t gUsbThread;

// Der USB-Hintergrund-Task für den ARM7
void usbThreadMain(void* arg) {
    (void)arg; // Verhindert Compiler-Warnungen für ungenutzte Variablen
    while (1) {
        rtos_lockMutex(&gCardMutex);
        tud_task();
        rtos_unlockMutex(&gCardMutex);
        rtos_delayThread(1);
    }
}

void vblankHandler() {}

int main() {
    // System-Interrupts initialisieren
    irqInit();
    irqSet(IRQ_VBLANK, vblankHandler);
    irqEnable(IRQ_VBLANK);

    // Hardware-Sperre aktivieren
    rtos_initMutex(&gCardMutex);
    
    // TinyUSB auf dem ARM7 hochfahren
    tusb_init();

    // USB-Thread starten, damit die Hardware im Hintergrund läuft
    rtos_createThread(&gUsbThread, 0x30, usbThreadMain, NULL, gUsbThreadStack, sizeof(gUsbThreadStack));
    rtos_wakeupThread(&gUsbThread);

    // Endlosschleife hält den ARM7 am Leben
    while (1) {
        swiWaitForVBlank();
    }
    
    return 0;
}
