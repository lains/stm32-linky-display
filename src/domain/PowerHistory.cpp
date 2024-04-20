#include "PowerHistory.h"

#include <climits>

PowerHistoryEntry::PowerHistoryEntry() :
    power(),
    timestamp(),
    nbSamples(0)
{
}

PowerHistoryEntry::PowerHistoryEntry(const TicEvaluatedPower& power, const Timestamp& timestamp) :
    power(power),
    timestamp(timestamp),
    nbSamples(1)
{
}

signed int PowerHistoryEntry::truncateSignedLongToSignedInt(const signed long input) {
    if (input > INT_MAX) {
        return INT_MAX;
    }
    if (input < INT_MIN) {
        return INT_MIN;
    }
    if (input>0) {
        return static_cast<signed int>(input);
    }
    else {
        return -static_cast<signed int>(static_cast<unsigned int>(-input));
    }
}

void PowerHistoryEntry::averageWithPowerSample(const TicEvaluatedPower& power, const Timestamp& timestamp) {
    PowerHistoryEntry result;

    /* If any of the two provided data instances are invalid, discard it and directly return the second one */
    if (!this->power.isValid) {
        this->power = power;
        this->timestamp = timestamp;
        this->nbSamples = 1;
        return;
    }
    if (!power.isValid)
        return; /* New sample is invalid: do nothing, no new calculation should be performed */

    if (timestamp > this->timestamp)
        this->timestamp = timestamp;  /* Update our internal timestamp */

    if (this->power.isExact && power.isExact) {
        /* Averaging two exact measurements */
        unsigned int totalNbSample = this->nbSamples + 1;
        /* Note: in the line below, division should be done in a separate instruction rather than on one calculation line. */
        /* If we don't do that, on some buggy compilers, the result would overflow to high positive when dividing negative values */
        signed long int averagePower = static_cast<signed long int>(this->power.minValue) * this->nbSamples;
        averagePower += static_cast<signed long int>(power.minValue);
        averagePower /= static_cast<signed long int>(totalNbSample);
        this->power.set(truncateSignedLongToSignedInt(averagePower));
        this->nbSamples = totalNbSample;
        return;
    }

    /* Either first, second or both are no exact values but ranges, the calculation is a bit mode complex */
    unsigned int totalNbSample = this->nbSamples + 1;

    /* Note: in the lines below, division should be done in a separate instruction rather than on one calculation line. */
    /* If we don't do that, on some buggy compilers, the result would overflow to high positive when dividing negative values */
    signed long int averageMinPower = static_cast<signed long int>(this->power.minValue) * this->nbSamples;
    averageMinPower += static_cast<signed long int>(power.minValue);
    averageMinPower /= static_cast<signed long int>(totalNbSample);
    signed long int averageMaxPower = static_cast<signed long int>(this->power.maxValue) * this->nbSamples;
    averageMaxPower += static_cast<signed long int>(power.maxValue);
    averageMaxPower /= static_cast<signed long int>(totalNbSample);
    /* We now get a high and low boundary (a range) for power value */

    /* Prior average and/or the new value are estimations, take min and max as they have been calculated */
    this->power.setMinMax(truncateSignedLongToSignedInt(averageMinPower), truncateSignedLongToSignedInt(averageMaxPower));
    this->nbSamples = totalNbSample;
    return;
}

PowerHistory::PowerHistory(AveragingMode averagingPeriod, TicProcessingContext* context) :
    data(),
    averagingPeriod(averagingPeriod),
    ticContext(context),
    lastPowerTimestamp()
{
}

void PowerHistory::setContext(TicProcessingContext* context) {
    this->ticContext = context;
}


