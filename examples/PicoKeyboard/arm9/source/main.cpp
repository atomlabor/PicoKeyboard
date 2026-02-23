#include <nds.h>
#include <stdio.h>
#include "hid_keycodes.h"

#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/mem/memExtern.h>
#include <libtwl/rtos/rtosIrq.h>
#include <libtwl/rtos/rtosThread.h>
#include <libtwl/rtos/rtosEvent.h>
#include <libtwl/ipc/ipcSync.h>
#include <libtwl/ipc/ipcFifoSystem.h>
#include "dldiIpc.h"

#define SHARED_KEY_ADDR 0x02300000

static rtos_event_t sVblankEvent;

static void vblankIrq(u32 irqMask) {
    (void)irqMask;
    rtos_signalEvent(&sVblankEvent);
}

static inline u32 make_hid_message(uint8_t modifier, uint8_t keycode) {
    return ((u32)modifier << 24) | ((u32)keycode << 16);
}

static uint8_t ascii_to_hid(int c, uint8_t *modifier) {
    *modifier = 0;
    if (c >= 'a' && c <= 'z') return HID_KEY_A + (c - 'a');
    if (c >= 'A' && c <= 'Z') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_A + (c - 'A'); }
    if (c >= '1' && c <= '9') return HID_KEY_1 + (c - '1');
    if (c == '0') return HID_KEY_0;
    if (c == ' ') return HID_KEY_SPACE;
    if (c == '\n' || c == '\r') return HID_KEY_ENTER;
    if (c == 8 || c == 127) return HID_KEY_BACKSPACE;
    if (c == '\t') return HID_KEY_TAB;
    
    switch(c) {
        case '-': return 45; case '=': return 46; case '[': return 47;
        case ']': return 48; case '\\': return 49; case ';': return 51;
        case '\'': return 52; case '`': return 53; case ',': return 54;
        case '.': return 55; case '/': return 56;
    }
    return 0;
}

int main(void) {
    *(vu32*)0x04000000 = 0x10000;
    *(vu16*)0x05000000 = 31 << 10; 
    *(vu16*)0x0400006C = 0;

    mem_setDsCartridgeCpu(EXMEMCNT_SLOT1_CPU_ARM7);

    rtos_initIrq();
    rtos_startMainThread();
    ipc_initFifoSystem();
    rtos_createEvent(&sVblankEvent);

    while (ipc_getArm7SyncBits() != 7);

    dldi_init();

    ipc_setArm9SyncBits(6);

    rtos_setIrqFunc(RTOS_IRQ_VBLANK, vblankIrq);
    rtos_enableIrqMask(RTOS_IRQ_VBLANK);
    gfx_setVBlankIrqEnabled(true);

    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    
    PrintConsole topScreen;
    consoleInit(&topScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleSelect(&topScreen);
    
    keyboardInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, false, true);
    keyboardShow();
    

    volatile u32* shared_key = (volatile u32*)SHARED_KEY_ADDR;
    *shared_key = 0xFFFFFFFF; 
    DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
    
    consoleClear();
    iprintf("\x1b[1;1H  PicoKeyboard v2.0.0");
    iprintf("\x1b[3;1H  USB Status: ACTIVE");
    iprintf("\x1b[5;1H  L-Button: Backspace");
    iprintf("\x1b[6;1H  R-Button: Escape");
    iprintf("\x1b[7;1H  START:    Enter");

    bool touch_active = false;

    while (1) {
        rtos_waitEvent(&sVblankEvent, true, true);
        scanKeys();
        
        u32 kDown = keysDown();
        u32 kUp = keysUp();
        int touch_char = keyboardUpdate();
        
        uint8_t keycode = 0;
        uint8_t modifier = 0;
        bool send = false;
        bool release = false;

        if (touch_char > 0) {
            keycode = ascii_to_hid(touch_char, &modifier);
            if (keycode > 0) { send = true; touch_active = true; }
        } else if (touch_active && touch_char == 0) {
            release = true;
            touch_active = false;
        }

        if (kDown & KEY_START)  { keycode = HID_KEY_ENTER; send = true; }
        if (kDown & KEY_L)      { keycode = HID_KEY_BACKSPACE; send = true; }
        if (kDown & KEY_R)      { keycode = HID_KEY_ESCAPE; send = true; }
        if (kDown & KEY_UP)     { keycode = HID_KEY_ARROW_UP; send = true; }
        if (kDown & KEY_DOWN)   { keycode = HID_KEY_ARROW_DOWN; send_key = true; }
        
        if (kUp & (KEY_START | KEY_L | KEY_R | KEY_UP | KEY_DOWN)) {
            release = true;
        }

        if (send) {
            *shared_key = make_hid_message(modifier, keycode);
            DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
        } else if (release) {
            *shared_key = make_hid_message(0, 0);
            DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
        }
    }
    return 0;
}
