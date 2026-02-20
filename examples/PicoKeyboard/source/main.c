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

// --- Hauptprogramm ---

int main(void) {
    // NDS Video- und Konsolen-Initialisierung
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    // Initialisiere die Konsole auf dem oberen Bildschirm
    consoleInit(0, 3, 0, NULL, 0, 15);
    
    // UI-Text (Linksbündig, 4 Zeilen Platz - internationale Sprache)
    iprintf("\x1b[1;1HPicoKeyboard Toolset");
    iprintf("\x1b[2;1HStatus: USB Active");
    iprintf("\x1b[3;1HPress NDS Keys to type");
    iprintf("\x1b[4;1HMode: Keyboard Output");

    // TinyUSB initialisieren (Nutzt den DSpico Hardware-Layer)
    tusb_init();

    while (1) {
        // TinyUSB Hintergrund-Task aufrufen
        tud_task();

        scanKeys();
        u32 keys_down = keysDown();
        u32 keys_up = keysUp();

        // Wenn der Mac (Host) bereit ist, Eingaben zu empfangen
        if (tud_hid_ready()) {
            uint8_t keycode[6] = {0};
            bool send_report = false;

            // Simples Mapping der NDS-Tasten auf USB-Keycodes
            if (keys_down & KEY_A) {
                keycode[0] = HID_KEY_A;
                send_report = true;
            } else if (keys_down & KEY_B) {
                keycode[0] = HID_KEY_B;
                send_report = true;
            } else if (keys_down & KEY_X) {
                keycode[0] = HID_KEY_X;
                send_report = true;
            } else if (keys_down & KEY_Y) {
                keycode[0] = HID_KEY_Y;
                send_report = true;
            } else if (keys_down & KEY_START) {
                keycode[0] = HID_KEY_ENTER;
                send_report = true;
            } else if (keys_down & KEY_SELECT) {
                keycode[0] = HID_KEY_SPACE;
                send_report = true;
            }

            // Leeren Report senden, wenn Tasten losgelassen werden (Key-Up)
            if (keys_up & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_START | KEY_SELECT)) {
                send_report = true;
                // Das Array 'keycode' bleibt {0}, was dem Mac signalisiert, dass keine Taste mehr gedrückt wird.
            }

            // Report via TinyUSB an den Mac senden
            if (send_report) {
                // report_id = 0, modifier = 0
                tud_hid_keyboard_report(0, 0, keycode);
            }
        }

        swiWaitForVBlank();
    }

    return 0;
}