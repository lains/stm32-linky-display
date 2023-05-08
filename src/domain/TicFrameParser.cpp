#include "TicFrameParser.h"

#include <climits>
#include <string.h>
#include <sstream>
#include <utility> // For std::swap()

#ifndef __UNIT_TEST__
extern "C" {
#include "main.h" // FIXME: For BSP_LED_Toogle() and LED1, but these concepts should go away from TIC domain
}
#else
#define BSP_LED_Toggle(x) do { } while (0)
#endif

TicEvaluatedPower::TicEvaluatedPower() :
    isValid(false),
    minValue(INT_MIN),
    maxValue(INT_MAX)
{
}

TicEvaluatedPower::TicEvaluatedPower(int minValue, int maxValue) :
    isValid(true),
    minValue(INT_MIN), /* Note: we don't set actual min and max here, but using the utility method setMinMax() below */
    maxValue(INT_MAX)
{
    this->setMinMax(minValue, maxValue);
}

void TicEvaluatedPower::setMinMax(int minValue, int maxValue) {
    this->isValid = true;
    if (minValue <= maxValue) {
        this->minValue = minValue;
        this->maxValue = maxValue;
    }
    else {
        this->minValue = maxValue;
        this->maxValue = minValue;

    }
    this->isExact = (minValue == maxValue);
}

void TicEvaluatedPower::set(int value) {
    this->setMinMax(value, value);
}

bool operator==(const TicEvaluatedPower& lhs, const TicEvaluatedPower& rhs) {
    return (lhs.isValid == rhs.isValid &&
            lhs.minValue == rhs.minValue &&
            lhs.maxValue == rhs.maxValue);
}

std::string TicEvaluatedPower::toString() const {
    std::stringstream result;
    result << "TicEvaluatedPower(";
    if (!this->isValid) {
        result << "INVALID";
    }
    else {
        result << this->minValue;
        result << ";";
        result << this->maxValue;
    }
    result << ")";
    return result.str();
}

void TicEvaluatedPower::swapWith(TicEvaluatedPower& other) {
    std::swap(this->isValid, other.isValid);
    std::swap(this->minValue, other.minValue);
    std::swap(this->maxValue, other.maxValue);
}

void std::swap(TicEvaluatedPower& first, TicEvaluatedPower& second) {
    first.swapWith(second);
}

std::string std::to_string(const TicEvaluatedPower& power) {
    return power.toString();
}

TicMeasurements::TicMeasurements() :
    fromFrameNb(static_cast<unsigned int>(-1)),
    horodate(),
    instVoltage(static_cast<unsigned int>(-1)),
    instAbsCurrent(static_cast<unsigned int>(-1)),
    instPower()
{
}

TicMeasurements::TicMeasurements(unsigned int fromFrameNb) :
    fromFrameNb(fromFrameNb),
    horodate(),
    instVoltage(static_cast<unsigned int>(-1)),
    instAbsCurrent(static_cast<unsigned int>(-1)),
    instPower()
{
}

void TicMeasurements::reset() {
    this->fromFrameNb = static_cast<unsigned int>(-1);
    this->horodate = TIC::Horodate();
    this->instVoltage = static_cast<unsigned int>(-1);
    this->instAbsCurrent = static_cast<unsigned int>(-1);
    this->instPower = TicEvaluatedPower();
}

void TicMeasurements::swapWith(TicMeasurements& other) {
    std::swap(this->fromFrameNb, other.fromFrameNb);
    std::swap(this->horodate, other.horodate);
    std::swap(this->instVoltage, other.instVoltage);
    std::swap(this->instAbsCurrent, other.instAbsCurrent);
    std::swap(this->maxSubscribedPower, other.maxSubscribedPower);
    std::swap(this->instPower, other.instPower);
}

void std::swap(TicMeasurements& first, TicMeasurements& second) {
    first.swapWith(second);
}

TicFrameParser::TicFrameParser(FOnNewPowerData onNewPowerData, void* onNewPowerDataContext) :
    onNewPowerData(onNewPowerData),
    onNewPowerDataContext(onNewPowerDataContext),
    nbFramesParsed(0),
    de(ticFrameParserUnWrapDatasetExtractor, this),
    lastFrameMeasurements()
{
}

#define RESET 0
#define WITHDRAWN_POWER 1
#define RMS_VOLTAGE 2
#define ABS_CURRENT 3

