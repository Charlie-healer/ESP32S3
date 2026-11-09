#pragma once
#include <stdint.h>

// Shared by the PWR-key handler, CPU governor and brightness settings.
// screenBright is the level restored after PWR wakes the panel.
bool screenDark = false;
uint16_t screenBright = 200;
