#ifndef _STM32LCDDRIVER_H_
#define _STM32LCDDRIVER_H_

#ifdef STM32F469xx
#include "stm32f4xx_hal.h"
#endif
#ifdef STM32F769xx
#include "stm32f7xx_hal.h"
#endif

#ifdef USE_STM32469I_DISCOVERY
#include "stm32469i_discovery_lcd.h"
#endif
#ifdef USE_STM32F769I_DISCO
#include "stm32f769i_discovery_lcd.h"
#endif

extern "C" {
DSI_HandleTypeDef* get_hdsi(void); // C-linkage exported getter for hdsi handler
void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi);
}

LTDC_HandleTypeDef* getLcdLtdcHandle();
DSI_HandleTypeDef* getLcdDsiHandle();

/**
 * @brief Serial link communication class (singleton)
 */
class Stm32LcdDriver {
public:
    typedef const uint8_t* (*FCharacterToGlyphPtr)(const char c); /*!< The prototype of functions transforming a character into a pointer to the character glyph */

    typedef enum {
        SwitchToDraftIsPending = 0,
        DisplayingDraft,
        CopyingDraftToFinalIsPending,
        CopyDraftToFinalIsDone, /* Unused, would be nice not to poll on DMA2D while copying */
        SwitchToFinalIsPending,
        DisplayingFinal
    } LCD_Display_Update_State;

    typedef enum {
        Black = LCD_COLOR_BLACK,
        White = LCD_COLOR_WHITE,
        Green = LCD_COLOR_GREEN,
        Orange = LCD_COLOR_ORANGE,
        Red = LCD_COLOR_RED,
        Blue = LCD_COLOR_BLUE,
        Grey = LCD_COLOR_GRAY,
        LightGrey = LCD_COLOR_LIGHTGRAY,
        DarkGreen = LCD_COLOR_DARKGREEN,
        DarkRed = LCD_COLOR_DARKRED,
        DarkBlue = LCD_COLOR_DARKBLUE,
        Transparent = static_cast<uint32_t>(0x00000000),
        None = static_cast<uint32_t>(0x00ffffff)
    } LCD_Color;

    typedef void(*FWaitForDisplayRefreshFunc)(void* context);

    /**
     * @brief Singleton instance getter
     * 
     * @return The singleton instance of this class
     */
    static Stm32LcdDriver& get();

    /**
     * @brief Initialize and draw the framebuffer to the LCD
     * 
     * @note When returing from this method, the LCD will be requested to display the final framebuffer
     *       Before drawing anything to the draft framebuffer, one should check that the display has actually finished switching by invoking waitForFinalDisplayed()
     * 
     * @return true On success, false otherwise
     */
    bool start();

    void requestDisplayDraft();

    void waitForDraftDisplayed(FWaitForDisplayRefreshFunc toRunWhileWaiting = nullptr, void* context = nullptr) const;

    void requestDisplayFinal();

    void waitForFinalDisplayed(FWaitForDisplayRefreshFunc toRunWhileWaiting = nullptr, void* context = nullptr) const;

    void displayDraft(FWaitForDisplayRefreshFunc toRunWhileWaiting = nullptr, void* context = nullptr); /* Combined requestDisplayDraft()+waitForDraftDisplayed() */

    void copyDraftToFinal();

    uint16_t getWidth() const;

    uint16_t getHeight() const;

    /**
     * @brief Draw a plain 1-pixel wide vertical line
     * 
     * @param x The x position of the line on the LCD
     * @param y The top y position of the line on the LCD
     * @param yPlus The length of the line (in pixels), drawn towards the bottom of the screen, starting from (x;y)
     * @param color An optional 32-bit text color to use when drawing
     * @param alternateColor An optional 32-bit text color that will alternate with @p color when drawing the line (if set to LCD_Color::None, we will draw a solid line)
     * @param alternateRatio The ratio between the number of pixels drawn with @p color and those drawn with @p alternateColor or 0 to only use color
     * 
     * @example Invoking this method with arguments color=Black, alternateColor=None and whatever value in alternateRatio will draw a solid black line
     *          Invoking this method with arguments color=Blue, alternateColor=Red and alternateRatio=0 will draw a solid blue line
     *          Invoking this method with arguments color=Blue, alternateColor=Red and alternateRatio=1 will draw a line with alternated blue and red pixels (will lead to a purple line)
     *          Invoking this method with arguments color=DarkGreen, alternateColor=Transparent and alternateRatio=3 will draw a line with on dark green dot every 4 pixels (*   *   *)
     */
    void drawVerticalLine(uint16_t x, uint16_t y, uint16_t yPlus, LCD_Color color = LCD_Color::Black, LCD_Color alternateColor = LCD_Color::None, uint8_t alternateRatio = 0);

