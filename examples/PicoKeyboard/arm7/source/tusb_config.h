#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// --- System-Konfiguration ---
#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE
#define CFG_TUD_ENDPOINT0_SIZE      64

// --- Aktivierte USB-Klassen ---
// Wir bauen eine Tastatur (HID), also wird nur diese Klasse aktiviert (1)
#define CFG_TUD_HID                 1
#define CFG_TUD_CDC                 0
#define CFG_TUD_MSC                 0
#define CFG_TUD_MIDI                0
#define CFG_TUD_VENDOR              0
#define CFG_TUD_AUDIO               0
#define CFG_TUD_VIDEO               0

// --- HID Puffergröße ---
#define CFG_TUD_HID_EP_BUFSIZE      16

#ifdef __cplusplus
}
#endif

#endif
