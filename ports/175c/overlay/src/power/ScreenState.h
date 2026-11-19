#pragma once
#include <stdint.h>

// Shared by the PWR-key handler, CPU governor and brightness settings.
// screenBright is the level restored after PWR wakes the panel.
bool screenDark = false;      // panel truly off, or AOD (see screenAod)
bool screenAod = false;       // panel stays on at aodBright with a 1 Hz clock
uint16_t screenBright = 128;
uint16_t aodBright = 18;
bool aodEnabled = true;
uint32_t aodAfterMs = 15000;
