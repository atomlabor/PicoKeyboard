#include "tusb.h"
#include "usb_descriptors.h"
#include <string.h>

//--------------------------------------------------------------------
// Device Descriptor
//--------------------------------------------------------------------

tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCAFE,
    .idProduct          = 0x4000,
    .bcdDevice          = 0x0100,

    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,

    .bNumConfigurations = 0x01
};

uint8_t const* tud_descriptor_device_cb(void)
{
    return (uint8_t const*) &desc_device;
}

//--------------------------------------------------------------------
// HID Report Descriptor
//--------------------------------------------------------------------

uint8_t const desc_hid_report[] =
{
    TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return desc_hid_report;
}

//--------------------------------------------------------------------
// Configuration Descriptor
//--------------------------------------------------------------------

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID         0x81

uint8_t const desc_configuration[] =
{
    TUD_CONFIG_DESCRIPTOR(
        1,
        ITF_NUM_TOTAL,
        0,
        CONFIG_TOTAL_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
        100
    ),

    TUD_HID_DESCRIPTOR(
        ITF_NUM_HID,
        0,
        HID_ITF_PROTOCOL_KEYBOARD,
        sizeof(desc_hid_report),
        EPNUM_HID,
        CFG_TUD_HID_EP_BUFSIZE,
        10
    )
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

//--------------------------------------------------------------------
// String Descriptors
//--------------------------------------------------------------------

char const* string_desc_arr[] =
{
    (const char[]) { 0x09, 0x04 },
    "Atomlabor",
    "DS Pico Keyboard",
    "0001"
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if (index == STRID_LANGID)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;

        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++)
        {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
//--------------------------------------------------------------------
// HID Callbacks (required by TinyUSB linker)
//--------------------------------------------------------------------

// Wird aufgerufen wenn Host GET_REPORT schickt
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

// Wird aufgerufen wenn Host SET_REPORT schickt (z.B. CapsLock-LED)
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) bufsize;

    // Optional: macOS setzt hier die Keyboard-LEDs
    // Bit 0 = NumLock, Bit 1 = CapsLock, Bit 2 = ScrollLock
    if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize >= 1) {
        // uint8_t leds = buffer[0];
        // -> hier könntest du z.B. eine LED auf dem DS ansteuern
    }
}
