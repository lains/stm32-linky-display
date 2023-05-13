#include "PowerHistory.h"

#include <climits>

PowerHistoryEntry::PowerHistoryEntry() :
    power(),
    horodate(),
    nbSamples(0)
{
}

PowerHistoryEntry::PowerHistoryEntry(const TicEvaluatedPower& power, const TIC::Horodate& horodate) :
    power(power),
    horodate(horodate),
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

void PowerHistoryEntry::averageWithPowerSample(const TicEvaluatedPower& power, const TIC::Horodate& horodate) {
    PowerHistoryEntry result;

    /* If any of the two provided data instances are invalid, discard it and directly return the second one */
    if (!this->power.isValid) {
        this->power = power;
        this->horodate = horodate;
        this->nbSamples = 1;
        return;
    }
    if (!power.isValid)
        return; /* New sample is invalid: do nothing, no new calculation should be performed */

    if (horodate > this->horodate)
        this->horodate = horodate;  /* Update our internal horodate */

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

PowerHistory::PowerHistory(AveragingMode averagingPeriod) :
    data(),
    averagingPeriod(averagingPeriod),
    lastPowerHorodate()
{
}

void PowerHistory::onNewPowerData(const TicEvaluatedPower& power, const TIC::Horodate& horodate) {
    if (!power.isValid || !horodate.isValid) {
        return;
    }
    if (this->horodatesAreInSamePeriodSample(horodate, this->lastPowerHorodate)) {
        PowerHistoryEntry* lastEntry = this->data.getPtrToLast();
        if (lastEntry != nullptr) {
            lastEntry->averageWithPowerSample(power, horodate);
            this->lastPowerHorodate = horodate;
            return;
        }
        /* If lastEntry is not valid, create a new entry by falling-through the following code */
    }
    this->data.push(PowerHistoryEntry(power, horodate)); /* First sample in this period */
    this->lastPowerHorodate = horodate;
}

bool PowerHistory::horodatesAreInSamePeriodSample(const TIC::Horodate& first, const TIC::Horodate& second) {
    if (first.year != second.year ||
        first.month != second.month ||
        first.day != second.day ||
        first.hour != second.hour)
        return false; /* Period is different whenever anything longer than one hour is different between the two horodates */
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
        return false; /* Period is different whenever the minute is different between the two horodates */
    unsigned int periodStepInSeconds = 0;
    switch (this->averagingPeriod) {
        case PerSecond:
            periodStepInSeconds = 1;
            break;
        case Per5Seconds:
            periodStepInSeconds = 5;
            break;
        case Per10Seconds:
            periodStepInSeconds = 10;
            break;
        case Per15Seconds:
            periodStepInSeconds = 15;
            break;
        case Per30Seconds:
            periodStepInSeconds = 30;
            break;
        default:    /* All other possible periods are over one minute, they have been handled above, this block should never be reached */
            return false;
    }
    return ((first.second / periodStepInSeconds) == (second.second / periodStepInSeconds)); /* return true only if we are in the same step, even if remainders (seconds) are different */
}

void PowerHistory::unWrapOnNewPowerData(const TicEvaluatedPower& power, const TIC::Horodate& horodate, void* context) {
    if (context == nullptr)
        return; /* Failsafe, discard if no context */
    PowerHistory* powerHistoryInstance = static_cast<PowerHistory*>(context);
    /* We have finished parsing a frame, if there is an open dataset, we should discard it and start over at the following frame */
    powerHistoryInstance->onNewPowerData(power, horodate);
}

void PowerHistory::getLastPower(unsigned int& nb, PowerHistoryEntry* result) const {
    if (nb > this->data.getCount())
        nb = this->data.getCount();   /* Saturate to the number of actual values we hold */
    for (unsigned int reversePos = 0; reversePos < nb; reversePos++) {
        result[reversePos] = this->data.getReverse(reversePos);
    }
}
