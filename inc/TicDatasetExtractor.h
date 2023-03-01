#pragma once
#include <cstddef> // For std::size_t
#include <stdint.h>

class TICDatasetExtractor {
public:
/* Types */
    typedef void(*FDatasetParserFunc)(const uint8_t* buf, std::size_t cnt, void* context);
    
    typedef enum {
        Historical = 0, /* Default TIC on newly installed meters */
        Standard,
    } TIC_Type;

/* Constants */
    static constexpr uint8_t LF = 0x0a; /* Line feed */
    static constexpr uint8_t CR = 0x0d; /* Carriage return */
    static constexpr std::size_t MAX_DATASET_SIZE = 128; /* Max size to store a dataset */

/* Methods */
    /**
     * @brief Construct a new TICDatasetExtractor object
     * 
     * @param onDatasetExtracted A FFrameParserFunc function to invoke for each valid TIC dataset extracted
     * @param onDatasetExtractedContext A user-defined pointer that will be passed as last argument when invoking onDatasetExtracted()
     * 
     * @note We are using C-style function pointers here (with data-encapsulation via a context pointer)
     *       This is because we don't have 100% guarantee that exceptions are allowed (especially on embedded targets) and using std::function requires enabling exceptions.
     *       We can still use non-capturing lambdas as function pointer if needed (see https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer)
     */
    TICDatasetExtractor(FDatasetParserFunc onDatasetExtracted = nullptr, void* onDatasetExtractedContext = nullptr);

    /**
     * @brief Reset the label parser state, this expecting a new dataset starting with the next input bytes
     */
    void reset();

    bool isInSync() const;

    /**
     * @brief Take new incoming bytes into account
     * 
     * @param buffer The new input TIC bytes
     * @param len The number of bytes to read from @p buffer
     * @return The number of bytes used from buffer (if it is <len, some bytes could not be processed due to a full buffer. This is an error case)
     */
    std::size_t pushBytes(const uint8_t* buffer, std::size_t len);

private:
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
    
    void processCurrentDataset(); /*!< Method called when the current frame parsing is complete */

/* Attributes */
    bool sync;  /*!< Are we currently in sync? (correct parsing) */
    FDatasetParserFunc onDatasetExtracted; /*!< A function pointer invoked for each valid TIC dataset extracted */
    void* onDatasetExtractedContext; /*!< A context pointer passed to onDatasetExtracted() at invokation */
    uint8_t currentDataset[MAX_DATASET_SIZE]; /*!< Our internal accumulating buffer used to store the current dataset */
    unsigned int nextWriteInCurrentDataset; /*!< The index of the next bytes to write in buffer currentDataset */
};
