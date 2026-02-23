#include "common.h"
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/sound/soundChannel.h>
#include <libtwl/timer/timer.h>
#include <libtwl/sound/sound.h>
#include <libtwl/sys/sysPower.h>
#include <libtwl/sio/sioRtc.h>
#include <libtwl/sio/sio.h>
#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/mem/memSwap.h>
#include <libtwl/i2c/i2cMcu.h>
#include <libtwl/spi/spiPmic.h>
#include <libtwl/ipc/ipcSync.h>
#include "ExitMode.h"
#include "Arm7State.h"
#include "tusb.h"

rtos_mutex_t gCardMutex;
static rtos_event_t sVBlankEvent;
static ExitMode sExitMode;
static Arm7State sState;
static volatile u8 sMcuIrqFlag = false;

static rtos_thread_t sUsbThread;
static u32 sUsbThreadStack[512];

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

static void usbThreadMain(void* arg) {
    (void)arg;
    volatile u32* shared_key = (volatile u32*)0x02300000; 
    uint8_t sHidReport[8] = {0};

    while (true) {
        tud_task();

        u32 val = *shared_key;
        if (val != 0xFFFFFFFF) {
            sHidReport[0] = (val >> 24) & 0xFF; 
            sHidReport[2] = (val >> 16) & 0xFF; 

            if (tud_hid_ready()) {
                tud_hid_report(0, sHidReport, sizeof(sHidReport));
            }
            *shared_key = 0xFFFFFFFF; 
        }
    }
}

static void initializeArm7() {
    rtos_initIrq();
    rtos_startMainThread();

    rtos_createMutex(&gCardMutex);
    dmaFillWords(0, (void*)0x04000400, 0x100);
    pmic_setAmplifierEnable(true);
    sys_setSoundPower(true);
    pmic_setPowerLedBlink(PMIC_CONTROL_POWER_LED_BLINK_NONE);
    sio_setGpioSiIrq(false);
    sio_setGpioMode(RCNT0_L_MODE_GPIO);
    rtc_init();

    snd_setMasterVolume(127);
    snd_setMasterEnable(true);

    rtos_createEvent(&sVBlankEvent);
    rtos_setIrqFunc(RTOS_IRQ_VBLANK, vblankIrq);
    rtos_enableIrqMask(RTOS_IRQ_VBLANK);
    gfx_setVBlankIrqEnabled(true);

    if (isDSiMode()) {
        rtos_setIrq2Func(RTOS_IRQ2_MCU, mcuIrq);
        rtos_enableIrq2Mask(RTOS_IRQ2_MCU);
    }

    // --- DSPICO HARDWARE HANDSHAKE ---
    // Der ARM7 signalisiert Bereitschaft und wartet auf die Cartridge-Rechte vom ARM9
    ipc_setArm7SyncBits(7);
    while (ipc_getArm9SyncBits() != 6) {
        rtos_waitEvent(&sVBlankEvent, true, true);
    }
    // ---------------------------------

    tusb_rhport_init_t dev_init = { .role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO };
    tusb_init(0, &dev_init);

    volatile u32* shared_key = (volatile u32*)0x02300000;
    *shared_key = 0xFFFFFFFF;

    rtos_createThread(&sUsbThread, 3, usbThreadMain, NULL, sUsbThreadStack, sizeof(sUsbThreadStack));
    rtos_wakeupThread(&sUsbThread);
}

static void updateArm7() {
    checkMcuIrq();
    if (sState == Arm7State::ExitRequested) {
        snd_setMasterVolume(0);
        if (sExitMode == ExitMode::Reset) {
            mcu_setWarmBootFlag(true);
            mcu_hardReset();
        } else {
            pmic_shutdown();
        }
        while (true);
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
