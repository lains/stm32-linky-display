#include "Timestamp.h"

constexpr unsigned int Timestamp::lastDayPerMonth[];

Timestamp::Timestamp():
    isValid(false),
    estimatedTime(true),
    month(static_cast<unsigned int>(-1)),
    day(static_cast<unsigned int>(-1)),
    knownDate(false),
    hour(static_cast<unsigned int>(-1)),
    minute(static_cast<unsigned int>(-1)),
    second(static_cast<unsigned int>(-1)),
    millisecond(static_cast<unsigned int>(-1)),
    knownMilliseconds(false) { }

Timestamp::Timestamp(unsigned int hour, unsigned int minute, unsigned int second, unsigned int millisecond) :
    Timestamp() {
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

Timestamp::Timestamp(unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second, unsigned int millisecond) :
    Timestamp(hour, minute, second, millisecond) {
    this->isValid = false; /* Even if timestamp construction has been delegated above, not all args have been checked yet */
    if (month == 0 || month > 12) {
        /* Error case */
        return;
    }
    if (day == 0 || day > Timestamp::lastDayPerMonth[month-1]) {
        /* Invalid day */
        return;
    }
    this->month = month;
    this->day = day;
    this->isValid = true;
    this->knownDate = true;
}

Timestamp::Timestamp(const TIC::Horodate& from) :
    Timestamp(from.month, from.day, from.hour, from.minute, from.second) {
        this->isValid = from.isValid;
        this->estimatedTime = false;
}

unsigned int Timestamp::addSecondsWrapDay(unsigned int seconds) {
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

unsigned int Timestamp::toSeconds() const {
    if (!this->isValid)
        return static_cast<unsigned int>(-1);
    return this->second + this->minute*60 + this->hour*3600;
}

int Timestamp::timeStampCmp(const Timestamp& other) const {
    if (!this->isValid && !other.isValid) {
        return 0;
    }
    if (this->isValid && !other.isValid) {
        return 1;
    }
    else if (this->isValid && !other.isValid) {
        return -1;
    }

    if (this->knownDate && !other.knownDate) {
        return 1;
    }
    else if (this->knownDate && !other.knownDate) {
        return -1;
    }
    if (this->knownDate && other.knownDate) {
        if (this->month > other.month) return 1;
        if (this->month < other.month) return -1;
        if (this->day > other.day) return 1;
        if (this->day < other.day) return -1;
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

bool Timestamp::operator==(const Timestamp& other) const {
    if (!this->isValid || !other.isValid)
        return false;
    
    return (this->timeStampCmp(other) == 0);
}

bool Timestamp::operator!=(const Timestamp& other) const {
    return !(*this == other);
}

bool Timestamp::operator>(const Timestamp& other) const {
    return (this->timeStampCmp(other) > 0);
}

bool Timestamp::operator<(const Timestamp& other) const {
    return (this->timeStampCmp(other) < 0);
}

bool Timestamp::operator>=(const Timestamp& other) const {
    return (this->timeStampCmp(other) >= 0);
}

bool Timestamp::operator<=(const Timestamp& other) const {
    return (this->timeStampCmp(other) <= 0);
}

#ifdef __TIC_LIB_USE_STD_STRING__
std::string Timestamp::toString() const {
    if (!this->isValid) {
        return "Invalid timestamp";
    }
    std::string result;
    if (this->knownDate) {
        result += '0' + ((this->month / 10)  % 10);
        result += '0' + ((this->month)       % 10);
        result += '/';
        result += '0' + ((this->day / 10)    % 10);
        result += '0' + ((this->day)         % 10);
        result += ' ';
    }
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
