#include "HistoryDraw.h"

#include <climits>

void drawDebugLine(Stm32LcdDriver& lcd, uint16_t y, const PowerHistory& history, unsigned int nbHistoryEntries, uint16_t debugX, uint16_t debugYtop, uint16_t debugYbottom, int debugPower, bool debugPowerIsExact, int debugValue=INT_MIN, void* debugContext=nullptr) {
    if (debugContext != nullptr && debugContext != (void*)(-1)) {
        uint32_t debugUint32 = *(static_cast<uint32_t*>(debugContext)); // Grab the uint32_t entry in the provided pointed context to display it
        debugValue = static_cast<int>(debugUint32);
    }
    uint8_t pos = 0;
    char statusLine[]="DH=@@@@ X=@@@ Yt=@@@ Yb=@@@ P=@@@@@@@          "; /* Dbg is not always displayed but should be allocated in the buffer (see following line) */
    //                "DH=@@@@ X=@@@ Yt=@@@ Yb=@@@ P=@@@@@@@ D=@@@@@@@";
    pos+=3; // "DH="
    statusLine[pos++]=(nbHistoryEntries / 1000) % 10 + '0';
    statusLine[pos++]=(nbHistoryEntries / 100) % 10 + '0';
    statusLine[pos++]=(nbHistoryEntries / 10) % 10 + '0';
    statusLine[pos++]=(nbHistoryEntries / 1) % 10 + '0';
    pos++;

    pos+=2;
    statusLine[pos++]=(debugX / 100) % 10 + '0';
    statusLine[pos++]=(debugX / 10) % 10 + '0';
    statusLine[pos++]=(debugX / 1) % 10 + '0';
    pos++;

    pos+=3;
    if (debugYtop < 1000) {
        statusLine[pos++]=(debugYtop / 100) % 10 + '0';
        statusLine[pos++]=(debugYtop / 10) % 10 + '0';
        statusLine[pos++]=(debugYtop / 1) % 10 + '0';
    }
    else if (debugYtop == UINT16_MAX) {
        statusLine[pos++]='!';
        statusLine[pos++]='!';
        statusLine[pos++]='!';
    }
    else
        pos+=3;
    pos++;

    pos+=3;
    if (debugYbottom < 1000) {
        statusLine[pos++]=(debugYbottom / 100) % 10 + '0';
        statusLine[pos++]=(debugYbottom / 10) % 10 + '0';
        statusLine[pos++]=(debugYbottom / 1) % 10 + '0';
    }
    else if (debugYbottom == UINT16_MAX) {
        statusLine[pos++]='!';
        statusLine[pos++]='!';
        statusLine[pos++]='!';
    }
    else
        pos+=3;
    pos++;

    pos+=2; // "P="
    if (debugPower != INT_MIN) {
        if (debugPower < -99999 || debugPower > 99999) { /* Max displayable value +/-99kW */
            statusLine[pos++]='*'; /* Overflow/underflow */
        }
        else {
            statusLine[pos++]=debugPowerIsExact?' ':'~';
        }
        if (debugPower < 0) {
            statusLine[pos++]='-';
            debugPower = -debugPower;
        }
        else {
            statusLine[pos++]=' ';
        }
        statusLine[pos++]=(debugPower / 10000) % 10 + '0';
        statusLine[pos++]=(debugPower / 1000) % 10 + '0';
        statusLine[pos++]=(debugPower / 100) % 10 + '0';
        statusLine[pos++]=(debugPower / 10) % 10 + '0';
        statusLine[pos++]=(debugPower / 1) % 10 + '0';
    }
    else
        pos+=7; /* Skip all digits related to P= */

    pos++;
    // statusLine[pos++]='D';
    // statusLine[pos++]='=';
    // if (debugValue != INT_MIN) {
    //     if (debugValue < 0) {
    //         statusLine[pos++]='-';
    //         debugValue = -debugValue;
    //     }
    //     else
    //         statusLine[pos++]=(debugValue / 1000000) % 10 + '0';
    //     statusLine[pos++]=(debugValue / 100000) % 10 + '0';
    //     statusLine[pos++]=(debugValue / 10000) % 10 + '0';
    //     statusLine[pos++]=(debugValue / 1000) % 10 + '0';
    //     statusLine[pos++]=(debugValue / 100) % 10 + '0';
    //     statusLine[pos++]=(debugValue / 10) % 10 + '0';
    //     statusLine[pos++]=(debugValue / 1) % 10 + '0';
    // }
    // pos++;

    auto get_font24_ptr = [](const char c) {
        unsigned int bytesPerGlyph = Font24.Height * ((Font24.Width + 7) / 8);
        return &(Font24.table[(c-' ') * bytesPerGlyph]);
    };

    lcd.drawText(0, y, statusLine, Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::White, Stm32LcdDriver::LCD_Color::Black);
}

