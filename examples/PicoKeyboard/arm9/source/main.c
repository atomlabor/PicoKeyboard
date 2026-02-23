#include <nds.h>
#include <stdio.h>
#include "hid_keycodes.h"

#define FIFO_KEYBOARD FIFO_USER_01

static inline u32 make_hid_message(uint8_t modifier, uint8_t keycode)
{
    return ((u32)modifier << 24) | ((u32)keycode << 16);
}

static uint8_t ascii_to_hid(int c, uint8_t *modifier)
{
    *modifier = 0;
    if (c >= 'a' && c <= 'z') return HID_KEY_A + (c - 'a');
    if (c >= 'A' && c <= 'Z') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_A + (c - 'A');
    }
    if (c >= '1' && c <= '9') return HID_KEY_1 + (c - '1');
    if (c == '0')               return HID_KEY_0;
    if (c == ' ')               return HID_KEY_SPACE;
    if (c == '\n' || c == '\r') return HID_KEY_ENTER;
    if (c == 8)                 return HID_KEY_BACKSPACE;
    return 0;
}

int main(void)
{
    // Video-Setup: Strikt getrennt, damit es keine Grafik-Fehler gibt
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    // Oberes Display: Sauberer Textmodus (statt consoleDemoInit)
    PrintConsole topScreen;
    consoleInit(&topScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleSelect(&topScreen);

    // Unteres Display: Tastatur initialisieren
    keyboardInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, false, true);
    keyboardShow();

    // WICHTIG: Das DSpico ARM7-Betriebssystem wartet auf dieses "Go"-Signal!
    // Ohne diese Zeile bleibt der ARM7 hängen und der USB-Port startet niemals.
    REG_IPC_SYNC = (REG_IPC_SYNC & 0xF0FF) | (6 << 8);

    // FIFO initialisieren
    fifoInit();

    // Status auf dem oberen Display
    consoleClear();
    iprintf("\x1b[1;1H  PicoKeyboard v2.0.0");
    iprintf("\x1b[2;1H  Status: USB Active");
    iprintf("\x1b[4;1H  Buttons:");
    iprintf("\x1b[5;1H  A/B = a/b");
    iprintf("\x1b[6;1H  START = Enter");
    iprintf("\x1b[7;1H  SELECT = Space");
    iprintf("\x1b[8;1H  DPAD = Arrows");
    iprintf("\x1b[9;1H  L = Backspace");
    iprintf("\x1b[10;1H  R = Escape");
    iprintf("\x1b[12;1H  Touchscreen: aktiv");

    bool touch_key_active = false;

    while (1)
    {
        // VBlank gehört IMMER an den Anfang der Schleife für flüssiges Timing
        swiWaitForVBlank(); 
        scanKeys();
        
        u32 keys_down = keysDown();
        u32 keys_up   = keysUp();
        int touch_char = keyboardUpdate();

        uint8_t keycode  = 0;
        uint8_t modifier = 0;
        bool send_key    = false;
        bool send_release = false;

        // Touchscreen-Eingabe hat Priorität
        if (touch_char > 0) {
            keycode = ascii_to_hid(touch_char, &modifier);
            if (keycode > 0) {
                send_key = true;
                touch_key_active = true;
            }
        }
        // Hardware-Buttons
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

        // Taste loslassen
        if (keys_up & (KEY_A | KEY_B | KEY_START | KEY_SELECT |
                       KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R)) {
            send_release = true;
        } else if (touch_key_active && touch_char == 0) {
            send_release = true;
            touch_key_active = false;
        }

        if (send_key) {
            fifoSendValue32(FIFO_KEYBOARD, make_hid_message(modifier, keycode));
        }
        if (send_release) {
            fifoSendValue32(FIFO_KEYBOARD, make_hid_message(0, 0));
        }
    }

    return 0;
}
