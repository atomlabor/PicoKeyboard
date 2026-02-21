#include <nds.h>

int main() {
    // arm9 macht nichts - USB läuft auf arm7
    while (true) {
        swiWaitForVBlank();
    }
    return 0;
}
