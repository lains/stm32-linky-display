#pragma once

#include "FixedSizeRingBuffer.h"
#include "TIC/DatasetView.h" // For TIC::Horodate
#include "TicProcessingContext.h"
#include "TicFrameParser.h" // For TicEvaluatedPower

struct PowerHistoryEntry {
    PowerHistoryEntry();
    PowerHistoryEntry(const TicEvaluatedPower& power, const TIC::Horodate& horodate);

private:
    /**
     * @brief Convert a signed long int to a signed int, truncating if needed
     * 
     * @param input The signed value to convert
     * @return A truncated value, equal to @p input if that value fits in a signed int, or maximized/minimized value if if overflows/underflows respectively
     */
    static signed int truncateSignedLongToSignedInt(const signed long input);

public:
    /**
     * @brief Update this object with an average between our current value and a new power measurement sample
     * 
     * @param power A new power measurement sample to take into account
     */
    void averageWithPowerSample(const TicEvaluatedPower& power, const TIC::Horodate& horodate);

/* Attributes */
    TicEvaluatedPower power; /*!< A power (in multiples or fractions of W... see scale below) */
    TIC::Horodate horodate; /*!< The horodate for the @p power entry */
    unsigned int nbSamples; /*!< The number of samples that have been averaged to produce the value in @p power */
    unsigned int scale; /*!< A divider for @p power. If scale=1000, then power is represented in mW */
};

struct PowerHistory {
    /* Types */
    typedef enum {
        PerSecond = 0,
        Per5Seconds,
        Per10Seconds,
        Per15Seconds,
        Per30Seconds,
        PerMinute,
        AveraginOverMinutesOrMore = PerMinute,
        Per5Minutes,
    } AveragingMode;

    /**
     * @brief Construct a new power history storage
     * 
     * @param averagingPeriod On which period do we average samples (we will only keep one value per period in the history, this is the step of our internal time resolution)
     * @param context A TicProcessingContext or null to disable any context update on new power data reception
     */
    PowerHistory(AveragingMode averagingPeriod, TicProcessingContext* context = nullptr);

    /**
     * @brief Set the TicProcessingContext instance we refresh on new power data reception
     * 
     * @param context A TicProcessingContext or null to disable any context update 
     */
    void setContext(TicProcessingContext* context = nullptr);

    void onNewPowerData(const TicEvaluatedPower& power, const TIC::Horodate& horodate);

    /**
     * @brief Check if two horodates are part of the same internal time resolution (and will thus be averaged to be stored in the same period history entry)
     * 
     * @param first The first horodate
     * @param second The second horodate
     * @return true If @p first and @p second end up in the same averaging period, and should thus be averaged to create one signel history entry
     */
    bool horodatesAreInSamePeriodSample(const TIC::Horodate& first, const TIC::Horodate& second);

    /**
     * @brief Get the averaging period value (in seconds)
     * 
     * @return The averaging period in seconds (the period that has been provided at construction), or 0 if unknown (error case)
     */
    unsigned int getAveragingPeriodInSeconds() const;

    /**
     * @brief Get the number of power history entries (averaging periods) per hour
     * 
     * @return The number of entries recorded per hour, or 0 if unknown (error case)
     */
    unsigned int getPowerRecordsPerHour() const;

    /**
     * @brief Utility function to unwrap a TicFrameParser instance and invoke onDatasetExtracted() on it
     * It is used as a callback provided to TIC::DatasetExtractor
     * 
     * @param context A context as provided by TIC::DatasetExtractor, used to retrieve the wrapped TicFrameParser instance
     */
    static void unWrapOnNewPowerData(const TicEvaluatedPower& power, const TIC::Horodate& horodate, void* context);

    /**
     * @brief Get the Last Values object
     * 
     * @param[in,out] nb The max number of measurements requested, modified at return to represent the number of measurements actually retrieved
     * @param[out] result A C-array of results, the first one being the most recent
     * 
     * @note The horodate of the most recent entry can be retrieved in the first element of the result array (if nb!=0 at return)
     */
    void getLastPower(unsigned int& nb, PowerHistoryEntry* result) const;

/* Attributes */
    FixedSizeRingBuffer<PowerHistoryEntry, 1024> data;    /*!< The last n instantaneous power measurements */
    AveragingMode averagingPeriod; /*!< Which sampling period do we record (we will perform an average on all samples within the period) */
    TicProcessingContext* ticContext;   /*!< An optional context structure instance that we should refresh on new power data reception */
    TIC::Horodate lastPowerHorodate;    /*!< The horodate of the last received power measurement */
};
