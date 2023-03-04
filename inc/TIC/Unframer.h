#pragma once
#include <cstddef> // For std::size_t
#include <stdint.h>
#include "FixedSizeRingBuffer.h"

/* Use catch2 framework for unit testing? https://github.com/catchorg/Catch2 */
namespace TIC {
class Unframer {
public:
/* Types */
    typedef void(*FFrameParserFunc)(const uint8_t* buf, std::size_t cnt, void* context); /*!< The prototype of callbacks invoked onFrameComplete */

/* Constants */
    static constexpr uint8_t TIC_STX = 0x02; /*!< The STX (start of frame) marker */
    static constexpr uint8_t TIC_ETX = 0x03; /*!< The ETX (end of TIC frame) marker */
    static constexpr std::size_t MAX_FRAME_SIZE = 2048; /* Max acceptable TIC frame payload size (excluding STX and ETX markers) */
    static constexpr unsigned int STATS_NB_FRAMES = 128;  /* On how many last frames do we compute statistics */

/* Methods */
    /**
     * @brief Construct a new TIC::Unframer object
     * 
     * @param onFrameComplete A FFrameParserFunc function to invoke for each full TIC frame received
     * @param onFrameCompleteContext A user-defined pointer that will be passed as last argument when invoking onFrameComplete()
     * 
     * @note We are using C-style function pointers here (with data-encapsulation via a context pointer)
     *       This is because we don't have 100% guarantee that exceptions are allowed (especially on embedded targets) and using std::function requires enabling exceptions.
     *       We can still use non-capturing lambdas as function pointer if needed (see https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer)
     */
    Unframer(FFrameParserFunc onFrameComplete = nullptr, void* onFrameCompleteContext = nullptr);

    /**
     * @brief Take new incoming bytes into account
     * 
     * @param buffer The new input TIC bytes
     * @param len The number of bytes to read from @p buffer
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    std::size_t pushBytes(const uint8_t* buffer, std::size_t len);

    /**
     * @brief Are we synchronized with a TIC frame stream
     * 
     * @return true When we have received a STX byte and we are supposedly parsing the TIC frame content
     */
    bool isInSync() const;

    /**
     * @brief Get the max TIC frame size from the recent history buffer
     * 
     * @return std::size_t The max frame size in bytes
     */
    std::size_t getMaxFrameSizeFromRecentHistory() const;

private:
    /**
     * @brief Get the remaining free size in our internal buffer currentFrame
     * 
     * @return std::size_t The number of bytes that we can still store or 0 if the buffer is full
     */
    std::size_t getFreeBytes() const;

    /**
     * @brief Take new frame bytes into account
     * 
     * @param buffer The buffer to the new frame bytes
     * @param len The number of bytes to read from @p buffer
     * @param frameComplete Is the frame complete?
     * @return std::size_t The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    std::size_t processIncomingFrameBytes(const uint8_t* buffer, std::size_t len, bool frameComplete = false);
    
    /**
     * @brief Process a current frame that has been completely received (from start to end markers)
     * 
     * @note This method is called internally when the current frame parsing is complete
     *       This means that we have a full frame available in buffer currentFrame for a length of nextWriteInCurrentFrame bytes
     */
    void processCurrentFrame();

    /**
     * @brief Record a new full frame size in our recent frame size history (for statistics)
     * 
     * @param frameSize The new frame size (in bytes) to record
     */
    void recordFrameSize(unsigned int frameSize); /*!< Record a frame size in our history */

/* Attributes */
    bool sync;  /*!< Are we currently in sync? (correct parsing) */
    FFrameParserFunc onFrameComplete; /*!< A function pointer invoked for each full TIC frame received */
    void* onFrameCompleteContext; /*!< A context pointer passed to onFrameComplete() at invokation */
    FixedSizeRingBuffer<unsigned int, STATS_NB_FRAMES> frameSizeHistory;  /* A rotating buffer containing the history of received TIC frames sizes */
    uint8_t currentFrame[MAX_FRAME_SIZE]; /*!< Our internal accumulating buffer used to store the current frame */
    unsigned int nextWriteInCurrentFrame; /*!< The index of the next bytes to receive in buffer currentFrame */
};
} // namespace TIC