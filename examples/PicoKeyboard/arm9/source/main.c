#include <nds.h>
#include <stdio.h>
#include "hid_keycodes.h"

// Sichere Shared Memory Adresse
#define SHARED_KEY_ADDR 0x02300000

// Erstellt HID-Message aus Modifier und Keycode
static inline u32 make_hid_message(uint8_t modifier, uint8_t keycode) {
    return ((u32)modifier << 24) | ((u32)keycode << 16);
}

// Konvertiert ASCII-Zeichen zu HID-Keycode
static uint8_t ascii_to_hid(int c, uint8_t *modifier) {
    *modifier = 0;
    
    // Kleinbuchstaben
    if (c >= 'a' && c <= 'z') return HID_KEY_A + (c - 'a');
    
    // Großbuchstaben (mit Shift)
    if (c >= 'A' && c <= 'Z') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_A + (c - 'A');
    }
    
    // Zahlen
    if (c >= '1' && c <= '9') return HID_KEY_1 + (c - '1');
    if (c == '0') return HID_KEY_0;
    
    // Häufige Sonderzeichen
    if (c == ' ')  return HID_KEY_SPACE;
    if (c == '\n' || c == '\r') return HID_KEY_ENTER;
    if (c == 8 || c == 127) return HID_KEY_BACKSPACE;  // FIX: ASCII 127 (DEL) hinzugefügt
    if (c == '\t') return HID_KEY_TAB;                 // FIX: Tab-Taste hinzugefügt
    
    // Erweiterte Sonderzeichen (optional, wenn Touchscreen-Tastatur sie unterstützt)
    if (c == '.') return HID_KEY_DOT;
    if (c == ',') return HID_KEY_COMMA;
    if (c == '-') return HID_KEY_MINUS;
    if (c == '=') return HID_KEY_EQUAL;
    if (c == '[') return HID_KEY_LEFTBRACE;
    if (c == ']') return HID_KEY_RIGHTBRACE;
    if (c == ';') return HID_KEY_SEMICOLON;
    if (c == '\'') return HID_KEY_APOSTROPHE;
    if (c == '\\') return HID_KEY_BACKSLASH;
    if (c == '/') return HID_KEY_SLASH;
    if (c == '`') return HID_KEY_GRAVE;
    
    // Sonderzeichen mit Shift
    if (c == '!') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_1;
    }
    if (c == '@') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_2;
    }
    if (c == '#') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_3;
    }
    if (c == '$') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_4;
    }
    if (c == '%') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_5;
    }
    if (c == '^') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_6;
    }
    if (c == '&') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_7;
    }
    if (c == '*') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_8;
    }
    if (c == '(') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_9;
    }
    if (c == ')') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_0;
    }
    
    return 0;
}

int main(void) {
    // Exception Handler für Debugging
    defaultExceptionHandler();
    
    // Video-System initialisieren
    powerOn(POWER_ALL_2D);
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    
    // Oberer Bildschirm: Console für HUD
    PrintConsole topScreen;
    consoleInit(&topScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    consoleSelect(&topScreen);
    
    // Unterer Bildschirm: Touchscreen-Tastatur
    keyboardInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x256, 20, 0, false, true);
    keyboardShow();
    
    // Shared Memory initialisieren
    volatile u32* shared_key = (volatile u32*)SHARED_KEY_ADDR;
    *shared_key = 0xFFFFFFFF;
    DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
    
    // HUD anzeigen (FIX: Konsistentes Deutsch, vollständige Dokumentation)
    consoleClear();
    iprintf("\x1b[1;1H  PicoKeyboard v2.0.1");
    iprintf("\x1b[2;1H  Status: USB Aktiv");
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
    iprintf("\x1b[15;1H  Verbindung: DSi USB");
    
    bool touch_key_active = false;
    
    // Hauptschleife
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
        
        // Touchscreen-Eingabe verarbeiten
        if (touch_char > 0) {
            keycode = ascii_to_hid(touch_char, &modifier);
            if (keycode > 0) {
                send_key = true;
                touch_key_active = true;
            }
        }
        // Hardware-Buttons verarbeiten
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
        
        // Taste loslassen erkennen
        if (keys_up & (KEY_A | KEY_B | KEY_START | KEY_SELECT |
                       KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R)) {
            send_release = true;
        } else if (touch_key_active && touch_char == 0) {
            send_release = true;
            touch_key_active = false;
        }
        
        // Keycode an ARM7 senden
        if (send_key) {
            *shared_key = make_hid_message(modifier, keycode);
            DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
        }
        
        // Release-Event senden (alle Tasten los)
        if (send_release) {
            *shared_key = make_hid_message(0, 0);
            DC_FlushRange((void*)SHARED_KEY_ADDR, 4);
        }
    }
    
    return 0;
}
