#include "TicProcessingContext.h"

SystemCurrentTime::SystemCurrentTime() :
    time(0, 0, 0, 0),
    relativeToBoot(true)
{
}

TicProcessingContext::TicProcessingContext(Stm32SerialDriver& ticSerial, TIC::Unframer& ticUnframer) :
    ticSerial(ticSerial),
    ticUnframer(ticUnframer),
    lostTicBytes(0),
    serialRxOverflowCount(0),
    instantaneousPower(),
    lastParsedFrameNb(static_cast<unsigned int>(-1)),
    displayTimeMs(0),
    fbCopyTimeMs(0),
    currentTime()
{
}
