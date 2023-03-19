/**
 * @file DatasetView.h
 * @brief TIC label decoder
 */
#pragma once
#include <cstddef> // For std::size_t
#define __TIC_TO_STRING__
#ifdef __TIC_TO_STRING__
#include <string>
#endif
#include <stdint.h>

namespace TIC {
class Horodate {
public:
/* Types */
    typedef enum {
        Unknown = 0,
        Winter, /* Winter */
        Summer, /* Summer */
        Malformed,
    } Season;
/* Constants */
    static constexpr std::size_t HORODATE_SIZE = 13; /*!< Size for a horodate tag (in bytes) */

/* Methods */
    Horodate():
    isValid(false),
    season(Season::Unknown),
    degradedTime(true),
    year(0),
    month(0),
    day(0),
    hour(0),
    minute(0),
    second(0) { }

    static Horodate fromLabelBytes(const uint8_t* bytes, unsigned int count);

    std::string toString() const;

public:
/* Attributes */
    bool isValid; /*!< Does this instance contain a valid horodate? */
    Season season; /*!< */
    bool degradedTime; /*!< The horodate was generated by a device with a degraded realtime-clock */
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
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
     * @brief Construct a new TIC::DatasetView object from a dataset buffer
     * 
     * @param datasetBuf A pointer to a byte buffer containing a full dataset
     * @param datasetBufSz The number of valid bytes in @p datasetBuf
     */
    DatasetView(const uint8_t* datasetBuf, std::size_t datasetBufSz);

    /**
     * @brief Does the dataset buffer used at constructor contain a properly formatted dataset?
     * 
     * @return false if the buffer is malformed
     */
    bool isValid() const;

private:
    static uint8_t computeCRC(const uint8_t* bytes, unsigned int count);

public:
/* Attributes */
    DatasetType decodedType;  /*!< What is the resulting type of the parsed dataset? */
    const uint8_t* labelBuffer; /*!< A pointer to the label buffer */
    std::size_t labelSz; /*!< The size of the label in bytes */
    const uint8_t* dataBuffer; /*!< A pointer to the data buffer associated with the label */
    std::size_t dataSz; /*!< The size of the label in bytes */
    Horodate horodate; /*!< The horodate for this dataset */
};
} // namespace TIC