#pragma once
#include <cstdint>

void powerBegin();                // call first in setup(): releases sleep holds, powers off the NeoPixel
void ampEnable(bool on);          // drives PIN_AMP_ENABLE; releases any sleep hold first
bool buttonPressedEdge();         // debounced HIGH->LOW edge; true once per press
[[noreturn]] void deepSleep();    // parks and holds output pins, arms ext0 wake on button, sleeps
uint32_t rngU32();
