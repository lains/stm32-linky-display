#include "TicFrameParser.h"

#include <climits>
#include <string.h>
#include <utility> // For std::swap()

#ifdef EMBEDDED_DEBUG_CONSOLE
#include "Stm32DebugOutput.h"
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

#ifdef __UNIT_TEST__
std::string TicEvaluatedPower::toString() const {
    std::string result("TicEvaluatedPower(");
    if (!this->isValid) {
        result += "INVALID";
    }
    else {
        result += std::to_string(this->minValue);
        result += ';';
        result += std::to_string(this->maxValue);
    }
    result += ")";
    return result;
}
#endif

void TicEvaluatedPower::swapWith(TicEvaluatedPower& other) {
    std::swap(this->isValid, other.isValid);
    std::swap(this->minValue, other.minValue);
    std::swap(this->maxValue, other.maxValue);
}

void std::swap(TicEvaluatedPower& first, TicEvaluatedPower& second) {
    first.swapWith(second);
}

#ifdef __UNIT_TEST__
std::string std::to_string(const TicEvaluatedPower& power) {
    return power.toString();
}
#endif

TicMeasurements::TicMeasurements() :
    fromFrameNb(static_cast<unsigned int>(-1)),
    timestamp(),
    instVoltage(static_cast<unsigned int>(-1)),
    instAbsCurrent(static_cast<unsigned int>(-1)),
    instPower()
{
}

TicMeasurements::TicMeasurements(unsigned int fromFrameNb) :
    fromFrameNb(fromFrameNb),
    timestamp(),
    instVoltage(static_cast<unsigned int>(-1)),
    instAbsCurrent(static_cast<unsigned int>(-1)),
    instPower()
{
}

void TicMeasurements::reset() {
    this->fromFrameNb = static_cast<unsigned int>(-1);
    this->timestamp = TimeOfDay();
    this->instVoltage = static_cast<unsigned int>(-1);
    this->instAbsCurrent = static_cast<unsigned int>(-1);
    this->instPower = TicEvaluatedPower();
}

void TicMeasurements::swapWith(TicMeasurements& other) {
    std::swap(this->fromFrameNb, other.fromFrameNb);
    std::swap(this->timestamp, other.timestamp);
    std::swap(this->instVoltage, other.instVoltage);
    std::swap(this->instAbsCurrent, other.instAbsCurrent);
    std::swap(this->maxSubscribedPower, other.maxSubscribedPower);
    std::swap(this->instPower, other.instPower);
}

void std::swap(TicMeasurements& first, TicMeasurements& second) {
    first.swapWith(second);
}

TicFrameParser::TicFrameParser(FOnNewPowerDataFunc onNewPowerData, void* onNewPowerDataContext) :
    onNewPowerData(onNewPowerData),
    onNewPowerDataContext(onNewPowerDataContext),
    onDayOverFunc([](void* context) { }),
    onDayOverFuncContext(nullptr),
    nbFramesParsed(0),
    de(ticFrameParserUnWrapDatasetExtractor, this),
    lastFrameMeasurements(),
    lastKnownMaxPower(0)
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
    this->lastFrameMeasurements.timestamp = TimeOfDay(horodate);
}

void TicFrameParser::guessFrameArrivalTime() {
    this->onNewMeasurementAvailable();
    if (this->lastFrameMeasurements.fromFrameNb != this->nbFramesParsed) {
#ifdef EMBEDDED_DEBUG_CONSOLE
        Stm32DebugOutput::get().send("It seems we are in a new frame ID ");
        Stm32DebugOutput::get().send(static_cast<unsigned int>(this->nbFramesParsed));
        Stm32DebugOutput::get().send("\n");
#endif
    }
    unsigned int emulatedSecond =  (this->nbFramesParsed * 3) % 60; /* Assume 1 historical TIC frame every 3 seconds */
    unsigned int emulatedHorodateRemainder = (this->nbFramesParsed / 20); /* Counts total remainder as minutes */
    unsigned int emulatedMinute = emulatedHorodateRemainder % 60;
    emulatedHorodateRemainder = emulatedHorodateRemainder / 60; /* Now count total remainder as hours */
    unsigned int emulatedHour = emulatedHorodateRemainder % 24;
    /* Note: we discard days and month for now */
    this->lastFrameMeasurements.timestamp = TimeOfDay(emulatedHour, emulatedMinute, emulatedSecond);
#ifdef EMBEDDED_DEBUG_CONSOLE
    Stm32DebugOutput::get().send("Injecting timestamp in historical frame: ");
    Stm32DebugOutput::get().send(static_cast<unsigned int>(this->lastFrameMeasurements.timestamp.hour));
    Stm32DebugOutput::get().send(":");
    Stm32DebugOutput::get().send(static_cast<unsigned int>(this->lastFrameMeasurements.timestamp.minute));
    Stm32DebugOutput::get().send(":");
    Stm32DebugOutput::get().send(static_cast<unsigned int>(this->lastFrameMeasurements.timestamp.second));
    Stm32DebugOutput::get().send("\n");
#endif
}

void TicFrameParser::onDayOverInvoke(FOnDayOverFunc dayOverFunc, void* context) {
    this->onDayOverFunc = dayOverFunc;
    this->onDayOverFuncContext = context;
}

void TicFrameParser::onRefPowerInfo(uint32_t power) {
    //FIXME: Todo
}

