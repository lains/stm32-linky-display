#pragma once
#include <cstddef> // For std::size_t
#include <stdint.h>
#include <string.h> // For memset()
#include "FixedSizeRingBuffer.h"

/* Use catch2 framework for unit testing? https://github.com/catchorg/Catch2 */
class TICUnframer {
public:
/* Types */
    typedef void(*FFrameParserFunc)(const uint8_t* buf, std::size_t cnt, void* context);

/* Constants */
    static constexpr uint8_t TIC_STX = 0x02;
    static constexpr uint8_t TIC_ETX = 0x03;
    static constexpr std::size_t MAX_FRAME_SIZE = 512; /* Make acceptable TIC frame payload size (excluding STX and ETX markers) */
    static constexpr unsigned int STATS_NB_FRAMES = 128;  /* On how many last frames do we compute statistics */

/* Methods */
    /**
     * @brief Construct a new TICUnframer object
     * 
     * @param onFrameComplete A FFrameParserFunc function to invoke for each full TIC frame received
     * @param onFrameCompleteContext A user-defined pointer that will be passed as last argument when invoking onFrameComplete()
     * 
     * @note We are using C-style function pointers here (with data-encapsulation via a context pointer)
     *       This is because we don't have 100% guarantee that exceptions are allowed (especially on embedded targets) and using std::function requires enabling exceptions.
     *       We can still use non-capturing lambdas as function pointer if needed (see https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer)
     */
    TICUnframer(FFrameParserFunc onFrameComplete = nullptr, void* onFrameCompleteContext = nullptr);

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
    FFrameParserFunc onFrameComplete; /*!< A function pointer invoked for each full TIC frame received */
    void* onFrameCompleteContext; /*!< A context pointer passed to onFrameComplete() at invokation */
    FixedSizeRingBuffer<unsigned int, STATS_NB_FRAMES> frameSizeHistory;  /* A rotating buffer containing the history of received TIC frames sizes */
    uint8_t currentFrame[MAX_FRAME_SIZE]; /*!< Our internal accumulating buffer */
    unsigned int nextWriteInCurrentFrame; /*!< The index of the next bytes to receive in buffer currentFrame */
};