void TicFrameParser::onNewWithdrawnPowerMesurement(uint32_t power) {
    this->onNewMeasurementAvailable();
    this->mayComputePower(WITHDRAWN_POWER, static_cast<unsigned int>(power));
}

void TicFrameParser::mayComputePower(unsigned int source, unsigned int value) {
    static bool powerKnownForCurrentFrame = false;
    static bool mayInject = false;
    static unsigned int knownRmsVoltage = static_cast<unsigned int>(-1);
    static unsigned int knownAbsCurrent = static_cast<unsigned int>(-1);

    if (source == RESET) {
        if (!powerKnownForCurrentFrame) {
           //this->ctx.tic.lastValidWithdrawnPower = INT32_MIN; /* Unknown power */
        }
        powerKnownForCurrentFrame = false;
        mayInject = false;
        knownRmsVoltage = static_cast<unsigned int>(-1);
        knownAbsCurrent = static_cast<unsigned int>(-1);
        return;
    }
    if (powerKnownForCurrentFrame)
        return;

    if (source == WITHDRAWN_POWER) {
        if (value > 0) {
            powerKnownForCurrentFrame = true;
            this->onNewComputedPower(value, value);
            return;
        }
        else { /* Withdrawn power is 0, we may actually inject */
            mayInject = true;
        }
    }
    else if (source == RMS_VOLTAGE) {
        knownRmsVoltage = value;
    }
    else if (source == ABS_CURRENT) {
        knownAbsCurrent = value;
    }
    if (!powerKnownForCurrentFrame &&
        mayInject &&
        knownRmsVoltage != static_cast<unsigned int>(-1) &&
        knownAbsCurrent != static_cast<unsigned int>(-1)) {
        /* We are able to estimate the injected power */
        /* We have both voltage and current, we can grossly approximate the power (withdrawn or injected) */
        unsigned int& urms = knownRmsVoltage;
        unsigned int& irms = knownAbsCurrent;
        unsigned int avg = irms * urms;
        unsigned int min = 0;
        if (avg >= urms/2) {
            min = avg - urms/2;
        }
        /* If average - urms/2 becomes negative, assume a minimum power of 0 instead of that negative value */

        unsigned int max = avg + urms/2;
        
        powerKnownForCurrentFrame = true;
        this->onNewComputedPower(-max, -min); /* min and max are *positive* minimum and maximum injected power values, we thus negate them before storing in the instantaneous power */
        return;
    }
}

void TicFrameParser::onNewMeasurementAvailable() {
    if (this->lastFrameMeasurements.fromFrameNb != this->nbFramesParsed) {
        TicMeasurements blankMeasurements(this->nbFramesParsed);
        std::swap(this->lastFrameMeasurements, blankMeasurements);  /* Create a new empty measurement datastore for the new frame */
        mayComputePower(RESET, 0); /* Reset the computed power, we will need to collect all data again from the new frame */
    }
}

void TicFrameParser::onNewDate(const TIC::Horodate& horodate) {
    this->onNewMeasurementAvailable();
    this->lastFrameMeasurements.horodate = horodate;
}

void TicFrameParser::onRefPowerInfo(uint32_t power) {
    //FIXME: Todo
}

void TicFrameParser::onNewInstVoltageMeasurement(uint32_t voltage) {
    this->onNewMeasurementAvailable();
    this->lastFrameMeasurements.instVoltage = voltage;
    this->mayComputePower(RMS_VOLTAGE, static_cast<unsigned int>(voltage));
}

void TicFrameParser::onNewInstCurrentMeasurement(uint32_t current) {
    this->onNewMeasurementAvailable();
    this->lastFrameMeasurements.instAbsCurrent = current;
    this->mayComputePower(ABS_CURRENT, static_cast<unsigned int>(current));
}

void TicFrameParser::onNewFrameBytes(const uint8_t* buf, unsigned int cnt) {
    this->de.pushBytes(buf, cnt);   /* Forward the bytes to the dataset extractor */
}

void TicFrameParser::onNewComputedPower(int minValue, int maxValue) {
    this->lastFrameMeasurements.instPower.setMinMax(minValue, maxValue);
    if (this->onNewPowerData != nullptr) {
        this->onNewPowerData(this->lastFrameMeasurements.instPower, this->lastFrameMeasurements.horodate, this->onNewPowerDataContext);
    }
}

