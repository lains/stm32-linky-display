#ifndef _STM32LCDDRIVER_H_
#define _STM32LCDDRIVER_H_

#include "stm32f4xx_hal.h"

extern "C" {
LTDC_HandleTypeDef* get_hltdc(void); // C-linkage exported getter for hltdc handler
DSI_HandleTypeDef* get_hdsi(void); // C-linkage exported getter for hdsi handler
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

    /**
     * @brief Singleton instance getter
     * 
     * @return The singleton instance of this class
     */
    static Stm32LcdDriver& get();

    bool start(DSI_HandleTypeDef* hdsi_eval, LTDC_HandleTypeDef* hltdc_eval);

    void copy_framebuffer(const uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize);

private:
    Stm32LcdDriver& operator= (const Stm32LcdDriver&) { return *this; }
    Stm32LcdDriver(const Stm32LcdDriver&) {}

    Stm32LcdDriver();
    ~Stm32LcdDriver();

    static Stm32LcdDriver instance;    /*!< Lazy singleton instance */
    LCD_Display_Update_State display_state;
public:
    LTDC_HandleTypeDef hltdc_eval;  /*!< Handle on the LCD/TFT display controller (LTDC) */
    DMA2D_HandleTypeDef hdma2d; /*!< Handle on the DMA2D controller */
    DSI_HandleTypeDef hdsi_eval; /*!< Handle on the Dsiplay Serial Interface (DSI) controller */
};

#endif