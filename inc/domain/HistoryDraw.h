#pragma once

#include "Stm32LcdDriver.h"
#include "PowerHistory.h"

void drawHistory(Stm32LcdDriver& lcd, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const PowerHistory& history);