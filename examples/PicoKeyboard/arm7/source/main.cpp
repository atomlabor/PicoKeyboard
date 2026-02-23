#include "common.h"
#include <nds/disc_io.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/sound/soundChannel.h>
#include <libtwl/timer/timer.h>
#include <libtwl/sound/sound.h>
#include <libtwl/ipc/ipcSync.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include <libtwl/sys/sysPower.h>
#include <libtwl/sio/sioRtc.h>
#include <libtwl/sio/sio.h>
#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/mem/memSwap.h>
#include <libtwl/i2c/i2cMcu.h>
#include <libtwl/spi/spiPmic.h>
#include "ipcServices/DldiIpcService.h"
#include "ExitMode.h"
#include "Arm7State.h"
#include "tusb.h"

static DldiIpcService sDldiIpcService;
rtos_mutex_t gCardMutex;

static rtos_event_t sVBlankEvent;
static ExitMode sExitMode;
static Arm7State sState;
static volatile u8 sMcuIrqFlag = false;

static rtos_thread_t sUsbThread;
static u32 sUsbThreadStack[512];

extern FN_MEDIUM_READSECTORS _DLDI_readSectors_ptr;
extern FN_MEDIUM_WRITESECTORS _DLDI_writeSectors_ptr;

static void vblankIrq(u32 irqMask) {
    (void)irqMask;
    rtos_signalEvent(&sVBlankEvent);
}

static void mcuIrq(u32 irq2Mask) {
    (void)irq2Mask;
    sMcuIrqFlag = true;
}

static void checkMcuIrq(void) {
    if (isDSiMode()) {
        if (mem_swapByte(false, &sMcuIrqFlag)) {
            u32 irqMask = mcu_getIrqMask();
            if (irqMask & MCU_IRQ_RESET) {
                sExitMode = ExitMode::Reset;
                sState = Arm7State::ExitRequested;
            } else if (irqMask & MCU_IRQ_POWER_OFF) {
                sExitMode = ExitMode::PowerOff;
                sState = Arm7State::ExitRequested;
            }
        }
    }
}

static void initializeVBlankIrq() {
    rtos_createEvent(&sVBlankEvent);
    rtos_setIrqFunc(RTOS_IRQ_VBLANK, vblankIrq);
    rtos_enableIrqMask(RTOS_IRQ_VBLANK);
    gfx_setVBlankIrqEnabled(true);
}

// Hier ist der USB-Thread mit dem Shared-RAM Briefkasten
static void usbThreadMain(void* arg) {
    (void)arg;
    // Feste Speicheradresse, die sich ARM7 und ARM9 teilen
    volatile u32* shared_key = (volatile u32*)0x023FFFE0;
    uint8_t sHidReport[8] = {0};

    while (true) {
        tud_task(); // USB am Leben halten

        u32 val = *shared_key;
        if (val != 0xFFFFFFFF) { // Wenn eine neue Taste im Briefkasten liegt
            sHidReport[0] = (val >> 24) & 0xFF; // Modifier
            sHidReport[2] = (val >> 16) & 0xFF; // Keycode

            if (tud_hid_ready()) {
                tud_hid_report(0, sHidReport, sizeof(sHidReport));
            }
            *shared_key = 0xFFFFFFFF; // Briefkasten wieder leeren
        }
    }
}

static void initializeArm7() {
    rtos_initIrq();
    rtos_startMainThread();
    ipc_initFifoSystem();
    rtos_createMutex(&gCardMutex);

    dmaFillWords(0, (void*)0x04000400, 0x100);
    pmic_setAmplifierEnable(true);
    sys_setSoundPower(true);
    readUserSettings();
    pmic_setPowerLedBlink(PMIC_CONTROL_POWER_LED_BLINK_NONE);
    sio_setGpioSiIrq(false);
    sio_setGpioMode(RCNT0_L_MODE_GPIO);
    rtc_init();
    sDldiIpcService.Start();
    snd_setMasterVolume(127);
    snd_setMasterEnable(true);
    initializeVBlankIrq();

    if (isDSiMode()) {
        rtos_setIrq2Func(RTOS_IRQ2_MCU, mcuIrq);
        rtos_enableIrq2Mask(RTOS_IRQ2_MCU);
    }

    ipc_setArm7SyncBits(7);
    while (ipc_getArm9SyncBits() != 6) {
        rtos_waitEvent(&sVBlankEvent, true, true);
    }

    tusb_rhport_init_t dev_init = { .role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO };
    tusb_init(0, &dev_init);

    // Briefkasten beim Start leeren
    volatile u32* shared_key = (volatile u32*)0x023FFFE0;
    *shared_key = 0xFFFFFFFF;

    rtos_createThread(&sUsbThread, 3, usbThreadMain, NULL, sUsbThreadStack, sizeof(sUsbThreadStack));
    rtos_wakeupThread(&sUsbThread);
}

static void updateArm7IdleState() {
    checkMcuIrq();
    if (sState == Arm7State::ExitRequested) snd_setMasterVolume(0);
}

static bool performExit(ExitMode exitMode) {
    switch (exitMode) {
        case ExitMode::Reset:
            mcu_setWarmBootFlag(true);
            mcu_hardReset();
            break;
        case ExitMode::PowerOff:
            pmic_shutdown();
            break;
    }
    while (true);
}

static void updateArm7ExitRequestedState() { performExit(sExitMode); }

static void updateArm7() {
    switch (sState) {
        case Arm7State::Idle: updateArm7IdleState(); break;
        case Arm7State::ExitRequested: updateArm7ExitRequestedState(); break;
    }
}

int main() {
    sState = Arm7State::Idle;
    initializeArm7();
    while (true) {
        rtos_waitEvent(&sVBlankEvent, true, true);
        updateArm7();
    }
    return 0;
}
