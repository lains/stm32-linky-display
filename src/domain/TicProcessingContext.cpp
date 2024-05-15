#include "TicProcessingContext.h"

SystemCurrentTime::SystemCurrentTime() :
    time(0, 0, 0, 0),
    relativeToBoot(true)
{
}

void SystemCurrentTime::startNewDayAtMidnight() {
    TimeOfDay midnight(0, 0, 0);
    std::swap(this->time, midnight);
    this->relativeToBoot = false;
}

TicProcessingContext::TicProcessingContext(Stm32SerialDriver& ticSerial, TIC::Unframer& ticUnframer) :
    ticSerial(ticSerial),
    ticUnframer(ticUnframer),
    lostTicBytes(0),
    serialRxOverflowCount(0),
    datasetsWithErrors(0),
    instantaneousPower(),
    lastParsedFrameNb(static_cast<unsigned int>(-1)),
    displayTimeMs(0),
    fbCopyTimeMs(0),
    currentTime()
{
}