void TicFrameParser::onMaxPowerInfo(uint32_t maxPower) {
    if (maxPower < this->lastKnownMaxPower) { /* Daily max power reduces (probably 0), this means we are starting a new day, we are at midnight */
#ifdef EMBEDDED_DEBUG_CONSOLE
        Stm32DebugOutput::get().send("Switch to new day detected\n");
#endif
        if (this->onDayOverFunc) {
            this->onDayOverFunc(this->onDayOverFuncContext);
        }
    }
    this->lastKnownMaxPower = maxPower;
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
    
#ifdef EMBEDDED_DEBUG_CONSOLE
    Stm32DebugOutput::get().send("onNewComputedPower(");
    if (minValue != maxValue)
        Stm32DebugOutput::get().send("[");
    {
        if (minValue < 0) {
            Stm32DebugOutput::get().send("-");
            Stm32DebugOutput::get().send(static_cast<unsigned int>(-minValue));
        }
        else {
            Stm32DebugOutput::get().send(static_cast<unsigned int>(minValue));
        }
    }
    if (minValue != maxValue) {
        Stm32DebugOutput::get().send(";");
        if (maxValue < 0) {
            Stm32DebugOutput::get().send("-");
            Stm32DebugOutput::get().send(static_cast<unsigned int>(-maxValue));
        }
        else {
            Stm32DebugOutput::get().send(static_cast<unsigned int>(maxValue));
        }
        Stm32DebugOutput::get().send("]");
    }
    Stm32DebugOutput::get().send("W) with ");
    if (this->lastFrameMeasurements.timestamp.isValid) {
        Stm32DebugOutput::get().send("a valid");
    }
    else {
        Stm32DebugOutput::get().send("no");
    }
    Stm32DebugOutput::get().send("horodate\n");
#endif
    this->lastFrameMeasurements.instPower.setMinMax(minValue, maxValue);
    if (this->onNewPowerData != nullptr) {
        this->onNewPowerData(this->lastFrameMeasurements.instPower, this->lastFrameMeasurements.timestamp, this->nbFramesParsed, onNewPowerDataContext);
    }
}

void TicFrameParser::onFrameComplete() {
    this->de.reset();
    this->nbFramesParsed++;
}

void TicFrameParser::onDatasetExtracted(const uint8_t* buf, unsigned int cnt) {
    /* This is our actual parsing of a newly received dataset */
    //std::cout << "Entering TicFrameParser::onDatasetExtracted() with a " << std::dec << cnt << " byte(s) long dataset\n";

#ifdef EMBEDDED_DEBUG_CONSOLE
    Stm32DebugOutput::get().send("onDatasetExtracted() called with ");
    Stm32DebugOutput::get().send(static_cast<unsigned int>(cnt));
    Stm32DebugOutput::get().send(" bytes\n");
#endif
    TIC::DatasetView dv(buf, cnt);    /* Decode the TIC dataset using a dataset view object */
    //std::cout << "Above dataset is " << std::string(dv.isValid()?"":"in") << "valid\n";
    if (dv.isValid()) {
#ifdef EMBEDDED_DEBUG_CONSOLE
        Stm32DebugOutput::get().send("New dataset: ");
        Stm32DebugOutput::get().send(dv.labelBuffer, dv.labelSz);
        Stm32DebugOutput::get().send("\n");
#endif
        if (dv.decodedType == TIC::DatasetView::ValidHistorical) {  /* In this case, we will have no horodate, evaluate time instead */
#ifdef EMBEDDED_DEBUG_CONSOLE
            Stm32DebugOutput::get().send("Dataset above is following historical format\n");
#endif
            this->guessFrameArrivalTime();
        }
        //std::vector<uint8_t> datasetLabel(dv.labelBuffer, dv.labelBuffer+dv.labelSz);
        //std::cout << "Dataset has label \"" << std::string(datasetLabel.begin(), datasetLabel.end()) << "\"\n";
        if (dv.labelEquals("DATE")) {
            if (dv.horodate.isValid) {
                this->onNewDate(dv.horodate);
            }
        }
        /* Search for withdrawn power labels */
        else if (dv.labelEquals("SINSTS") || dv.labelEquals("PAPP")) {
            //std::cout << "Found inst power dataset\n";
            if (dv.dataSz > 0) {
                //std::vector<uint8_t> datasetValue(dv.dataBuffer, dv.dataBuffer+dv.dataSz);
                //std::cout << "Power data received: \"" << std::string(datasetValue.begin(), datasetValue.end()) << "\"\n";
                uint32_t withdrawnPower = dv.dataToUint32();
                //std::cout << "interpreted as " << withdrawnPower << "W\n";
                if (withdrawnPower != (uint32_t)-1)
                    this->onNewWithdrawnPowerMesurement(withdrawnPower);
            }
        }
        /* Search for rms voltage value */
        else if (dv.labelEquals("URMS1")) {
            uint32_t urms1 = dv.dataToUint32();
            if (urms1 != (uint32_t)-1)
                this->onNewInstVoltageMeasurement(urms1);
        }
        /* Search for rms current value */
        else if (dv.labelEquals("IRMS1")) {
            uint32_t irms1 = dv.dataToUint32();
            if (irms1 != (uint32_t)-1)
                this->onNewInstCurrentMeasurement(irms1);
        }
        /* Search for PREF */
        else if (dv.labelEquals("PREF")) {
            uint32_t pref = dv.dataToUint32();
            if (pref != (uint32_t)-1)
                this->onRefPowerInfo(pref);
        }
        else if (dv.labelEquals("PMAX") || dv.labelEquals("SMAXSN")) {
            uint32_t pmax = dv.dataToUint32();
            if (pmax != (uint32_t)-1)
                this->onMaxPowerInfo(pmax);
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
