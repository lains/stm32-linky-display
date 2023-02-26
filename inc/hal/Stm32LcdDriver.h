#ifndef _STM32LCDDRIVER_H_
#define _STM32LCDDRIVER_H_

#include "stm32f4xx_hal.h"

extern "C" {
LTDC_HandleTypeDef* get_hltdc(void); // C-linkage exported getter for hltdc handler
DSI_HandleTypeDef* get_hdsi(void); // C-linkage exported getter for hdsi handler
void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi);
}

/**
 * @brief Serial link communication class (singleton)
 */
class Stm32LcdDriver {
public:
    typedef enum {
        SwitchToDraftIsPending = 0,
        DisplayingDraft,
        CopyingDraftToFinalIsPending,
        CopyDraftToFinalIsDone, /* Unused, would be nice not to poll on DMA2D while copying */
        SwitchToFinalIsPending,
        DisplayingFinal
    } LCD_Display_Update_State;

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

    friend void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef *hdsi); /* This interrupt hanlder accesses our display state */

private:
    void hdma2dCopyFramebuffer(const uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize);

private:
    Stm32LcdDriver& operator= (const Stm32LcdDriver&) { return *this; }
    Stm32LcdDriver(const Stm32LcdDriver& other) : hltdc(other.hltdc), hdsi(other.hdsi) {}

    Stm32LcdDriver();
    ~Stm32LcdDriver();

    static Stm32LcdDriver instance;    /*!< Lazy singleton instance */
public: // For debug only
    LCD_Display_Update_State display_state;
public:
    LTDC_HandleTypeDef& hltdc;  /*!< Handle on the LCD/TFT display controller (LTDC), it is unfortunately external to us, defined in stm32469i_discovery_lcd.c */
    DMA2D_HandleTypeDef hdma2d; /*!< Handle on the DMA2D controller */
    DSI_HandleTypeDef& hdsi; /*!< Handle on the Dsiplay Serial Interface (DSI) controller, it is unfortunately external to us, defined in stm32469i_discovery_lcd.c */
};

#endif
