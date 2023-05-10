#pragma once

#include <string>

#include "TIC/DatasetExtractor.h"
#include "TIC/DatasetView.h"
#include "FixedSizeRingBuffer.h"

/* Forward declarations */
class TicFrameParser;
class TicEvaluatedPower;
class TicMeasurements;
namespace std {
     void swap(TicEvaluatedPower& first, TicEvaluatedPower& second);
     void swap(TicMeasurements& first, TicMeasurements& second);
}

class TicEvaluatedPower {
public:
    TicEvaluatedPower();

    TicEvaluatedPower(int minValue, int maxValue);

    void setMinMax(int minValue, int maxValue);

    void set(int value);

    void swapWith(TicEvaluatedPower& other);

#ifdef __UNIT_TEST__
    std::string toString() const;
#endif

    friend bool operator==(const TicEvaluatedPower& lhs, const TicEvaluatedPower& rhs);

    friend void ::std::swap(TicEvaluatedPower& first, TicEvaluatedPower& second);

/* Attributes */
    bool isValid; /*!< Is this evaluated absolute power valid? */
    bool isExact; /*!< Is this power exact (in such cases, min and max are equal), if false, we store an approxmation range)*/
    int minValue; /*!< Minimum possible power, in Watts), minValue <= maxValue is always satisfied if isValid==true */
    int maxValue; /*!< Maximum possible power, in Watts), maxValue >= maxValue is always satisfied if isValid==true */
};

class TicMeasurements {
public:
    TicMeasurements();

    TicMeasurements(unsigned int fromFrameNb);

    void reset();

    void swapWith(TicMeasurements& other);

    friend void ::std::swap(TicMeasurements& first, TicMeasurements& second);

/* Attributes */
    unsigned int fromFrameNb; /*!< The TIC frame ID from which the enclosed data has been extracted */
    TIC::Horodate horodate; /*!< The optional horodate for the TIC frame */
    unsigned int instVoltage; /*!< The instantaneous (ie within the last TIC frame) RMS voltage, in Volts */
    unsigned int instAbsCurrent; /*!< The instantaneous (ie within the last TIC frame) absolute current, in Amps */
    unsigned int maxSubscribedPower; /*!< The maximum allowed withdrawned power (subscribed), in Watts */
    TicEvaluatedPower instPower; /*!< The instantaneous (ie within the last TIC frame) signed power (negative if injected, positive if withdrawn), in Watts... may be an exact value or a range if approximated */
};

class TicFrameParser {
public:
/* Types */
    typedef void(*FOnNewPowerData)(const TicEvaluatedPower& power, const TIC::Horodate& horodate, void* context); /*!< The prototype of callbacks invoked on new power data */

/* Methods */
    /**
     * @brief Construct a new TicFrameParser object
     * 
     * @param onNewPowerData A FOnNewPowerData function to invoke for each new power data received
     * @param onNewPowerDataContext A user-defined pointer that will be passed as last argument when invoking onNewPowerData()
     * 
     * @note We are using C-style function pointers here (with data-encapsulation via a context pointer)
     *       This is because we don't have 100% guarantee that exceptions are allowed (especially on embedded targets) and using std::function requires enabling exceptions.
     *       We can still use non-capturing lambdas as function pointer if needed (see https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer)
     */
    TicFrameParser(FOnNewPowerData onNewPowerData = nullptr, void* onNewPowerDataContext = nullptr);

protected:
    void onNewMeasurementAvailable();

    void onNewDate(const TIC::Horodate& horodate);

    void onRefPowerInfo(uint32_t power);

    void onNewComputedPower(int minValue, int maxValue);

    /**
     * @brief Take into account a refreshed instantenous withdrawn power measurement
     * 
     * @param power The instantaneous withdrawn power (in Watts)
     */
    void onNewWithdrawnPowerMesurement(uint32_t power);

    /**
     * @brief Take into account a refreshed instantenous voltage measurement
     * 
     * @param voltage The instantaneous mains voltage (in Volts)
     */
    void onNewInstVoltageMeasurement(uint32_t voltage);

