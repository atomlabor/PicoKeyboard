#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include "hid_keycodes.h"

#define SHARED_KEY_ADDR 0x02300000

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
    if (c == '-') return 45; 
    if (c == '=') return 46; 
    if (c == '[') return 47; 
    if (c == ']') return 48; 
    if (c == '\\') return 49; 
    if (c == ';') return 51; 
    if (c == '\'') return 52; 
    if (c == '`') return 53; 
    if (c == ',') return 54; 
    if (c == '.') return 55; 
    if (c == '/') return 56; 
    if (c == '!') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_1; }
    if (c == '@') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_2; }
    if (c == '#') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_3; }
    if (c == '$') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_4; }
    if (c == '%') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_5; }
    if (c == '^') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_6; }
    if (c == '&') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_7; }
    if (c == '*') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_8; }
    if (c == '(') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_9; }
    if (c == ')') { *modifier = KEYBOARD_MODIFIER_LEFTSHIFT; return HID_KEY_0; }
    return 0;
}

int main(int argc, char* argv[]) {
    // 1. Zwingend für nds-bootstrap: Loader-Absturz verhindern
    fatInitDefault();

    // 2. Cartridge-Steuerung an DSpico / ARM7 abgeben
    sysSetCartOwner(BUS_OWNER_ARM7);

    // 3. LEBENSWICHTIGER SPINLOCK (Kein swiWaitForVBlank nutzen!)
    while (((REG_IPC_SYNC >> 8) & 0x0F) != 7);
    REG_IPC_SYNC = (REG_IPC_SYNC & 0xFFF0) | 6;

    // --- Ab hier läuft reine, kompatible NDS-Grafik ---
    powerOn(POWER_ALL_2D);
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
    iprintf("\x1b[1;1H  PicoKeyboard v2.0.4");
    iprintf("\x1b[2;1H  Status: Boot OK");
    iprintf("\x1b[3;1H  ----------------------");
    iprintf("\x1b[4;1H  Hardware-Tasten:");
    iprintf("\x1b[5;1H    A/B     = a/b");
    iprintf("\x1b[6;1H    START   = Enter");
    iprintf("\x1b[7;1H    SELECT  = Leertaste");
    iprintf("\x1b[8;1H    D-Pad   = Pfeiltasten");
    iprintf("\x1b[9;1H    L       = Backspace");
    iprintf("\x1b[10;1H    R       = Escape");
    iprintf("\x1b[11;1H  ----------------------");
    iprintf("\x1b[12;1H  Touchscreen: Aktiv");
    iprintf("\x1b[13;1H  ----------------------");
    iprintf("\x1b[15;1H  Verbindung: DSpico");
    
    bool touch_key_active = false;
    
    while (1) {
        swiWaitForVBlank();
        scanKeys();
        
        u32 keys_down = keysDown();
        u32 keys_up   = keysUp();
        int touch_char = keyboardUpdate();
        
        uint8_t keycode  = 0;
        uint8_t modifier = 0;
        bool send_key    = false;
        bool send_release = false;
        
        if (touch_char > 0) {
            keycode = ascii_to_hid(touch_char, &modifier);
            if (keycode > 0) { send_key = true; touch_key_active = true; }
        }
        else if (keys_down & KEY_A)      { keycode = HID_KEY_A;           send_key = true; }
        else if (keys_down & KEY_B)      { keycode = HID_KEY_B;           send_key = true; }
        else if (keys_down & KEY_START)  { keycode = HID_KEY_ENTER;       send_key = true; }
        else if (keys_down & KEY_SELECT) { keycode = HID_KEY_SPACE;       send_key = true; }
        else if (keys_down & KEY_UP)     { keycode = HID_KEY_ARROW_UP;    send_key = true; }
        else if (keys_down & KEY_DOWN)   { keycode = HID_KEY_ARROW_DOWN;  send_key = true; }
        else if (keys_down & KEY_LEFT)   { keycode = HID_KEY_ARROW_LEFT;  send_key = true; }
        else if (keys_down & KEY_RIGHT)  { keycode = HID_KEY_ARROW_RIGHT; send_key = true; }
        else if (keys_down & KEY_L)      { keycode = HID_KEY_BACKSPACE;   send_key = true; }
        else if (keys_down & KEY_R)      { keycode = HID_KEY_ESCAPE;      send_key = true; }
        
        if (keys_up & (KEY_A | KEY_B | KEY_START | KEY_SELECT | KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R)) {
            send_release = true;
        } else if (touch_key_active && touch_char == 0) {
            send_release = true;
            touch_key_active = false;
        }
        
        if (send_key) {
            *shared_key = make_hid_message(modifier, keycode);
            DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
        }
        
        if (send_release) {
            *shared_key = make_hid_message(0, 0);
            DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
        }
    }
    
    return 0;
}
