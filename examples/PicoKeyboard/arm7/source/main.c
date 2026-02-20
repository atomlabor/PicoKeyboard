#include <nds.h>
#include <dspico.h>

void vblankHandler(void) {}

int main(void) {
    irqInit();
    irqSet(IRQ_VBLANK, vblankHandler);
    irqEnable(IRQ_VBLANK);

    dspicoInit();

    while (1) {
        swiWaitForVBlank();
    }
    return 0;
}
