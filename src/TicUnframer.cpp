#include "TicUnframer.h"

TICUnframer::TICUnframer(FFrameParserFunc onFrameComplete) :
sync(false),
nextWriteInCurrentFrame(0),
onFrameComplete(onFrameComplete) {
    memset(currentFrame, 0, sizeof(currentFrame));
}

size_t TICUnframer::pushBytes(uint8_t* buffer, size_t len) {
    size_t usedBytes = 0;
    if (!this->sync) {  /* We don't record bytes, we'll just look for a start of frame */
        uint8_t* firstStx = (uint8_t*)(memchr(buffer, TICUnframer::TIC_STX, len));
        if (firstStx) {
            this->sync = true;
            size_t bytesToSkip =  firstStx - buffer;  /* Bytes processed (but ignored) */
            usedBytes = bytesToSkip;
            if (bytesToSkip < len) {  /* we have at least one trailing byte */
                usedBytes++;
                firstStx++; /* Skip STX marker (it won't be included inside the buffered frame) */
                usedBytes += this->pushBytes(firstStx, len - usedBytes);  /* Recursively parse the trailing bytes, now that we are in sync */
            }
        }
        else {
            /* Skip all bytes */
            usedBytes = len;
        }
    }
    else {
        /* We are inside a TIC frame */
        uint8_t* etx = (uint8_t*)(memchr(buffer, TICUnframer::TIC_ETX, len)); /* Search for end of frame */
        if (etx) {  /* We have an ETX in the buffer, we can extract the full frame */
            size_t leadingBytesInPreviousFrame = etx - buffer;
            usedBytes = this->pushBytes(buffer, leadingBytesInPreviousFrame); /* Copy the buffer up to (but exclusing the ETX marker) */
            this->onFrameComplete(this->currentFrame, this->nextWriteInCurrentFrame);
            if (leadingBytesInPreviousFrame < len) { /* We have at least one byte after ETX */
                leadingBytesInPreviousFrame++; /* Skip the ETX marker */
                usedBytes++;
                this->sync = false; /* Consider we are outside of a frame now */
                usedBytes += this->pushBytes(buffer + leadingBytesInPreviousFrame, len - leadingBytesInPreviousFrame); /* Process the trailing bytes (probably the next frame, starting with STX) */
            }
        }
        else { /* No ETX, copy the whole chunk */
            size_t maxCopy = this->getFreeBytes();
            size_t szCopy = len;
            if (szCopy > maxCopy) {  /* currentFrame overflow */
                szCopy = maxCopy;
            }
            memcpy(this->currentFrame + nextWriteInCurrentFrame, buffer, szCopy);
            nextWriteInCurrentFrame += szCopy;
            usedBytes = szCopy;
        }
    }
    return usedBytes;
}

bool TICUnframer::isInSync() {
    return this->sync;
}

size_t TICUnframer::getFreeBytes() {
    return sizeof(currentFrame) - nextWriteInCurrentFrame;
}
