/**
 * @file DatasetExtractor.h
 * @brief TIC dataset sequence extractor
 */
#pragma once
#include <stdint.h>

namespace TIC {
/**
 * @brief Class to process a continuous stream of bytes and extract TIC datasets out of this stream
 * 
 * Incoming TiC bytes should be input via the pushBytes() method
 * When a TIC dataset payload is correctly parsed, it will be sent to the onDatasetExtracted() function provided as argument to the constructor.
 * That function will be invoked with 3 arguments matching with the prototype FDatasetParserFunc
 * * The first argument is a buffer containing the dataset payload (start and end markers are excluded)
 * * The second argument is the number of valid payload bytes in the above buffer
 * * The third argument is a generic context pointer, identical to the onDatasetExtractedContext provided as argument to the constructor. It can be used to provide context to onDatasetExtracted() that, in turn, for example, can read data structures from this context pointer.
 * 
 * Sample code to count al TIC datasets from a TIC byte stream:

void onDatasetExtracted(const uint8_t* buf, unsigned int cnt, void* context) {
  unsigned int* datasetCount = static_cast<unsigned int*>(context);
  (*datasetCount)++;
}
void onFrameComplete(const uint8_t* buf, unsigned int cnt, void* context) {
  TIC::DatasetExtractor* datasetExtractor = static_cast<TIC::DatasetExtractor*>(context);
  datasetExtractor->reset();
  datasetExtractor->pushBytes(buf, cnt);
}

unsigned int datasetCount = 0;
TIC::DatasetExtractor datasetExtractor(onDatasetExtracted, &datasetCount);
TIC::Unframer unframer(onFrameComplete, &datasetExtractor);
uint8_t inputBytes[]={TIC::Unframer::STX, TIC::DatasetExtractor::START_MARKER, 0x31, 0x32, TIC::DatasetExtractor::END_MARKER, TIC::DatasetExtractor::START_MARKER, 0x33, 0x34, TIC::DatasetExtractor::END_MARKER, TIC::Unframer::ETX};
unframer.pushBytes(inputBytes, sizeof(inputBytes));
	
printf("%u\n", datasetCount); // Two TIC datasets have been parsed inside one signal frame, this line will thus print datasetCount that equals 2
 * 
 * @note This class is able to parse historical and standard TIC datasets
 * 
 * @warning At the beginning of each TIC frame that contains the byte stream fed into this class, the reset() method should be invoked to discard any previously stored incoming bytes and start from scratch
 */

class DatasetExtractor {
public:
/* Types */
    typedef void(*FDatasetParserFunc)(const uint8_t* buf, unsigned int cnt, void* context); /*!< The prototype of callbacks invoked onDatasetExtracted */
    
/* Constants */
    static constexpr uint8_t LF = 0x0a;
    static constexpr uint8_t CR = 0x0d;
    static constexpr uint8_t START_MARKER = LF; /*!< Dataset start marker (line feed) */
    static constexpr uint8_t END_MARKER = CR; /*!< Dataset end marker (carriage return) */
    static constexpr unsigned int MAX_DATASET_SIZE = 128; /*!< Max size for a dataset storage (in bytes) */

/* Methods */
    /**
     * @brief Construct a new TIC::DatasetExtractor object
     * 
     * @param onDatasetExtracted A FFrameParserFunc function to invoke for each valid TIC dataset extracted
     * @param onDatasetExtractedContext A user-defined pointer that will be passed as last argument when invoking onDatasetExtracted()
     * 
     * @note We are using C-style function pointers here (with data-encapsulation via a context pointer)
     *       This is because we don't have 100% guarantee that exceptions are allowed (especially on embedded targets) and using std::function requires enabling exceptions.
     *       We can still use non-capturing lambdas as function pointer if needed (see https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer)
     */
    DatasetExtractor(FDatasetParserFunc onDatasetExtracted = nullptr, void* onDatasetExtractedContext = nullptr);

    /**
     * @brief Reset the label parser state
     * 
     * @note After calling this methods, we will expecting the next input bytes to correspond to the start of a new dataset
     *       Calling this method is required between two frames to avoid concatenation of a previous frame's unterminated dataset with the next frame's dataset
     *       Historical TIC frames pose a problem here because at the very beginning of a frame, we get an leading end of dataset marker, and at the very end, a trailing start of dataset marker
     *       so concatenating these two extra dataset markers across a frame boundary will led to a concatenated empty dataset (containing only 2 bytes: start, then end markers).
     *       By resetting, we get rid of the trailing byte (frame being over, it is wiped away)
     */
    void reset();

    /**
     * @brief Take new incoming bytes into account
     * 
     * @param buffer The new input TIC bytes
     * @param len The number of bytes to read from @p buffer
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    unsigned int pushBytes(const uint8_t* buffer, unsigned int len);

    /**
     * @brief Are we synchronized with a TIC dataset
     * 
     * @return true When we have received a dataset start marker and we are supposedly parsing the TIC dataset content
     */
    bool isInSync() const;

private:
    /**
     * @brief Get the remaining free size in our internal buffer currentDataset
     * 
     * @return The number of bytes that we can still store or 0 if the buffer is full
     */
    unsigned int getFreeBytes() const;

    /**
     * @brief Take new dataset bytes into account
     * 
     * @param buffer The buffer to the new dataset bytes
     * @param len The number of bytes to read from @p buffer
     * @param datasetComplete Is the dataset complete?
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    unsigned int processIncomingDatasetBytes(const uint8_t* buffer, unsigned int len, bool datasetComplete = false);
    
    /**
     * @brief Process a current dataset that has been completely received (from start to end markers)
     * 
     * @note This method is called internally when the current dataset parsing is complete
     *       This means that we have a full dataset available in buffer currentDataset for a length of nextWriteInCurrentDataset bytes
     */
    void processCurrentDataset();

/* Attributes */
    bool sync;  /*!< Are we currently in sync? (correct parsing) */
    FDatasetParserFunc onDatasetExtracted; /*!< A function pointer invoked for each valid TIC dataset extracted */
    void* onDatasetExtractedContext; /*!< A context pointer passed to onDatasetExtracted() at invokation */
    uint8_t currentDataset[MAX_DATASET_SIZE]; /*!< Our internal accumulating buffer used to store the current dataset */
    unsigned int nextWriteInCurrentDataset; /*!< The index of the next bytes to write in buffer currentDataset */
};
} // namespace TIC