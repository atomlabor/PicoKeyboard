# DSpico USB Keyboard Test

This repository contains examples of using the DSpico USB port from a DS application. The primary goal of this project is to emulate a USB HID keyboard using the Slot-1 hardware of the DSpico.

## Project Overview

The implementation utilizes a dual-core architecture to enable communication between the Nintendo DS and the RP2040 chip on the DSpico:

* **ARM7**: Handles low-level hardware abstraction, the TinyUSB stack interface, and direct access to the DSpico USB controller.
* **ARM9**: Manages the user interface (libnds keyboard component) and the logic for processing key inputs.
* **Shared Memory**: HID keycodes are communicated via a defined memory area at address `0x02300000`.

## Technical Details

To ensure stability on real hardware (specifically DS Lite and DSi), this build implements a specific boot sequence:

1. Initialization of the libtwl RTOS kernel on the ARM9.
2. Synchronized handshake with the ARM7 via IPC sync bits.
3. Integration of a DLDI proxy driver (`dldiIpc`) to ensure compatibility with the DSpico Launcher and prevent ARM9 crashes during loading.
4. Mapping of ASCII values from the DS keyboard to USB HID scancodes, including modifier support (Shift, etc.).

## Directory Structure

* `arm7/`: Source code for the ARM7 processor, including the USB stack.
* `arm9/`: Source code for the main application and user interface.
* `platform/`: Contains the TinyUSB platform code specific to the DSpico.
* `libs/`: Necessary libraries (libtwl) for hardware interaction.

## Environment and Build Process

The examples require a functional **devkitPro/libnds** environment. The build process depends on the libtwl infrastructure.

To compile, follow these steps:
1. Ensure the `DEVKITARM` environment variable is correctly set.
2. Run `make` in the root directory.
3. Launch the resulting `.nds` file via the DSpico Launcher on the hardware.

## License

The platform code and examples are licensed under the **Zlib license**. For details, see the `LICENSE.txt` file.