void TicFrameParser::onFrameComplete() {
    this->de.reset();
    BSP_LED_Toggle(LED1); // Toggle the green LED when a frame has been received
    this->nbFramesParsed++;
}

void TicFrameParser::onDatasetExtracted(const uint8_t* buf, unsigned int cnt) {
    /* This is our actual parsing of a newly received dataset */
    TIC::DatasetView dv(buf, cnt);    /* Decode the TIC dataset using a dataset view object */
    if (dv.isValid()) {
        if (dv.labelSz == 4 &&
            memcmp(dv.labelBuffer, "DATE", 4) == 0) {
            /* The current label is a DATE */
            if (dv.horodate.isValid) {
                this->onNewDate(dv.horodate);
            }
        }
        /* Search for SINSTS */
        else if (dv.labelSz == 6 &&
            memcmp(dv.labelBuffer, "SINSTS", 6) == 0 &&
            dv.dataSz > 0) {
            /* The current label is a SINSTS with some value associated */
            uint32_t sinsts = dv.uint32FromValueBuffer(dv.dataBuffer, dv.dataSz);
            if (sinsts != (uint32_t)-1)
                this->onNewWithdrawnPowerMesurement(sinsts);
        }
        /* Search for URMS1 */
        else if (dv.labelSz == 5 &&
            memcmp(dv.labelBuffer, "URMS1", 5) == 0 &&
            dv.dataSz > 0) {
            /* The current label is a URMS1 with some value associated */
            uint32_t urms1 = dv.uint32FromValueBuffer(dv.dataBuffer, dv.dataSz);
            if (urms1 != (uint32_t)-1)
                this->onNewInstVoltageMeasurement(urms1);
        }
        /* Search for IRMS1 */
        else if (dv.labelSz == 5 &&
            memcmp(dv.labelBuffer, "IRMS1", 5) == 0 &&
            dv.dataSz > 0) {
            /* The current label is a URMS1 with some value associated */
            uint32_t irms1 = dv.uint32FromValueBuffer(dv.dataBuffer, dv.dataSz);
            if (irms1 != (uint32_t)-1)
                this->onNewInstCurrentMeasurement(irms1);
        }
        /* Search for PREF */
        else if (dv.labelSz == 4 &&
            memcmp(dv.labelBuffer, "PREF", 4) == 0 &&
            dv.dataSz > 0) {
            /* The current label is a URMS1 with some value associated */
            uint32_t pref = dv.uint32FromValueBuffer(dv.dataBuffer, dv.dataSz);
            if (pref != (uint32_t)-1)
                this->onRefPowerInfo(pref);
        }
    }
}

void TicFrameParser::unwrapInvokeOnFrameNewBytes(const uint8_t* buf, unsigned int cnt, void* context) {
    if (context == nullptr)
        return; /* Failsafe, discard if no context */
    TicFrameParser* ticFrameParserInstance = static_cast<TicFrameParser*>(context);
    ticFrameParserInstance->onNewFrameBytes(buf, cnt);
}

void TicFrameParser::unwrapInvokeOnFrameComplete(void *context) {
    if (context == nullptr)
        return; /* Failsafe, discard if no context */
    TicFrameParser* ticFrameParserInstance = static_cast<TicFrameParser*>(context);
    /* We have finished parsing a frame, if there is an open dataset, we should discard it and start over at the following frame */
    ticFrameParserInstance->onFrameComplete();
}

    /**
     * @brief Utility function to unwrap a TicFrameParser instance and invoke onDatasetExtracted() on it
     * It is used as a callback provided to TIC::DatasetExtractor
     * 
     * @param context A context as provided by TIC::DatasetExtractor, used to retrieve the wrapped TicFrameParser instance
     */
void TicFrameParser::ticFrameParserUnWrapDatasetExtractor(const uint8_t* buf, unsigned int cnt, void* context) {
    if (context == nullptr)
        return; /* Failsafe, discard if no context */
    TicFrameParser* ticFrameParserInstance = static_cast<TicFrameParser*>(context);
    /* We have finished parsing a frame, if there is an open dataset, we should discard it and start over at the following frame */
    ticFrameParserInstance->onDatasetExtracted(buf, cnt);
}
