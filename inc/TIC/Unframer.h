/**
 * @file Unframer.h
 * @brief TIC frame payload extractor
 */
#pragma once
#include <cstddef> // For std::size_t
#include <stdint.h>
#include "FixedSizeRingBuffer.h"

/* Use catch2 framework for unit testing? https://github.com/catchorg/Catch2 */
namespace TIC {
/**
 * @brief Class to process a continuous stream of bytes and extract TIC frames payload out of this stream
 * 
 * Incoming TiC bytes should be input via the pushBytes() method
 * When a TIC frame payload is correctly parsed, it will be sent to the onFrameComplete() function provided as argument to the constructor.
 * That function will be invoked with 3 arguments matching with the prototype FFrameParserFunc
 * * The first argument is a buffer containing the frame payload (start and end markers are excluded)
 * * The second argument is the number of valid payload bytes in the above buffer
 * * The third argument is a generic context pointer, identical to the onFrameCompleteContext provided as argument to the constructor. It can be used to provide context to onFrameComplete() that, in turn, for example, can read data structures from this context pointer.
 * 
 * Sample code to count TIC frames from a byte stream:
 * unsigned int frameCount = 0;
 * void onFrameComplete(const uint8_t* buf, std::size_t cnt, void* context) {
 *   unsigned int& frameCount = static_cast<unsigned int*>(context);
 *   frameCount++;
 * }
 * TIC::Unframer unframer(onFrameComplete, &frameCount);
 * 
 * unframer.pushBytes({TIC::Unframer::STX, 0x01, 0x02, TIC::Unframer::ETX, TIC::Unframer::STX, 0x03, 0x04; TIC::Unframer::ETX});
 * 
 * // This results in two TiC frames being parsed, and frameCount now equals 2
 * 
 * @note This class is able to parse historical and standard TIC frames
 */
class Unframer {
public:
/* Types */
    typedef void(*FFrameParserFunc)(const uint8_t* buf, std::size_t cnt, void* context); /*!< The prototype of callbacks invoked onFrameComplete */

/* Constants */
    static constexpr uint8_t STX = 0x02; /*!< The STX marker */
    static constexpr uint8_t ETX = 0x03; /*!< The ETX marker */
    static constexpr uint8_t START_MARKER = STX; /*!< Frame start marker (STX) */
    static constexpr uint8_t END_MARKER = ETX; /*!< Frame end marker (ETX) */
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