void PowerHistory::onNewPowerData(const TicEvaluatedPower& power, const Timestamp& timestamp, unsigned int frameSequenceNb) {
    if (!power.isValid || !timestamp.isValid) {
        return;
    }

    if (this->ticContext != nullptr) {
        this->ticContext->instantaneousPower = power;
        this->ticContext->lastParsedFrameNb = frameSequenceNb;
    }

    if (this->timestampsAreInSamePeriodSample(timestamp, this->lastPowerTimestamp)) {
        PowerHistoryEntry* lastEntry = this->data.getPtrToLast();
        if (lastEntry != nullptr) {
            lastEntry->averageWithPowerSample(power, timestamp);
            this->lastPowerTimestamp = timestamp;
            return;
        }
        /* If lastEntry is not valid, create a new entry by falling-through the following code */
    }
    if (false && this->lastPowerTimestamp.isValid) { /* If we already store some historical data, make sure last timestamp and new timestamp are consecutive (in periods), or otherwise pad */
        Timestamp forwardTimestamp = this->lastPowerTimestamp;
        for (unsigned int padding = 0; padding < this->data.getCapacity(); padding++) {
            forwardTimestamp.addSecondsWrapDay(this->getAveragingPeriodInSeconds());  /* Check what would happen if timestamp was in the next averaging period */
            if (this->timestampsAreInSamePeriodSample(timestamp, forwardTimestamp))
                break;
            /* If that timestamp slot does not match the timestamp passed as argument, we have a hole in our history entries, pad it */
            this->data.push(PowerHistoryEntry());   /* Push an invalid power history entry to pad the history */
        }
    }
    /* Warning: these lines are not reached when the new power is in the same average period as a previously valid entry (see the above return statement) */
    /* If code needs to be executed systematically, put it at the top of this function, not here... */
    this->data.push(PowerHistoryEntry(power, timestamp)); /* First sample in this period */
    this->lastPowerTimestamp = timestamp;
}

bool PowerHistory::timestampsAreInSamePeriodSample(const Timestamp& first, const Timestamp& second) {
    if (first.month != second.month ||
        first.day != second.day ||
        first.hour != second.hour)
        return false; /* Period is different whenever anything longer than one hour is different between the two timestamps */
    if (this->averagingPeriod >= AveraginOverMinutesOrMore) { /* We are averaging over at least 1 minute, so only compare minutes */
        switch (this->averagingPeriod) {
            case PerMinute:
                return (first.minute == second.minute); /* Minutes should be equal to be in the same sampling period */
            case Per5Minutes:
                if (first.minute > second.minute) {
                    return ((first.minute - second.minute) < 5);
                }
                else {
                    return ((second.minute - first.minute) < 5);
                }
            default:    /* Unsupported averaging period */
                return false;
        }
    }
    /* If we fall here, we are averaging over less than 1 minute */
    if (first.minute != second.minute)
        return false; /* Period is different whenever the minute is different between the two timestamps */
    unsigned int periodStepInSeconds = this->getAveragingPeriodInSeconds();
    return ((first.second / periodStepInSeconds) == (second.second / periodStepInSeconds)); /* return true only if we are in the same step, even if remainders (seconds) are different */
}

unsigned int PowerHistory::getAveragingPeriodInSeconds() const {
    switch (this->averagingPeriod) {
        case PerSecond:
            return 1;
        case Per5Seconds:
            return 5;
        case Per10Seconds:
            return 10;
        case Per15Seconds:
            return 15;
        case Per30Seconds:
            return 30;
        case PerMinute:
            return 60;
        case Per5Minutes:
            return 60*5;
        default:
            return 0;
    }
}

unsigned int PowerHistory::getPowerRecordsPerHour() const {
    unsigned int averagingPeriodInSeconds = this->getAveragingPeriodInSeconds();
    if (averagingPeriodInSeconds == 0)
        return 0;
    return (60 * 60 / averagingPeriodInSeconds);
}

void PowerHistory::unWrapOnNewPowerData(const TicEvaluatedPower& power, const Timestamp& timestamp, unsigned int frameSequenceNb, void* context) {
    if (context == nullptr)
        return; /* Failsafe, discard if no context */
    PowerHistory* powerHistoryInstance = static_cast<PowerHistory*>(context);
    /* We have finished parsing a frame, if there is an open dataset, we should discard it and start over at the following frame */
    powerHistoryInstance->onNewPowerData(power, timestamp, frameSequenceNb);
}

void PowerHistory::getLastPower(unsigned int& nb, PowerHistoryEntry* result) const {
    if (nb > this->data.getCount())
        nb = this->data.getCount();   /* Saturate to the number of actual values we hold */
    for (unsigned int reversePos = 0; reversePos < nb; reversePos++) {
        result[reversePos] = this->data.getReverse(reversePos);
    }
}
