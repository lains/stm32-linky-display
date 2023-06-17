#pragma once

#include "Stm32LcdDriver.h"
#include "PowerHistory.h"

/**
 * @brief Represent a power history as a graph on a display
 * 
 * @param lcd The display to use to draw
 * @param x The x coord (in pixels) of the top left position of the rectangle area to use to draw
 * @param y The y coord (in pixels) of the top left position of the rectangle area to use to draw
 * @param width The width (in pixels) of the rectangle area to use to draw
 * @param height The height (in pixels) of the rectangle area to use to draw
 * @param history The history data to draw
 * @param debugContext An optional debug context pointer to display a debug information line
 */
void drawHistory(Stm32LcdDriver& lcd, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const PowerHistory& history, void* debugContext = nullptr);