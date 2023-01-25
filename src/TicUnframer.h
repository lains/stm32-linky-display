#pragma once
#include <stdint.h>
#include <string.h> // For memset()
#include <functional>

class TICUnframer {
/* Types */
typedef std::function<void (const uint8_t* buf, size_t cnt)> FFrameParserFunc;

/* Methods */
public:
    const uint8_t TIC_STX = 0x02;
    const uint8_t TIC_ETX = 0x03;

    TICUnframer(FFrameParserFunc onFrameComplete);

    /**
     * @brief Take new incoming bytes into account
     * 
     * @param buffer The new input TIC bytes
     * @param len The number of bytes to read from @p buffer
     * @return size_t The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    size_t pushBytes(uint8_t* buffer, size_t len);

    bool isInSync();

private:
    size_t getFreeBytes();

/* Attributes */
    bool sync;
    uint8_t currentFrame[256];
    unsigned int nextWriteInCurrentFrame;
    FFrameParserFunc onFrameComplete;
};
