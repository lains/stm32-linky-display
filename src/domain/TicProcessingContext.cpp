#include "TicProcessingContext.h"

TicProcessingContext::TicProcessingContext(Stm32SerialDriver& ticSerial, TIC::Unframer& ticUnframer) :
    ticSerial(ticSerial),
    ticUnframer(ticUnframer),
    lostTicBytes(0),
    serialRxOverflowCount(0),
    instantaneousPower(),
    lastParsedFrameNb(static_cast<unsigned int>(-1)) {

}
