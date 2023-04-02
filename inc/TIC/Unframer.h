/**
 * @file Unframer.h
 * @brief TIC frame payload extractor
 */
#pragma once
#include <stdint.h>

#define __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__

/* Use catch2 framework for unit testing? https://github.com/catchorg/Catch2 */
namespace TIC {
/**
 * @brief Class to process a continuous stream of bytes and extract TIC frames payload out of this stream
 * 
 * Incoming TIC bytes should be input via the pushBytes() method
 * 
 * Note that onNewFrameBytes() and onFrameComplete() functions referred to below are the function pointer that have been provided as constructor argument.
 * Both may be null, in which case, related function calls won't be performed.
 * 
 * There are two modes of operation, selected by the __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__ preprocessor directive
 * 1. If __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__ is defined, each time new bytes are parsed within a valid frame, they are forwarded to onNewFrameBytes()
 *    When the end-of-frame is met, a call to onFrameComplete() is performed as well.
 * 2. If __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__ is undefined, bytes for each frame will be cached by instances of this class and forwarded as a whole chunk to onNewFrameBytes(), immediately followed by a call to onFrameComplete()
 * 
 * Defining __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__ will be more adapted to embedded systems and avoids having to determine the maximum size of a TIC frame.
 * However, the caller will have to cope with determining datasets that below to the same frame (by implementing a state machine based on calls to onFrameComplete)
 * Undefining __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__ will allocate a fixed sized buffer for storing whole frames. Bytes belonging to the same frame will however be sent altogether.
 * 
 * onNewFrameBytes() will be invoked with 3 arguments (see its the prototype FOnNewFrameBytesFunc)
 * * The first argument is a buffer containing the frame payload (start and end markers are never sent)
 * * The second argument is the number of valid payload bytes in the above buffer
 * * The third argument is a generic context pointer, identical to the parserFuncContext provided as argument to the constructor. It can be used to provide context to onNewFrameBytes() that, in turn, for example, can read data structures from this context pointer.
 * 
 * * onFrameComplete() will be invoked with 1 argument (see its the prototype FOnFrameCompleteFunc)
 * * The first argument is a generic context pointer, identical to the parserFuncContext provided as argument to the constructor. It can be used to provide context to onFrameComplete() that, in turn, for example, can read data structures from this context pointer.
 *
 * Sample code to count TIC frames from a byte stream:
void onNewFrameBytes(const uint8_t* buf, unsigned int cnt, void* context) {
  unsigned int* frameCount = static_cast<unsigned int*>(context);
  (*frameCount)++;
}

unsigned int frameCount = 0;
TIC::Unframer unframer(nullptr, onFrameComplete, &frameCount);
uint8_t inputBytes[]={TIC::Unframer::STX, 0x31, 0x32, TIC::Unframer::ETX, TIC::Unframer::STX, 0x33, 0x34, TIC::Unframer::ETX};
unframer.pushBytes(inputBytes, sizeof(inputBytes));

printf("%u\n", frameCount); // Two TIC frames have been parsed, this line will thus print frameCount that equals 2
 * 
 * @note This class is able to parse historical and standard TIC frames
 */
class Unframer {
public:
/* Types */
    typedef void(*FOnNewFrameBytesFunc)(const uint8_t* buf, unsigned int cnt, void* context); /*!< The prototype of callbacks invoked onNewFrameBytes */
    typedef void(*FOnFrameCompleteFunc)(void* context); /*!< The prototype of callbacks invoked onFrameComplete */

/* Constants */
    static constexpr uint8_t STX = 0x02; /*!< The STX marker */
    static constexpr uint8_t ETX = 0x03; /*!< The ETX marker */
    static constexpr uint8_t START_MARKER = STX; /*!< Frame start marker (STX) */
    static constexpr uint8_t END_MARKER = ETX; /*!< Frame end marker (ETX) */
    static constexpr unsigned int MAX_FRAME_SIZE = 2048; /* Max acceptable TIC frame payload size (excluding STX and ETX markers) */

/* Methods */
    /**
     * @brief Construct a new TIC::Unframer object
     * 
     * @param onNewFrameBytes A FOnNewFrameBytesFunc function to invoke for each byte received in the current TIC frame received
     * @param onFrameComplete A FOnFrameCompleteFunc function to invoke after a full TiC frame has been received
     * @param parserFuncContext A user-defined pointer that will be passed as last argument when invoking onFrameComplete()
     * 
     * @note We are using C-style function pointers here (with data-encapsulation via a context pointer)
     *       This is because we don't have 100% guarantee that exceptions are allowed (especially on embedded targets) and using std::function requires enabling exceptions.
     *       We can still use non-capturing lambdas as function pointer if needed (see https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer)
     */
    Unframer(FOnNewFrameBytesFunc onNewFrameBytes = nullptr, FOnFrameCompleteFunc onFrameComplete = nullptr, void* parserFuncContext = nullptr);

    /**
     * @brief Take new incoming bytes into account
     * 
     * @param buffer The new input TIC bytes
     * @param len The number of bytes to read from @p buffer
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    unsigned int pushBytes(const uint8_t* buffer, unsigned int len);

    /**
     * @brief Are we synchronized with a TIC frame stream
     * 
     * @return true When we have received a STX byte and we are supposedly parsing the TIC frame content
     */
    bool isInSync() const;

    /**
     * @brief Get the max TIC frame size from the recent history buffer
     * 
     * @return The max frame size in bytes
     */
    unsigned int getMaxFrameSizeFromRecentHistory() const;

private:
#ifndef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
    /**
     * @brief Get the remaining free size in our internal buffer currentFrame
     * 
     * @return The number of bytes that we can still store or 0 if the buffer is full
     */
    unsigned int getFreeBytes() const;
#endif

    /**
     * @brief Take new frame bytes into account
     * 
     * @param buffer The buffer to the new frame bytes
     * @param len The number of bytes to read from @p buffer
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    unsigned int processIncomingFrameBytes(const uint8_t* buffer, unsigned int len);
    
    /**
     * @brief Process a current frame that has been completely received (from start to end markers)
     * 
     * @note This method is called internally when the current frame parsing is complete
     *       This means that we have a full frame available in buffer currentFrame for a length of nextWriteInCurrentFrame bytes
     */
    void processCurrentFrame();

/* Attributes */
    bool sync;  /*!< Are we currently in sync? (correct parsing) */
    FOnNewFrameBytesFunc onNewFrameBytes; /*!< Pointer to a function invoked at each new byte block added inside the current frame */
    FOnFrameCompleteFunc onFrameComplete; /*!< Pointer to a function invoked for each full TIC frame received */
    void* parserFuncContext; /*!< A context pointer passed to onNewFrameBytes() and onFrameComplete() at invokation */
#ifndef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
    uint8_t currentFrame[MAX_FRAME_SIZE]; /*!< Our internal accumulating buffer used to store the current frame */
    unsigned int nextWriteInCurrentFrame; /*!< The index of the next bytes to receive in buffer currentFrame */
#endif
};
} // namespace TIC
