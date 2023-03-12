#include <string.h> // For memset()
#include "TIC/DatasetView.h"

TIC::DatasetView::DatasetView(const uint8_t* datasetBuf, std::size_t datasetBufSz) :
decodedType(TIC::DatasetView::DatasetType::Malformed),
labelBuffer(datasetBuf),
labelSz(datasetBufSz),
dataBuffer(datasetBuf),
dataSz(datasetBufSz),
horodate() {
    /* In a TIC frame, TIC labels follow the format:
    [LF]<label>[Sp]123456789012[Sp][CSUM][CR]
    Where LF is the ASCII character 0x0a (line-feed)
    <label> is the TIC label (between 1 and 8 ascii characters)
    Where [Sp] is a delimiter that can be 0x20 (space) or 0x09 (horizontal tab)
    <value> is the value associated wit the label (between 1 and 12 characters)
    <CSUM> is a one character checksum
    Where [CR] is the ASCII character 0x0d (carriage return)

    The max storage size for one dataset string (label+'\0'+value+'\'0') is thus ?

    See https://lucidar.me/fr/home-automation/linky-customer-tele-information/
    */
    
    bool isTicStandard = true;

    if (datasetBufSz < 5) { /* The minimum size for a valid dataset is 5 bytes (1 byte label + 1 delimiter + 1 byte value + 1 delimiter + 1 byte CRC) */
        return; /* isValid is false, invalid dataset */
    }

    if (*datasetBuf == 0x0A) { /* If we get a LF as first byte, assume this is the dataset start marker, and skip it*/
        datasetBuf++;
        datasetBufSz--;
    }

    uint8_t crcByte = *(datasetBuf + datasetBufSz - 1);  /* Last character of the dataset is the CRC */
    datasetBufSz--;
    if (crcByte == 0x0D) { /* If we get a CR as last byte, assume this is the dataset end marker, skip it, and use the previous byte instead as CRC */
        crcByte = *(datasetBuf + datasetBufSz - 1);
        datasetBufSz--;
    }

    /* Once checksum byte has been removed, we have a delimiter just at the end */
    uint8_t delimiter = *(datasetBuf + datasetBufSz - 1);
    if (delimiter == TIC::DatasetView::HT) {
        /* Getting a HT here means this is a standard TIC dataset */
        isTicStandard = true;
    }
    else if (delimiter == TIC::DatasetView::SP) {
        isTicStandard = false;
    }
    else {
        return; /* isValid is false, invalid dataset */
    }
    datasetBufSz--; /* Reduce the dataset buffer to exclude the last delimiter */

    /* In [datasetBuf;datasetBuf+datasetBufSz[, we now have the byte sequence on which CRC applies */
    uint8_t computedCrc = this->computeCRC(datasetBuf, datasetBufSz);
    this->labelBuffer = datasetBuf; /* Initially conside that the remaining bytes are one full label */
    this->labelSz = datasetBufSz;
    this->dataBuffer = datasetBuf;
    this->dataSz = 0;

    if (computedCrc != crcByte) {
        this->decodedType = TIC::DatasetView::DatasetType::WrongCRC;
        this->labelSz = 0;
        this->dataSz = 0;
        return; 
    }

    /* Now, in the frame, we have a label+delim+optional(horodate+delim)+data*/
    uint8_t* datasetDelimFound = (uint8_t*)(memchr(datasetBuf, delimiter, datasetBufSz));

    if (datasetDelimFound == nullptr) { /* We expect at least one delimiter */
        this->decodedType = TIC::DatasetView::DatasetType::Malformed;
        this->labelSz = 0;
        this->dataSz = 0;
        return; /* Invalid dataset */
    }

    {
        uint8_t* trailingBytes = datasetDelimFound + 1; /* Skip the delimiter, point right after*/
        this->labelSz = datasetDelimFound - datasetBuf;
        //this->labelBuffer = datasetBuf; /* Already set this way a few lines above, only the size has to be adjusted */

        /* labelBuffer and labelBufferSz are now valid and point to the label part of the dataset */
        if (trailingBytes >= datasetBuf + datasetBufSz) {    /* The first trailing byte is not within the buffer boundary */
            this->decodedType = TIC::DatasetView::DatasetType::Malformed;
            this->labelSz = 0;
            this->dataSz = 0;
            return; /* Invalid dataset */
        }

        datasetBufSz -= trailingBytes - datasetBuf; /* Adjust the dataset buffer to skip the leading found label + delim */
        datasetBuf = trailingBytes;
    } // trailingBytes goes out of scope here...

    /* Now, in the frame, we have either a horodate+delim+data or just data */
    datasetDelimFound = (uint8_t*)(memchr(datasetBuf, delimiter, datasetBufSz));

    if (datasetDelimFound == nullptr) { /* No more delimiter, so no horodate */
        this->dataBuffer = datasetBuf;
        this->dataSz = datasetBufSz;
        /* this->horodate has been constructed by default, and is thus invalid (there is no horodate) */
        /* We have a valid dataset without horodate, continue through */
    }
    else {
        uint8_t* trailingBytes = datasetDelimFound + 1; /* Skip the delimiter, point right after*/
        std::size_t horodateSz = datasetDelimFound - datasetBuf;

        this->horodate = Horodate::fromLabelBytes(datasetBuf, horodateSz);

        if (trailingBytes >= datasetBuf + datasetBufSz) {    /* The first trailing byte is not within the buffer boundary */
            this->decodedType = TIC::DatasetView::DatasetType::Malformed;
            this->labelSz = 0;
            this->dataSz = 0;
            return; /* Invalid dataset */
        }
        this->dataSz -= trailingBytes - datasetBuf; /* Skip the horodate found + delim */
        this->dataBuffer = trailingBytes;
        /* We have a valid dataset with horodate, continue through */
    }
    this->decodedType = isTicStandard ? TIC::DatasetView::DatasetType::ValidStandard : TIC::DatasetView::DatasetType::ValidHistorical;
}

uint8_t TIC::DatasetView::computeCRC(const uint8_t* bytes, unsigned int count) {
    uint8_t S1 = 0;
    while (count--) {
        S1 += *bytes;
        bytes++;
    }
    return (S1 & 0x3f) + 0x20;
}

bool TIC::DatasetView::isValid() const {
    return (this->decodedType == TIC::DatasetView::DatasetType::ValidHistorical || this->decodedType == TIC::DatasetView::DatasetType::ValidStandard);
}