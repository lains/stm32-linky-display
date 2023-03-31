#include <string.h> // For memset()
#include "TIC/Unframer.h"

TIC::Unframer::Unframer(FOnNewFrameBytesFunc onNewFrameBytes, FOnFrameCompleteFunc onFrameComplete, void* parserFuncContext) :
sync(false),
onNewFrameBytes(onNewFrameBytes),
onFrameComplete(onFrameComplete),
parserFuncContext(parserFuncContext),
frameSizeHistory()
#ifndef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
,
nextWriteInCurrentFrame(0)
#endif
{
#ifndef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
    memset(this->currentFrame, 0, MAX_FRAME_SIZE);
#endif
}

/* TODO: check result implementation using either:
https://github.com/oktal/result
Or (seems better) https://github.com/bitwizeshift/result */


unsigned int TIC::Unframer::pushBytes(const uint8_t* buffer, unsigned int len) {
    if (len == 0)
        return 0;
    unsigned int usedBytes = 0;
//#ifdef __TIC_UNFRAMER_RECURSIVE_PUSHBYTES__
    if (!this->sync) {  /* We don't record bytes, we'll just look for a start of frame */
        uint8_t* firstStx = (uint8_t*)(memchr(buffer, TIC::Unframer::START_MARKER, len));
        if (firstStx) {
            this->sync = true;
            unsigned int bytesToSkip =  firstStx - buffer;  /* Bytes processed (but ignored) */
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
        /* We are inside a TIC frame, search for the end of frame (ETX) marker */
        uint8_t* etx = (uint8_t*)(memchr(buffer, TIC::Unframer::END_MARKER, len)); /* Search for end of frame */
        if (etx) {  /* We have an ETX in the buffer, we can extract the full frame */
            unsigned int leadingBytesInPreviousFrame = etx - buffer;
            usedBytes = this->processIncomingFrameBytes(buffer, leadingBytesInPreviousFrame); /* Copy the buffer up to (but exclusing the ETX marker) */
            this->processCurrentFrame(); /* The frame is complete */
            leadingBytesInPreviousFrame++; /* Skip the ETX marker */
            usedBytes++;
            this->sync = false; /* Consider we are outside of a frame now */
            if (leadingBytesInPreviousFrame < len) { /* We have at least one byte after the frame ETX */
                usedBytes += this->pushBytes(buffer + leadingBytesInPreviousFrame, len - leadingBytesInPreviousFrame); /* Process the trailing bytes (probably the next frame, starting with STX) */
            }
        }
        else { /* No ETX, copy the whole chunk */
            /* Incoming bytes */
            usedBytes = this->processIncomingFrameBytes(buffer, len); /* Process these bytes as valid */
        }
    }
//#else
//#endif
    return usedBytes;
}

unsigned int TIC::Unframer::processIncomingFrameBytes(const uint8_t* buffer, unsigned int len) {
#ifdef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
    if (this->onNewFrameBytes != nullptr && len > 0)
        this->onNewFrameBytes(buffer, len, this->parserFuncContext);
    return len;
#else
    unsigned int maxCopy = this->getFreeBytes();
    unsigned int szCopy = len;
    if (szCopy > maxCopy) {  /* currentFrame overflow */
        szCopy = maxCopy; /* FIXME: Error case */
    }
    memcpy(this->currentFrame + this->nextWriteInCurrentFrame, buffer, szCopy);
    this->nextWriteInCurrentFrame += szCopy;
    return szCopy;
#endif
}

void TIC::Unframer::processCurrentFrame() {
#ifndef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
    this->recordFrameSize(this->nextWriteInCurrentFrame);
    if (this->onNewFrameBytes != nullptr)
        this->onNewFrameBytes(this->currentFrame, this->nextWriteInCurrentFrame, this->parserFuncContext);
    this->nextWriteInCurrentFrame = 0; /* Wipe any data in the current frame, start over */
#endif
    if (this->onFrameComplete != nullptr)
        this->onFrameComplete(this->parserFuncContext);
}

bool TIC::Unframer::isInSync() const {
    return this->sync;
}

#ifndef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
unsigned int TIC::Unframer::getFreeBytes() const {
    return MAX_FRAME_SIZE - this->nextWriteInCurrentFrame;
}
#endif

void TIC::Unframer::recordFrameSize(unsigned int frameSize) {
    this->frameSizeHistory.push(frameSize);
}
