#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// String IDs
enum
{
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

// Interface numbers
enum
{
    ITF_NUM_HID = 0,
    ITF_NUM_TOTAL
};

#ifdef __cplusplus
}
#endif
