#include "TimeOfDay.h"

TimeOfDay::TimeOfDay():
    isValid(false),
    hour(static_cast<unsigned int>(-1)),
    minute(static_cast<unsigned int>(-1)),
    second(static_cast<unsigned int>(-1)),
    millisecond(static_cast<unsigned int>(-1)),
    knownMilliseconds(false) { }

TimeOfDay::TimeOfDay(unsigned int hour, unsigned int minute, unsigned int second, unsigned int millisecond) :
    TimeOfDay() {
    if (hour >= 24 || minute >= 60 || second >= 60) {
        return;
    }

    if (millisecond < 1000) {
        this->millisecond = millisecond;
        this->knownMilliseconds = true;
    }
    else {
        if (millisecond != static_cast<unsigned int>(-1)) {
            return; /* -1 millisecond means unknown and is allowed, other values should be considered invalid */
        }
    }
    this->hour = hour;
    this->minute = minute;
    this->second = second;
    this->isValid = true;
}

TimeOfDay::TimeOfDay(const TIC::Horodate& from) :
    TimeOfDay(from.hour, from.minute, from.second) {
        this->isValid = from.isValid;
        this->estimatedTime = false;
}

unsigned int TimeOfDay::addSeconds(unsigned int seconds) {
    this->second += seconds; /* Forward in time */
    if (this->second < 60)
        return 0;
    this->minute += this->second / 60;
    this->second = this->second % 60;
    if (this->minute < 60)
        return 0;
    this->hour += this->minute / 60;
    this->minute = this->minute % 60;
    if (this->hour < 24)
        return 0;
    unsigned int dayAdd = this->hour / 24;
    this->hour = this->hour % 24;
    return dayAdd; /* Day wrap occured, return the number of days ahead */
}

unsigned int TimeOfDay::toSeconds() const {
    if (!this->isValid)
        return static_cast<unsigned int>(-1);
    return this->second + this->minute*60 + this->hour*3600;
}

int TimeOfDay::timeStampCmp(const TimeOfDay& other) const {
    if (!this->isValid && !other.isValid) {
        return 0;
    }
    if (this->isValid && !other.isValid) {
        return 1;
    }
    else if (this->isValid && !other.isValid) {
        return -1;
    }

    if (this->hour > other.hour) return 1;
    if (this->hour < other.hour) return -1;

    if (this->minute > other.minute) return 1;
    if (this->minute < other.minute) return -1;

    if (this->second > other.second) return 1;
    if (this->second < other.second) return -1;

    if (this->knownMilliseconds && other.knownMilliseconds) {
        if (this->millisecond > other.millisecond) return 1;
        if (this->millisecond < other.millisecond) return -1;
    }
    return 0;
}

bool TimeOfDay::operator==(const TimeOfDay& other) const {
    if (!this->isValid || !other.isValid)
        return false;
    
    return (this->timeStampCmp(other) == 0);
}

bool TimeOfDay::operator!=(const TimeOfDay& other) const {
    return !(*this == other);
}

bool TimeOfDay::operator>(const TimeOfDay& other) const {
    return (this->timeStampCmp(other) > 0);
}

bool TimeOfDay::operator<(const TimeOfDay& other) const {
    return (this->timeStampCmp(other) < 0);
}

bool TimeOfDay::operator>=(const TimeOfDay& other) const {
    return (this->timeStampCmp(other) >= 0);
}

bool TimeOfDay::operator<=(const TimeOfDay& other) const {
    return (this->timeStampCmp(other) <= 0);
}

#ifdef __TIC_LIB_USE_STD_STRING__
std::string TimeOfDay::toString() const {
    if (!this->isValid) {
        return "Invalid timestamp";
    }
    std::string result;
    result += '0' + ((this->hour / 10)   % 10);
    result += '0' + ((this->hour)        % 10);
    result += ':';
    result += '0' + ((this->minute / 10) % 10);
    result += '0' + ((this->minute)      % 10);
    result += ':';
    result += '0' + ((this->second / 10) % 10);
    result += '0' + ((this->second)      % 10);
    if (this->knownMilliseconds) {
        result += '.';
        result += '0' + ((this->millisecond / 100) % 10);
        result += '0' + ((this->millisecond / 10)  % 10);
        result += '0' + ((this->millisecond)       % 10);
    }
    return result;
}
#endif // __TIC_LIB_USE_STD_STRING__
