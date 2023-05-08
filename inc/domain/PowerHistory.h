#include "FixedSizeRingBuffer.h"
#include "TIC/DatasetView.h" // For TIC::Horodate
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

    PowerHistory(AveragingMode averagingPeriod);

    void onNewPowerData(const TicEvaluatedPower& power, const TIC::Horodate& horodate);

    bool horodatesAreInSamePeriodSample(const TIC::Horodate& first, const TIC::Horodate& second);

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
    TIC::Horodate lastPowerHorodate;    /*!< The horodate of the last received power measurement */
};
