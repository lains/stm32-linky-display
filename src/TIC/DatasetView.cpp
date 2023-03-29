#include <string.h> // For memset()
#include "TIC/DatasetView.h"

TIC::Horodate TIC::Horodate::fromLabelBytes(const uint8_t* bytes, unsigned int count) {
    TIC::Horodate result;
    if (count != TIC::Horodate::HORODATE_SIZE) {
        return result;
    }
    result.isValid = true;
    switch(bytes[0]) {  /* First character is the season character */
    case 'H':
        result.degradedTime = false;
        result.season = TIC::Horodate::Season::Winter;
        break;
    case 'h':
        result.degradedTime = true;
        result.season = TIC::Horodate::Season::Winter;
        break;
    case 'E':
        result.degradedTime = false;
        result.season = TIC::Horodate::Season::Summer;
        break;
    case 'e':
        result.degradedTime = true;
        result.season = TIC::Horodate::Season::Summer;
        break;
    case ' ':
        result.degradedTime = false;
        result.season = TIC::Horodate::Season::Unknown;
        break;
    default:
        result.season = TIC::Horodate::Season::Malformed;
        result.isValid = false;
    }

    for (unsigned int idx = 1; idx < count; idx++) {
        if (bytes[idx] < '0' || bytes[idx] > '9') { /* Only digits allowed in horodate value */
            result.isValid = false;
            return result;
        }
    }

    result.year = 2000 + (bytes[1] - '0') * 10 + (bytes[2] - '0');

    result.month = (bytes[3] - '0') * 10 + (bytes[4] - '0');
    if (result.month > 12 || result.month == 0) { /* Invalid */
        result.isValid = false;
    }

    result.day = (bytes[5] - '0') * 10 + (bytes[6] - '0');
    if (result.day > 31 || result.day == 0) { /* Invalid */
        result.isValid = false;
    }

    result.hour = (bytes[7] - '0') * 10 + (bytes[8] - '0');
    if (result.hour > 24) { /* Invalid */
        result.isValid = false;
    }

    result.minute = (bytes[9] - '0') * 10 + (bytes[10] - '0');
    if (result.minute >= 60) { /* Invalid */
        result.isValid = false;
    }

    result.second = (bytes[11] - '0') * 10 + (bytes[12] - '0');
    if (result.second >= 60) { /* Invalid */
        result.isValid = false;
    }

    return result;
}

#ifdef __TIC_LIB_USE_STD_STRING__
std::string TIC::Horodate::toString() const {
    if (!this->isValid) {
        return "Invalid horodate";
    }
    std::string result;
    result += '0' + ((this->year / 1000) % 10);
    result += '0' + ((this->year / 100)  % 10);
    result += '0' + ((this->year / 10)   % 10);
    result += '0' + ((this->year)        % 10);
    result += '/';
    result += '0' + ((this->month / 10)  % 10);
    result += '0' + ((this->month)       % 10);
    result += '/';
    result += '0' + ((this->day / 10)    % 10);
    result += '0' + ((this->day)         % 10);
    result += ' ';
    result += '0' + ((this->hour / 10)   % 10);
    result += '0' + ((this->hour)        % 10);
    result += ':';
    result += '0' + ((this->minute / 10) % 10);
    result += '0' + ((this->minute)      % 10);
    result += ':';
    result += '0' + ((this->second / 10) % 10);
    result += '0' + ((this->second)      % 10);

    if (this->season == Season::Winter) {
        result += " (winter)";
    }
    else if (this->season == Season::Winter) {
        result += " (summer)";
    }
    return result;
}
#endif // __TIC_LIB_USE_STD_STRING__

TIC::DatasetView::DatasetView(const uint8_t* datasetBuf, unsigned int datasetBufSz) :
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
    
    bool isTicStandard = false;

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

    if (!isTicStandard) { /* In historical TIC, the delimiter just before CRC byte is not included */
        datasetBufSz--; /* Reduce the dataset buffer to exclude the last delimiter */
    }
    /* In [datasetBuf;datasetBuf+datasetBufSz[, we now have the byte sequence on which CRC applies */
    uint8_t computedCrc = this->computeCRC(datasetBuf, datasetBufSz);

    if (isTicStandard) { /* In standard TIC, the delimiter just before CRC byte has been included in the CRC computation above, now remove it */
        datasetBufSz--; /* Reduce the dataset buffer to exclude the last delimiter */
    }

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

    if (datasetDelimFound != nullptr) { /* There is another delimiter further away, so horodate is included */
        unsigned int horodateSz = datasetDelimFound - datasetBuf;

        this->horodate = Horodate::fromLabelBytes(datasetBuf, horodateSz);

        datasetBuf = datasetDelimFound; /* Point to the delimiter just after horodate */
        datasetBuf++;

        /* TODO: Debug mode-only assert: datasetBuf should not point outside of the buffer */
        /* TODO: Debug mode-only assert: datasetBufSz should be >= horodateSz+1 */
        datasetBufSz -= horodateSz; /* The horodate is not in the buffer anymore */
        datasetBufSz--; /* Nor the delimiter found*/
    }
    else {
        /* this->horodate has been constructed by default, and is thus invalid (there is no horodate) */
        /* We have a valid dataset without horodate, continue through */
    }

    this->dataSz = datasetBufSz; /* Skip the horodate found + delim */
    this->dataBuffer = datasetBuf;

    this->decodedType = isTicStandard ? TIC::DatasetView::DatasetType::ValidStandard : TIC::DatasetView::DatasetType::ValidHistorical;
}

bool TIC::DatasetView::isValid() const {
    return (this->decodedType == TIC::DatasetView::DatasetType::ValidHistorical || this->decodedType == TIC::DatasetView::DatasetType::ValidStandard);
}

uint8_t TIC::DatasetView::computeCRC(const uint8_t* bytes, unsigned int count) {
    uint8_t S1 = 0;
    while (count--) {
        S1 += *bytes;
        bytes++;
    }
    return (S1 & 0x3f) + 0x20;
}

uint32_t TIC::DatasetView::uint32FromValueBuffer(const uint8_t* buf, unsigned int cnt) {
    uint32_t result = 0;
    for (unsigned int idx = 0; idx < cnt; idx++) {
        uint8_t digit = buf[idx];
        if (digit < '0' || digit > '9') {   /* Invalid decimal value */
            return -1;
        }
        if (result > ((uint32_t)-1 / 10)) { /* Failsafe: multiplication by 10 would overflow uint32_t */
            return -1;
        }
        result *= 10;
        if (result > (uint32_t)-1 - (digit - '0')) {    /* Failsafe: addition of unit would overflow uint32_t */
            return -1;
        }
        result += (digit - '0');    /* Take this digit into account */
    }
    return result;
}
