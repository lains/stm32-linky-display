#pragma once

#include "TIC/DatasetExtractor.h"
#include "TIC/DatasetView.h"

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

    TicEvaluatedPower(unsigned int minValue, unsigned int maxValue);

    void setMinMax(unsigned int minValue, unsigned int maxValue);

    void swapWith(TicEvaluatedPower& other);

    friend void ::std::swap(TicEvaluatedPower& first, TicEvaluatedPower& second);

/* Attributes */
    bool isValid; /*!< Is this evaluated absolute power valid? */
    unsigned int minValue; /*!< Minimum possible power, in Watts) */
    unsigned int maxValue; /*!< Maximum possible power, in Watts) */
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
    unsigned int instDrawnPower; /*!< The instantaneous (ie within the last TIC frame) withdrawn power, in Watts */
    unsigned int instVoltage; /*!< The instantaneous (ie within the last TIC frame) RMS voltage, in Volts */
    unsigned int instAbsCurrent; /*!< The instantaneous (ie within the last TIC frame) absolute current, in Amps */
    unsigned int maxSubscribedPower; /*!< The maximum allowed withdrawned power (subscribed), in Watts */
    TicEvaluatedPower instEvalPower; /*!< The instantaneous (ie within the last TIC frame) signed computed power (negative is injected, positive is withdrawn), in Amps */
};

class TicFrameParser {
public:
    TicFrameParser();

    void onNewMeasurementAvailable();

    void onNewDate(const TIC::Horodate& horodate);

    void onRefPowerInfo(uint32_t power);

    /**
     * @brief Take into account a refreshed instantenous power measurement
     * 
     * @param power The instantaneous power (in Watts)
     */
    void onNewInstPowerMesurement(uint32_t power);

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
     * @brief Try to guess a power range withdrawn or injected, based on RMS voltage+amps
     * 
     * @note This is very approximative, but if we don't have a precise instantaneous power, but we do have I and U, this is the best we can do
     */
    void mayComputePowerFromIURms();

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
    static void ticFrameParserUnWrapDatasetExtracter(const uint8_t* buf, unsigned int cnt, void* context);

/* Attributes */
    unsigned int nbFramesParsed; /*!< Total number of complete frames parsed */
    TIC::DatasetExtractor de;   /*!< The encapsulated dataset extractor instance (programmed to call us back on newly decoded datasets) */
    TicMeasurements lastFrameMeasurements;    /*!< Gathers all interesting measurement of the last frame */
};