    /**
     * @brief Take into account a refreshed instantenous current measurement
     * 
     * @param voltage The instantaneous mains absolute current (in Amperes), the sign is not provided so this current can be widthdrawn or injected
     */
    void onNewInstCurrentMeasurement(uint32_t current);

    /**
     * @brief Try to compute the current withdrawn or injected power as soon as we have collected enough values (power, abs current, rms voltage)
     * 
     * @param source The measurement type to take into account
     * @param value The instantaneous value corresponding to @p source
     */
    void mayComputePower(unsigned int source, unsigned int value);

public:
    /* The 3 methods below are invoked as callbacks by TIC::Unframer and TIC::DatasetExtractor durig the TIC decoding process */
    /**
     * @brief Method invoked on new bytes received inside a TIC frame
     * 
     * When receiving new bytes, we will directly forward them to the encapsulated dataset extractor
     * 
     * @param buf A buffer containing new TIC frame bytes
     * @param len The number of bytes stored inside @p buf
     */
    void onNewFrameBytes(const uint8_t* buf, unsigned int cnt);

    /**
     * @brief Method invoked when we reach the end of a TIC frame
     * 
     * @warning When reaching the end of a frame, it is mandatory to reset the encapsulated dataset extractor state, so that it starts from scratch on the next frame.
     *          Not doing so would mix datasets content accross two successive frames if we have unterminated datasets, which may happen in historical TIC streams
     */
    void onFrameComplete();

    /**
     * @brief Method invoken when a new dataset has been extracted from the TIC stream
     * 
     * @param buf A buffer containing full TIC dataset bytes
     * @param len The number of bytes stored inside @p buf
     */
    void onDatasetExtracted(const uint8_t* buf, unsigned int cnt);

    /* The 3 commodity functions below are used as callbacks to retrieve a TicFrameParser casted as a context */
    /* They are retrieving our instance on TicFrameParser, and invoking the 3 above corresponding methods of TicFrameParser, forwarding their arguments */
    /**
     * @brief Utility function to unwrap a TicFrameParser instance and invoke onNewFrameBytes() on it
     * It is used as a callback provided to TIC::Unframer
     * 
     * @param buf A buffer containing new TIC frame bytes
     * @param len The number of bytes stored inside @p buf
     * @param context A context as provided by TIC::Unframer, used to retrieve the wrapped TicFrameParser instance
     */
    static void unwrapInvokeOnFrameNewBytes(const uint8_t* buf, unsigned int cnt, void* context);

    /**
     * @brief Utility function to unwrap a TicFrameParser instance and invoke onFrameComplete() on it
     * It is used as a callback provided to TIC::Unframer
     * 
     * @param context A context as provided by TIC::Unframer, used to retrieve the wrapped TicFrameParser instance
     */
    static void unwrapInvokeOnFrameComplete(void *context);

    /**
     * @brief Utility function to unwrap a TicFrameParser instance and invoke onDatasetExtracted() on it
     * It is used as a callback provided to TIC::DatasetExtractor
     * 
     * @param context A context as provided by TIC::DatasetExtractor, used to retrieve the wrapped TicFrameParser instance
     */
    static void ticFrameParserUnWrapDatasetExtractor(const uint8_t* buf, unsigned int cnt, void* context);

/* Attributes */
    FOnNewPowerData onNewPowerData; /*!< Pointer to a function invoked at each new power data (withdrawn or injected) computed from a TIC frame */
    void* onNewPowerDataContext; /*!< A context pointer passed to onNewFrameBytes() and onFrameComplete() at invokation */
    unsigned int nbFramesParsed; /*!< Total number of complete frames parsed */
    TIC::DatasetExtractor de;   /*!< The encapsulated dataset extractor instance (programmed to call us back on newly decoded datasets) */
    TicMeasurements lastFrameMeasurements;    /*!< Gathers all interesting measurement of the last frame */
};

#ifdef __UNIT_TEST__
/* std::to_string() for TicEvaluatedPower */
namespace std {
    std::string to_string(const TicEvaluatedPower& power);
}
#endif
