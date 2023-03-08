#include <string.h> // For memset()
#include "TIC/DatasetExtractor.h"

TIC::DatasetExtractor::DatasetExtractor(FDatasetParserFunc onDatasetExtracted, void* onDatasetExtractedContext) :
sync(false),
onDatasetExtracted(onDatasetExtracted),
onDatasetExtractedContext(onDatasetExtractedContext),
nextWriteInCurrentDataset(0) {
    memset(this->currentDataset, 0, MAX_DATASET_SIZE);
}

std::size_t TIC::DatasetExtractor::pushBytes(const uint8_t* buffer, std::size_t len) {
    /* In a TIC frame, TIC labels follow the format:
    [LF]<dataset>[CR]
    Where:
    - LF is the ASCII character 0x0a (line-feed)
    - <dataset> is the TIC dataset we want to extract
    - [CR] is the ASCII character 0x0d (carriage return)

    See https://lucidar.me/fr/home-automation/linky-customer-tele-information/
    */
    if (len == 0)
        return 0;
    std::size_t usedBytes = 0;
    if (!this->sync) {  /* We don't record bytes, we'll just look for a start of dataset */
        uint8_t* firstStartOfDataset = (uint8_t*)(memchr(buffer, TIC::DatasetExtractor::START_MARKER, len));
        if (firstStartOfDataset) {
            this->sync = true;
            std::size_t bytesToSkip =  firstStartOfDataset - buffer;  /* Bytes processed (but ignored) */
            usedBytes = bytesToSkip;
            if (bytesToSkip < len) {  /* we have at least one trailing byte */
                usedBytes++;
                firstStartOfDataset++; /* Skip LF marker (it won't be included inside the buffered dataset) */
                usedBytes += this->pushBytes(firstStartOfDataset, len - usedBytes);  /* Recursively parse the trailing bytes, now that we are in sync */
            }
        }
        else {
            /* Skip all bytes */
            usedBytes = len;
        }
    }
    else {
        /* We are inside a TIC dataset, search for the end of dataset marker */
        uint8_t* endOfDataset = (uint8_t*)(memchr(buffer, TIC::DatasetExtractor::END_MARKER, len)); /* Search for end of dataset */
        if (endOfDataset) {  /* We have an end of dataset marker in the buffer, we can extract the full dataset */
            std::size_t leadingBytesInPreviousDataset = endOfDataset - buffer;
            usedBytes = this->processIncomingDatasetBytes(buffer, leadingBytesInPreviousDataset, true);  /* Copy the buffer up to (but exclusing the end of dataset marker), the dataset is complete */
            this->nextWriteInCurrentDataset = 0; /* Wipe any data in the current dataset, start over */
            leadingBytesInPreviousDataset++; /* Skip the end of dataset marker */
            usedBytes++;
            this->sync = false; /* Consider we are outside of a dataset now */
            if (leadingBytesInPreviousDataset < len) { /* We have at least one byte after the end of dataset marker */
                usedBytes += this->pushBytes(buffer + leadingBytesInPreviousDataset, len - leadingBytesInPreviousDataset); /* Process the trailing bytes (probably the next dataset */
            }
        }
        else { /* No end of dataset marker, copy the whole chunk */
            usedBytes = this->processIncomingDatasetBytes(buffer, len); /* Process these bytes as valid */
        }
    }
    return usedBytes;
}

std::size_t TIC::DatasetExtractor::processIncomingDatasetBytes(const uint8_t* buffer, std::size_t len, bool datasetComplete) {
    std::size_t maxCopy = this->getFreeBytes();
    std::size_t szCopy = len;
    if (szCopy > maxCopy) {  /* currentFrame overflow */
        szCopy = maxCopy; /* FIXME: Error case */
    }
    memcpy(this->currentDataset + nextWriteInCurrentDataset, buffer, szCopy);
    nextWriteInCurrentDataset += szCopy;

    if (datasetComplete) {
        this->processCurrentDataset();
    }
    return szCopy;
}

void TIC::DatasetExtractor::processCurrentDataset() {
    this->onDatasetExtracted(this->currentDataset, this->nextWriteInCurrentDataset, this->onDatasetExtractedContext);
}

bool TIC::DatasetExtractor::isInSync() const {
    return this->sync;
}

std::size_t TIC::DatasetExtractor::getFreeBytes() const {
    return MAX_DATASET_SIZE - this->nextWriteInCurrentDataset;
}

void TIC::DatasetExtractor::reset() {
    this->sync = false;
    memset(this->currentDataset, 0, MAX_DATASET_SIZE);
    this->nextWriteInCurrentDataset = 0;
}

