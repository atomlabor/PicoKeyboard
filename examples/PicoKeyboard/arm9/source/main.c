#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "tusb.h"

// --- TinyUSB HID Callbacks ---
// (void)-Casts verhindern Compiler-Fehler wegen ungenutzter Variablen

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

// --- Hilfsfunktion: NDS Touch-Zeichen zu USB HID Keycode ---
uint8_t ascii_to_hid(int c, uint8_t *modifier) {
    *modifier = 0;
    
    // Kleinbuchstaben
    if (c >= 'a' && c <= 'z') return HID_KEY_A + (c - 'a');
    
    // Großbuchstaben (Erfordert Shift-Modifier)
    if (c >= 'A' && c <= 'Z') {
        *modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return HID_KEY_A + (c - 'A');
    }
    
    // Zahlen
    if (c >= '1' && c <= '9') return HID_KEY_1 + (c - '1');
    if (c == '0') return HID_KEY_0;
    
    // Spezielle Tasten
    if (c == ' ') return HID_KEY_SPACE;
    if (c == '\n' || c == '\r') return HID_KEY_ENTER;
    if (c == 8) return HID_KEY_BACKSPACE; // 8 = ASCII für Backspace
    
    return 0;
}

// --- Hauptprogramm ---

int main(void) {
    // Initialisiert den unteren Touchscreen mit der libnds-Tastatur 
    // und den oberen Bildschirm als Textkonsole.
    keyboardDemoInit();
    
    // Konsole leeren für sauberes Layout
    consoleClear();
    
    // UI-Text (Linksbündig, 4 Zeilen Platz - internationale Sprache)
    iprintf("\x1b[1;1HPicoKeyboard v2.0.0");
    iprintf("\x1b[2;1HStatus: USB Active");
    iprintf("\x1b[3;1HMode: Touchscreen + Keys");
    iprintf("\x1b[4;1HAverage Pace: N/A");

    // TinyUSB initialisieren (Nutzt den DSpico Hardware-Layer)
    tusb_init();

    bool touch_key_active = false;

    while (1) {
        // TinyUSB Hintergrund-Task aufrufen
        tud_task();

        scanKeys();
        u32 keys_down = keysDown();
        u32 keys_up = keysUp();
        
        // Liest Eingaben der virtuellen NDS-Tastatur aus
        int touch_char = keyboardUpdate();

        // Wenn der Mac bereit ist, Eingaben zu empfangen
        if (tud_hid_ready()) {
            uint8_t keycode[6] = {0};
            uint8_t modifier = 0;
            bool send_report = false;

            // 1. Virtuelle Touchscreen-Tastatur verarbeiten
            if (touch_char > 0) {
                uint8_t hid_code = ascii_to_hid(touch_char, &modifier);
                if (hid_code > 0) {
                    keycode[0] = hid_code;
                    send_report = true;
                    touch_key_active = true;
                }
            } 
            // 2. Hardware-Tasten verarbeiten (nur wenn Touch gerade nicht genutzt wird)
            else if (keys_down & KEY_A) { keycode[0] = HID_KEY_A; send_report = true; }
            else if (keys_down & KEY_B) { keycode[0] = HID_KEY_B; send_report = true; }
            else if (keys_down & KEY_START) { keycode[0] = HID_KEY_ENTER; send_report = true; }
            else if (keys_down & KEY_SELECT) { keycode[0] = HID_KEY_SPACE; send_report = true; }
            else if (keys_down & KEY_UP) { keycode[0] = HID_KEY_ARROW_UP; send_report = true; }
            else if (keys_down & KEY_DOWN) { keycode[0] = HID_KEY_ARROW_DOWN; send_report = true; }
            else if (keys_down & KEY_LEFT) { keycode[0] = HID_KEY_ARROW_LEFT; send_report = true; }
            else if (keys_down & KEY_RIGHT) { keycode[0] = HID_KEY_ARROW_RIGHT; send_report = true; }
            else if (keys_down & KEY_L) { keycode[0] = HID_KEY_BACKSPACE; send_report = true; }
            else if (keys_down & KEY_R) { keycode[0] = HID_KEY_ESCAPE; send_report = true; }

            // Leeren Report senden, wenn Tasten losgelassen werden
            if (keys_up & (KEY_A | KEY_B | KEY_START | KEY_SELECT | KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_L | KEY_R)) {
                send_report = true;
            } else if (touch_key_active && touch_char == 0) {
                // Simuliert das Loslassen einer Touchscreen-Taste
                send_report = true;
                touch_key_active = false;
            }

            // Report via TinyUSB an den Mac senden
            if (send_report) {
                tud_hid_keyboard_report(0, modifier, keycode);
            }
        }

        swiWaitForVBlank();
    }

    return 0;
}
