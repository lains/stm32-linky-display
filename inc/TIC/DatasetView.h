/**
 * @file DatasetView.h
 * @brief TIC label decoder
 */
#pragma once
#include <cstddef> // For std::size_t
#include <stdint.h>

namespace TIC {
class Horodate {
public:
    Horodate(): isValid(false) { }
    static Horodate fromLabelBytes(const uint8_t* bytes, unsigned int count) {
        Horodate result;
        result.isValid = true;
        //FIXME: we should parse the remaining horodate bytes and fill-in the appropriate structure here
        return result;
    }
private:
    bool isValid; /*!< Does this instance contain a valid horodate? */
};

class DatasetView {
public:
/* Types */
    typedef enum {
        Malformed = 0, /* Malformed dataset */
        WrongCRC,
        ValidHistorical, /* Default TIC on newly installed meters */
        ValidStandard,
    } DatasetType;

/* Constants */
    static constexpr uint8_t HT = 0x09; /* Horizontal tab */
    static constexpr uint8_t SP = 0x20; /* Space */

/* Methods */
    /**
     * @brief Construct a new TIC::LabelDecoderView object from a dataset buffer
     * 
     * @param datasetBuf A pointer to a byte buffer containing a full dataset
     * @param datasetBufSz The number of valid bytes in @p datasetBuf
     */
    DatasetView(const uint8_t* datasetBuf, std::size_t datasetBufSz);


    bool isValid() const;

private:
    static uint8_t computeCRC(const uint8_t* bytes, std::size_t count);

/* Attributes */
    DatasetType decodedType;  /*!< What is the resulting type of the parsed dataset? */
    const uint8_t* labelBuffer; /*!< A pointer to the label buffer */
    std::size_t labelSz; /*!< The size of the label in bytes */
    const uint8_t* dataBuffer; /*!< A pointer to the data buffer associated with the label */
    std::size_t dataSz; /*!< The size of the label in bytes */
    Horodate horodate; /*!< The horodate for this dataset */
};
} // namespace TIC