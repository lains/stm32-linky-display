#include "TicUnframer.h"



TICUnframer::TICUnframer(FFrameParserFunc onFrameComplete) :
sync(false),
onFrameComplete(onFrameComplete),
frameSizeHistory(),
nextWriteInCurrentFrame(0) {
    memset(this->currentFrame, 0, MAX_FRAME_SIZE);
}

/* TODO: check result implementation using either:
https://github.com/oktal/result
Or (seems better) https://github.com/bitwizeshift/result */


std::size_t TICUnframer::pushBytes(const uint8_t* buffer, std::size_t len) {
    std::size_t usedBytes = 0;
    if (!this->sync) {  /* We don't record bytes, we'll just look for a start of frame */
        uint8_t* firstStx = (uint8_t*)(memchr(buffer, TICUnframer::TIC_STX, len));
        if (firstStx) {
            this->sync = true;
            std::size_t bytesToSkip =  firstStx - buffer;  /* Bytes processed (but ignored) */
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
            std::size_t leadingBytesInPreviousFrame = etx - buffer;
            usedBytes = this->pushBytes(buffer, leadingBytesInPreviousFrame); /* Copy the buffer up to (but exclusing the ETX marker) */
            this->processCurrentFrame();
            this->nextWriteInCurrentFrame = 0; /* Wipe any data in the current frame, start over */
            leadingBytesInPreviousFrame++; /* Skip the ETX marker */
            usedBytes++;
            this->sync = false; /* Consider we are outside of a frame now */
            if (leadingBytesInPreviousFrame < len) { /* We have at least one byte after the frame ETX */
                usedBytes += this->pushBytes(buffer + leadingBytesInPreviousFrame, len - leadingBytesInPreviousFrame); /* Process the trailing bytes (probably the next frame, starting with STX) */
            }
        }
        else { /* No ETX, copy the whole chunk */
            std::size_t maxCopy = this->getFreeBytes();
            std::size_t szCopy = len;
            if (szCopy > maxCopy) {  /* currentFrame overflow */
                szCopy = maxCopy; /* FIXME: Error case */
            }
            memcpy(this->currentFrame + nextWriteInCurrentFrame, buffer, szCopy);
            nextWriteInCurrentFrame += szCopy;
            usedBytes = szCopy;
        }
    }
    return usedBytes;
}

void TICUnframer::processCurrentFrame() {
    this->recordFrameSize(this->nextWriteInCurrentFrame);
    this->onFrameComplete(this->currentFrame, this->nextWriteInCurrentFrame);
}

bool TICUnframer::isInSync() {
    return this->sync;
}

std::size_t TICUnframer::getFreeBytes() const {
    return MAX_FRAME_SIZE - nextWriteInCurrentFrame;
}

void TICUnframer::recordFrameSize(unsigned int frameSize) {
    this->frameSizeHistory.push(frameSize);
}