    /**
     * @brief Draw a plain 1-pixel wide vertical line
     * 
     * @param x The right x position of the line on the LCD
     * @param y The y position of the line on the LCD
     * @param xPlus The length of the line (in pixels), drawn towards the right of the screen, starting from (x;y)
     * @param color An optional 32-bit text color to use when drawing
     * @param alternateColor An optional 32-bit text color that will alternate with @p color when drawing the line (if set to LCD_Color::None, we will draw a solid line)
     * @param alternateRatio The ratio between the number of pixels drawn with @p color and those drawn with @p alternateColor or 0 to only use color
     * 
     * @example Invoking this method with arguments color=Black, alternateColor=None and whatever value in alternateRatio will draw a solid black line
     *          Invoking this method with arguments color=Blue, alternateColor=Red and alternateRatio=0 will draw a solid blue line
     *          Invoking this method with arguments color=Blue, alternateColor=Red and alternateRatio=1 will draw a line with alternated blue and red pixels (will lead to a purple line)
     *          Invoking this method with arguments color=DarkGreen, alternateColor=Transparent and alternateRatio=3 will draw a line with on dark green dot every 4 pixels (*   *   *)
     */
    void drawHorizontalLine(uint16_t x, uint16_t y, uint16_t xPlus, LCD_Color color = LCD_Color::Black, LCD_Color alternateColor = LCD_Color::None, uint8_t alternateRatio = 0);

    /**
     * @brief Draw a character (glyph) on the LCD
     * 
     * @param x The origin (left boundary) of the character on the LCD
     * @param y The origin (top boundary) of the character on the LCD
     * @param c A pointer to a buffer containing the character pixel (one bit per dot)
     * @param fontHeight The height of the character in buffer @p c in pixels
     * @param fontWidth The width of the character in buffer @p c in pixels
     * @param fgColor An optional 32-bit text color to use when drawing
     * @param bgColor An optional 32-bit background color to use when drawing
     */
    void drawGlyph(uint16_t x, uint16_t y, const uint8_t *c, unsigned int fontWidth, unsigned int fontHeight, LCD_Color fgColor = LCD_Color::Black, LCD_Color bgColor = LCD_Color::Transparent);

    /**
     * @brief Draw a character string on the LCD
     * 
     * @param x The origin (left boundary) of the string on the LCD
     * @param y The origin (top boundary) of the string on the LCD
     * @param text A pointer to a '\0'-terminated buffer containing the character string
     * @param fontHeight The height of the character in buffer @p c in pixels
     * @param fontWidth The width of the character in buffer @p c in pixels
     * @param fgColor An optional 32-bit text color to use when drawing
     * @param bgColor An optional 32-bit background color to use when drawing
     */
    void drawText(uint16_t x, uint16_t y, const char *text, unsigned int fontWidth, unsigned int fontHeight, FCharacterToGlyphPtr charToGlyphFunc, LCD_Color fgColor, LCD_Color bgColor);

    /**
     * @brief Draw a full rectangle
     * @param x The origin (left boundary) of the rectangle on the LCD
     * @param y The origin (top boundary) of the rectangle on the LCD
     * @param width The height of the rectangle in pixels
     * @param height The width of the rectangle in pixels
     * @param color The 32-bit color to use when drawing the rectangle
     */
    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, LCD_Color color);

    friend void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi); /* This interrupt hanlder accesses our display state */

private:
    void hdma2dCopyFramebuffer(const void* src, void* dst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize);
    void LL_FillBuffer(uint32_t LayerIndex, void *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex);

private:
    Stm32LcdDriver& operator= (const Stm32LcdDriver&) { return *this; }
    Stm32LcdDriver(const Stm32LcdDriver& other) : hltdc(other.hltdc), hdsi(other.hdsi) {}

    Stm32LcdDriver();
    ~Stm32LcdDriver();

    static Stm32LcdDriver instance;    /*!< Lazy singleton instance */
    volatile LCD_Display_Update_State displayState;  /*!< Used to keep track of state transitions between buffers on LCD driver */
    static void* const draftFramebuffer;    /*!< A pointer to the beginning of the draft frambuffer */
    static void* const finalFramebuffer;    /*!< A pointer to the beginning of the final frambuffer */

public:
    LTDC_HandleTypeDef& hltdc;  /*!< Handle on the LCD/TFT display controller (LTDC), it is unfortunately external to us, defined in stm32469i_discovery_lcd.c */
    DMA2D_HandleTypeDef hdma2d; /*!< Handle on the DMA2D controller */
    DSI_HandleTypeDef& hdsi; /*!< Handle on the Dsiplay Serial Interface (DSI) controller, it is unfortunately external to us, defined in stm32469i_discovery_lcd.c */
};

#endif // _STM32LCDDRIVER_H_