void drawHistory(Stm32LcdDriver& lcd, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const PowerHistory& history, void* debugContext) {
    if (width == 0 || height == 0)
        return;
    
    if (debugContext) { /* If the debug line needs to be drawn, reduce the history graph area height accordinly */
        height -= Font24.Height;
    }
    
    uint16_t xright = x + width - 1;

    unsigned int nbHistoryEntries = width; /* Initially try to fill-in the full width of the area */

    PowerHistoryEntry powerMeasurements[nbHistoryEntries];
    history.getLastPower(nbHistoryEntries, powerMeasurements);
    if (nbHistoryEntries > width) {    /* Sanity, overflow , exclude oldest entries to fit on display */
        nbHistoryEntries = width;
    }

    uint16_t debugX = UINT16_MAX;
    uint16_t debugYtop = UINT16_MAX;
    uint16_t debugYbottom = UINT16_MAX;
    int debugPower = INT_MIN;
    bool debugPowerIsExact = false;
    int debugValue = INT_MIN;

    const int maxPower = 3000;
    const int minPower = -1500;
    uint16_t zeroSampleAbsoluteY = y;
    zeroSampleAbsoluteY += static_cast<uint16_t>(static_cast<unsigned long int>(maxPower) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    for (unsigned int measurementAge = 0; measurementAge < nbHistoryEntries; measurementAge++) {
        uint16_t thisSampleAbsoluteX = xright - measurementAge;   /* First measurement sample is at the extreme right of the allocated area, next samples will be placed to the left */
        if (debugX == UINT16_MAX) debugX = thisSampleAbsoluteX;
        PowerHistoryEntry& thisSampleEntry = powerMeasurements[measurementAge];
        TicEvaluatedPower& thisSamplePower = thisSampleEntry.power;

        if (debugValue == INT_MIN) {
            if (nbHistoryEntries>0) {
                debugValue = thisSampleEntry.nbSamples;
            }
        }
        if (debugPower == INT_MIN) {
            if (thisSamplePower.isValid) {
                if (thisSamplePower.isExact) {
                    debugPower = thisSamplePower.minValue;
                    debugPowerIsExact = true;
                }
                else {
                    if (thisSamplePower.maxValue > 0) {
                        debugPower = thisSamplePower.minValue;
                        debugPowerIsExact = true;
                    }
                    else {  /* For negative value, compute an average */
                        debugPower = (thisSamplePower.minValue + thisSamplePower.maxValue) / 2;
                        debugPowerIsExact = false;
                    }
                }
            }
            else {
                debugPower = 0;
                debugPowerIsExact = false;
            }
        }

        if (thisSamplePower.isValid) {
            /* TODO: Review the code below */
            if (thisSamplePower.maxValue > 0) { /* Positive value, even if range, display the highest value of the range (worst case) */
                uint16_t thisSampleTopAbsoluteY = 0;
                uint16_t thisSampleBottomAbsoluteY = 0;
                uint16_t thisSampleRelativeY = 0;
                if (true || thisSamplePower.maxValue > 0) { /* Positive and exact power */
                    thisSampleRelativeY = (static_cast<unsigned long int>(thisSamplePower.maxValue) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY - thisSampleRelativeY;
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY;
                    if (debugYtop == UINT16_MAX) debugYtop = thisSampleTopAbsoluteY;
                    if (debugYbottom == UINT16_MAX) debugYbottom = thisSampleBottomAbsoluteY;
                    lcd.drawVerticalLine(thisSampleAbsoluteX, thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY-thisSampleTopAbsoluteY, Stm32LcdDriver::Red);
                }
                else if (thisSamplePower.maxValue < 0) { /* Negative and exact power */
                    thisSampleRelativeY = (static_cast<unsigned long int>(-thisSamplePower.maxValue) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY;
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY + thisSampleRelativeY;
                    if (debugYtop == UINT16_MAX) debugYtop = thisSampleTopAbsoluteY;
                    if (debugYbottom == UINT16_MAX) debugYbottom = thisSampleBottomAbsoluteY;
                    lcd.drawVerticalLine(thisSampleAbsoluteX, thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY-thisSampleTopAbsoluteY, Stm32LcdDriver::DarkGreen);
                }
            }
            else {  /* Max value is negative, we are injecting, display the range */
                uint16_t thisSampleMaxRelativeY = 0;
                uint16_t thisSampleMinRelativeY = 0;
                bool maxIsBelow0 = (thisSamplePower.maxValue < 0);
                bool minIsBelow0 = (thisSamplePower.minValue < 0);
                unsigned int maxAbsValue = maxIsBelow0?(-thisSamplePower.maxValue):thisSamplePower.maxValue;
                unsigned int minAbsValue = minIsBelow0?(-thisSamplePower.minValue):thisSamplePower.minValue;
                thisSampleMaxRelativeY = (static_cast<unsigned int>(maxAbsValue) * static_cast<unsigned long int>(height) / static_cast<unsigned int>(maxPower - minPower));
                thisSampleMinRelativeY = (static_cast<unsigned int>(minAbsValue) * static_cast<unsigned long int>(height) / static_cast<unsigned int>(maxPower - minPower));
                uint16_t thisSampleTopAbsoluteY = 0;
                uint16_t thisSampleBottomAbsoluteY = 0;
                if (maxIsBelow0)
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY + thisSampleMaxRelativeY;
                else
                    thisSampleTopAbsoluteY = zeroSampleAbsoluteY - thisSampleMaxRelativeY;
                if (minIsBelow0)
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY + thisSampleMinRelativeY;
                else
                    thisSampleBottomAbsoluteY = zeroSampleAbsoluteY - thisSampleMinRelativeY;
                
                if (thisSampleTopAbsoluteY > thisSampleBottomAbsoluteY) {   /* Failsafe, should not occur */
                    std::swap(thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY);
                }
                if (debugYtop == UINT16_MAX) debugYtop = thisSampleTopAbsoluteY;
                if (debugYbottom == UINT16_MAX) debugYbottom = thisSampleBottomAbsoluteY;
                lcd.drawVerticalLine(thisSampleAbsoluteX, thisSampleTopAbsoluteY, thisSampleBottomAbsoluteY-thisSampleTopAbsoluteY, Stm32LcdDriver::DarkGreen, Stm32LcdDriver::Green, 5);
            }
        }
    }

    auto get_font24_ptr = [](const char c) {
        unsigned int bytesPerGlyph = Font24.Height * ((Font24.Width + 7) / 8);
        return &(Font24.table[(c-' ') * bytesPerGlyph]);
    };

    lcd.drawHorizontalLine(x, zeroSampleAbsoluteY, width, Stm32LcdDriver::Black); /* Draw 0 */
    uint16_t gridY;
    uint16_t gridWidth = nbHistoryEntries;
    uint16_t gridX = x;
    if (width - gridWidth > x)
        gridX = width - gridWidth;
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower - 3000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Black); /* Draw +3000W) */
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower - 2000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Black); /* Draw +2000W */

    bool drawLabels = (gridWidth > 24*5 + 10 && width > 24*5 + 10); /* Enough room to draw 1000W and 2000W label (5 chars)? */
    if (drawLabels) /* Enough room to draw 2000W label (5 chars). Draw above the line (substracting text height from y) */
        lcd.drawText(gridX + 2, gridY - 22, "2000W", Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::Black, Stm32LcdDriver::LCD_Color::Transparent); /* Draw above the line (substracting text height from y) */
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower - 1000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Black); /* Draw +1000W */
    if (drawLabels) /* Enough room to draw 1000W label (5 chars). Draw above the line (substracting text height from y) */
        lcd.drawText(gridX + 2, gridY - 22, "1000W", Font24.Width, Font24.Height, get_font24_ptr, Stm32LcdDriver::LCD_Color::Black, Stm32LcdDriver::LCD_Color::Transparent); /* Draw above the line (substracting text height from y) */
    gridY = y + static_cast<uint16_t>(static_cast<unsigned long int>(maxPower + 1000) * static_cast<unsigned long int>(height) / static_cast<unsigned long int>(maxPower - minPower));
    lcd.drawHorizontalLine(gridX, gridY, gridWidth, Stm32LcdDriver::Black); /* Draw -1000W */

    debugValue = nbHistoryEntries * (4*3) / history.getPowerRecordsPerHour(); /* We divide hours in 12 steps, thus each step is 5 mins */
    for (unsigned int fiveMinStep = 1; fiveMinStep <= nbHistoryEntries * (4*3) / history.getPowerRecordsPerHour(); fiveMinStep++) {
        uint16_t gridX = width;
        if (gridX >= history.getPowerRecordsPerHour() * fiveMinStep / (4*3)) {
            gridX -= history.getPowerRecordsPerHour() * fiveMinStep / (4*3);
            if (fiveMinStep % 12 == 0) { /* gridX is pointing to the edge a hour */
                lcd.drawVerticalLine(gridX, y, height, Stm32LcdDriver::Black); /* Draw each hour, with a double line */
                if (gridX > x)
                    lcd.drawVerticalLine(gridX-1, y, height, Stm32LcdDriver::Black); /* Draw each hour, with a double line */
            }
            else if (fiveMinStep % 3 == 0)  /* gridX is pointing to the edge of a quarter of an hour */
                lcd.drawVerticalLine(gridX, y, height, Stm32LcdDriver::Black); /* Draw each other quarter */
            else
                lcd.drawVerticalLine(gridX, y, height, Stm32LcdDriver::LightGrey); /* Draw each other step of 5 mins */
        }
    }
    if (debugContext) {
        /* If debug line needs to be drawn, draw it just under the history graph */
        drawDebugLine(lcd, y+height, history, nbHistoryEntries, debugX, debugYtop, debugYbottom, debugPower, debugPowerIsExact, debugValue, debugContext);
    }
}
