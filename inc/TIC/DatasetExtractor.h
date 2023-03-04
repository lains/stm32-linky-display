#pragma once
#include <cstddef> // For std::size_t
#include <stdint.h>

namespace TIC {
class DatasetExtractor {
public:
/* Types */
    typedef void(*FDatasetParserFunc)(const uint8_t* buf, std::size_t cnt, void* context); /*!< The prototype of callbacks invoked onDatasetExtracted */
    
    typedef enum {
        Historical = 0, /* Default TIC on newly installed meters */
        Standard,
    } TIC_Type;

/* Constants */
    static constexpr uint8_t LF = 0x0a; /*!< Dataset start marker (line feed) */
    static constexpr uint8_t CR = 0x0d; /*!< Dataset end marker (carriage return) */
    static constexpr std::size_t MAX_DATASET_SIZE = 128; /*!< Max size for a dataset storage (in bytes) */

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
     */
    void reset();

    /**
     * @brief Take new incoming bytes into account
     * 
     * @param buffer The new input TIC bytes
     * @param len The number of bytes to read from @p buffer
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    std::size_t pushBytes(const uint8_t* buffer, std::size_t len);

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
     * @return std::size_t The number of bytes that we can still store or 0 if the buffer is full
     */
    std::size_t getFreeBytes() const;

    /**
     * @brief Take new dataset bytes into account
     * 
     * @param buffer The buffer to the new dataset bytes
     * @param len The number of bytes to read from @p buffer
     * @param datasetComplete Is the dataset complete?
     * @return std::size_t The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    std::size_t processIncomingDatasetBytes(const uint8_t* buffer, std::size_t len, bool datasetComplete = false);
    
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