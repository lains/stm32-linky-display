#pragma once
#include <cstddef> // For std::size_t
#include <stdint.h>
#include <string.h> // For memset()
#include <functional>
#include "FixedSizeRingBuffer.h"

class TICUnframer {
public:
/* Types */
    typedef std::function<void (const uint8_t* buf, std::size_t cnt)> FFrameParserFunc;

/* Constants */
    static constexpr uint8_t TIC_STX = 0x02;
    static constexpr uint8_t TIC_ETX = 0x03;
    static constexpr std::size_t MAX_FRAME_SIZE = 512; /* Make acceptable TIC frame payload size (excluding STX and ETX markers) */
    static constexpr unsigned int STATS_NB_FRAMES = 128;  /* On how many last frames do we compute statistics */

/* Methods */
    TICUnframer(FFrameParserFunc onFrameComplete);

    /**
     * @brief Take new incoming bytes into account
     * 
     * @param buffer The new input TIC bytes
     * @param len The number of bytes to read from @p buffer
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    std::size_t pushBytes(const uint8_t* buffer, std::size_t len);

    bool isInSync();

    std::size_t getMaxFrameSizeFromRecentHistory() const;

private:
    std::size_t getFreeBytes() const;
    void processCurrentFrame(); /*!< Method called when the current frame parsing is complete */
    void recordFrameSize(unsigned int frameSize); /*!< Record a frame size in our history */

/* Attributes */
    bool sync;
    FFrameParserFunc onFrameComplete;
    FixedSizeRingBuffer<unsigned int, STATS_NB_FRAMES> frameSizeHistory;
    unsigned int nextWriteInCurrentFrame;
    uint8_t currentFrame[MAX_FRAME_SIZE];
};
