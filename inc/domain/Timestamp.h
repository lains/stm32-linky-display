#pragma once

#include "TIC/DatasetView.h" // For TIC::Horodate

class Timestamp {
private:
    static constexpr unsigned int lastDayPerMonth[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
public:
/* Methods */
    Timestamp();

    /**
     * @brief Construct from hour, minute, seconds and optionally milliseconds
    */
    Timestamp(unsigned int hour, unsigned int minute, unsigned int second, unsigned int millisecond = static_cast<unsigned int>(-1));

    /**
     * @brief Construct from month, day, hour, minute, seconds and optionally milliseconds
    */
    Timestamp(unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second, unsigned int millisecond = static_cast<unsigned int>(-1));

    /**
     * @brief Construct from a TIC::Horodate
    */
    Timestamp(const TIC::Horodate& from);

    /**
     * @brief Make the current horodate go forward a given seconds in time
     * 
     * @param seconds The seconds to add
     * @return 0 if the result is in the same day as the original value, otherwise, the number of days forward (the current instance will be updated up to hours, but days will remain unchanged in any case)
     */
    unsigned int addSecondsWrapDay(unsigned int seconds);

private:
    /**
     * @brief Comparison of two timestamps
     * 
     * @param other The other Timestamp to compare with
     * @return int -1 is we are earlier than @other, 1 if we are later than @other and 0 if both are equal
     * 
     * @note If one timestamp is invalid, it is considered as the origin of time, thus earlier (-1) than any valid timestamps
     *       If both timestamps are invalid, they are considered equal (0)
     * @note We perform date comparison (day+month) if knownDate is true on timestamps
     *       If one date is unknown, it is considered as the origin of time, thus earlier (-1) than any valid timestamps
     * @note We only compare milliseconds if available on both objects
     */
    int timeStampCmp(const Timestamp& other) const;

public:
    bool operator==(const Timestamp& other) const;
    bool operator!=(const Timestamp& other) const;
    bool operator<(const Timestamp& other) const;
    bool operator>(const Timestamp& other) const;
    bool operator<=(const Timestamp& other) const;
    bool operator>=(const Timestamp& other) const;

#ifdef __TIC_LIB_USE_STD_STRING__
    std::string toString() const;
#endif

public:
/* Attributes */
    bool isValid; /*!< Does this instance contain a valid timestamp? */
    bool estimatedTime; /*!< The timestamp was generated by a precise clock */
    unsigned int month; /*!< The month */
    unsigned int day; /*!< The day */
    bool knownDate; /*!< Are the month+day valid? If false, the year will be invalid as well */
    unsigned int hour; /*!< The hour */
    unsigned int minute; /*!< The minute */
    unsigned int second; /*!< The second */
    unsigned int millisecond; /*!< The milliseconds */
    bool knownMilliseconds; /*!< Is the millisecond attribute valid? */
    bool absolute; /*!< Is the timestamp absolute, relative to the power up time or to the start of the current day? */
};