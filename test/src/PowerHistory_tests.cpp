#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>
#include <climits>

#include "PowerHistory.h"
#include "TimeOfDay.h"

TEST(PowerHistoryEntry_tests, DefaultInstanciation) {
    PowerHistoryEntry phe;

    EXPECT_FALSE(phe.power.isValid);
    EXPECT_FALSE(phe.timestamp.isValid);
    EXPECT_EQ(0, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, InstanciationFromPowerHorodate) {
    TicEvaluatedPower power(1000, 1000);
    TimeOfDay tod(5, 2, 12, 49, 03);

    PowerHistoryEntry phe(power, tod);
    EXPECT_EQ(tod, phe.timestamp);
    EXPECT_EQ(power, phe.power);
    EXPECT_EQ(1, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSampleTwoIdenticalValues) {
    TimeOfDay tod1(5, 2, 12, 49, 03);

    PowerHistoryEntry phe(TicEvaluatedPower(1000, 1000), tod1);

    TimeOfDay tod2(5, 2, 13, 49, 03);

    phe.averageWithPowerSample(TicEvaluatedPower(1000, 1000), tod2);

    EXPECT_EQ(tod2, phe.timestamp); /* Horodate should be updated to the last value imported to average */
    EXPECT_EQ(TicEvaluatedPower(1000, 1000), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSampleTwoDifferentValues) {
    TimeOfDay tod1(5, 2, 12, 49, 03);

    PowerHistoryEntry phe(TicEvaluatedPower(200, 200), tod1);

    TimeOfDay tod2(5, 2, 13, 49, 03);

    phe.averageWithPowerSample(TicEvaluatedPower(2200, 2200), tod2);

    EXPECT_EQ(tod2, phe.timestamp);
    EXPECT_EQ(TicEvaluatedPower(1200, 1200), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSampleTwoDifferentValuesNegativePositive) {
    TimeOfDay tod1(5, 2, 12, 49, 03);

    PowerHistoryEntry phe(TicEvaluatedPower(-2000, -2000), tod1);

    TimeOfDay tod2(5, 2, 13, 49, 03);

    phe.averageWithPowerSample(TicEvaluatedPower(+1000, +1000), tod2);

    EXPECT_EQ(tod2, phe.timestamp);
    EXPECT_EQ(TicEvaluatedPower(-500, -500), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSample2NegativePositiveRangeAverage) {
    TimeOfDay tod1(5, 2, 12, 49, 03);

    PowerHistoryEntry phe(TicEvaluatedPower(-2000, -1000), tod1);

    TimeOfDay tod2(5, 2, 13, 49, 03);

    phe.averageWithPowerSample(TicEvaluatedPower(+1000, +1000), tod2);

    EXPECT_EQ(tod2, phe.timestamp);
    EXPECT_EQ(TicEvaluatedPower(-500, 0), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSample7NegativePositiveRangeAverage) {
    PowerHistoryEntry phe;

    TimeOfDay tod1(5, 2, 12, 49, 03);
    signed int min1 = 500;
    signed int max1 = 500;
    phe.averageWithPowerSample(TicEvaluatedPower(min1, max1), tod1);

    TimeOfDay tod2(5, 2, 13, 49, 03);
    signed int min2 = 1000;
    signed int max2 = 1000;
    phe.averageWithPowerSample(TicEvaluatedPower(min2, max2), tod2);

    TimeOfDay tod3(5, 2, 14, 49, 03);
    signed int min3 = -1000;
    signed int max3 = -1000;
    phe.averageWithPowerSample(TicEvaluatedPower(min3, max3), tod3);

    TimeOfDay tod4(5, 2, 15, 49, 03);
    signed int min4 = -1000;
    signed int max4 = 0;
    phe.averageWithPowerSample(TicEvaluatedPower(min4, max4), tod4);

    TimeOfDay tod5(5, 2, 16, 49, 03);
    signed int min5 = -3000;
    signed int max5 = -2000;
    phe.averageWithPowerSample(TicEvaluatedPower(min5, max5), tod5);

    TimeOfDay tod6(5, 2, 17, 49, 03);
    signed int min6 = -500;
    signed int max6 = -500;
    phe.averageWithPowerSample(TicEvaluatedPower(min6, max6), tod6);

    signed int min7 = 750;
    signed int max7 = 750;
    TimeOfDay tod7(5, 2, 18, 49, 03);
    phe.averageWithPowerSample(TicEvaluatedPower(min7, max7), tod7);

    EXPECT_EQ(tod7, phe.timestamp);
    signed int expectedAvgMin = (min1 + min2 + min3 + min4 + min5 + min6 + min7) / 7;
    signed int expectedAvgMax = (max1 + max2 + max3 + max4 + max5 + max6 + max7) / 7;
    EXPECT_TRUE(phe.power.isValid);
    EXPECT_FALSE(phe.power.isExact);
    EXPECT_EQ(7, phe.nbSamples);
    if (phe.power.minValue < expectedAvgMin - 2 || phe.power.minValue > expectedAvgMin + 2) { /* We tolereate +/-2 errors due to cumulative rounding */
        FAIL() << "Error on power min boundary: got " << phe.power.minValue << ", expected " << expectedAvgMin << "+/-2";
    }
    if (phe.power.maxValue < expectedAvgMax - 2 || phe.power.maxValue > expectedAvgMax + 2) { /* We tolereate +/-2 errors due to cumulative rounding */
        FAIL() << "Error on power max boundary: got " << phe.power.maxValue << ", expected " << expectedAvgMax << "+/-2";
    }
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSample4NegativePositiveRangeAverage) {
    PowerHistoryEntry phe;

    TimeOfDay tod1(5, 2, 12, 49, 03);
    signed int min1 = -2000;
    signed int max1 = -1000;
    phe.averageWithPowerSample(TicEvaluatedPower(min1, max1), tod1);

    TimeOfDay tod2(5, 2, 13, 49, 03);
    signed int min2 = 1000;
    signed int max2 = 1000;
    phe.averageWithPowerSample(TicEvaluatedPower(min2, max2), tod2);

    TimeOfDay tod3(5, 2, 14, 49, 03);
    signed int min3 = -3000;
    signed int max3 = -2000;
    phe.averageWithPowerSample(TicEvaluatedPower(min3, max3), tod3);

    TimeOfDay tod4(5, 2, 15, 49, 03);
    signed int min4 = -1000;
    signed int max4 = 0;
    phe.averageWithPowerSample(TicEvaluatedPower(min4, max4), tod4);

    EXPECT_EQ(tod4, phe.timestamp);
    signed int expectedAvgMin = (min1 + min2 + min3 + min4) / 4;
    signed int expectedAvgMax = (max1 + max2 + max3 + max4) / 4;
    EXPECT_TRUE(phe.power.isValid);
    EXPECT_FALSE(phe.power.isExact);
    EXPECT_EQ(4, phe.nbSamples);
    if (phe.power.minValue < expectedAvgMin - 2 || phe.power.minValue > expectedAvgMin + 2) { /* We tolereate +/-2 errors due to cumulative rounding */
        FAIL() << "Error on power min boundary: got " << phe.power.minValue << ", expected " << expectedAvgMin << "+/-2";
    }
    if (phe.power.maxValue < expectedAvgMax - 2 || phe.power.maxValue > expectedAvgMax + 2) { /* We tolereate +/-2 errors due to cumulative rounding */
        FAIL() << "Error on power max boundary: got " << phe.power.maxValue << ", expected " << expectedAvgMax << "+/-2";
    }
}

TEST(PowerHistoryEntry_tests, truncateSignedLongToSignedInt) {
    EXPECT_EQ(0, PowerHistoryEntry::truncateSignedLongToSignedInt(0));
    EXPECT_EQ(1, PowerHistoryEntry::truncateSignedLongToSignedInt(1));
    EXPECT_EQ(-1, PowerHistoryEntry::truncateSignedLongToSignedInt(-1));
    EXPECT_EQ(INT_MAX, PowerHistoryEntry::truncateSignedLongToSignedInt(INT_MAX));
    EXPECT_EQ(INT_MIN, PowerHistoryEntry::truncateSignedLongToSignedInt(INT_MIN));
    EXPECT_EQ(INT_MAX, PowerHistoryEntry::truncateSignedLongToSignedInt(static_cast<signed long>(INT_MAX)+1));
    EXPECT_EQ(INT_MIN, PowerHistoryEntry::truncateSignedLongToSignedInt(static_cast<signed long>(INT_MIN)-1));
    EXPECT_EQ(INT_MAX, PowerHistoryEntry::truncateSignedLongToSignedInt(LONG_MAX));
    EXPECT_EQ(INT_MIN, PowerHistoryEntry::truncateSignedLongToSignedInt(LONG_MIN));
}

TEST(PowerHistory_tests, DefaultInstanciationPerSecond) {
    PowerHistory ph(PowerHistory::PerSecond);

    EXPECT_EQ(PowerHistory::PerSecond, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerTimeOfDay.isValid);
    EXPECT_EQ(1, ph.getAveragingPeriodInSeconds());
}

TEST(PowerHistory_tests, DefaultInstanciationPer10Seconds) {
    PowerHistory ph(PowerHistory::Per10Seconds);

    EXPECT_EQ(PowerHistory::Per10Seconds, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerTimeOfDay.isValid);
    EXPECT_EQ(10, ph.getAveragingPeriodInSeconds());
}

TEST(PowerHistory_tests, DefaultInstanciationPerMinute) {
    PowerHistory ph(PowerHistory::PerMinute);

    EXPECT_EQ(PowerHistory::PerMinute, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerTimeOfDay.isValid);
    EXPECT_EQ(60, ph.getAveragingPeriodInSeconds());
}

TEST(PowerHistory_tests, getLastPowerOnEmptyDatabase) {
    PowerHistory ph(PowerHistory::PerSecond);
    PowerHistoryEntry result;

    EXPECT_EQ(PowerHistory::PerSecond, ph.averagingPeriod);
    unsigned int nb = 1;
    ph.getLastPower(nb, &result);
    EXPECT_EQ(0, nb);
}

TEST(PowerHistory_tests, PeriodPerSecondWithOneSampleInPeriod) {
    PowerHistory ph(PowerHistory::PerSecond);
    PowerHistoryEntry result[5];

    TimeOfDay timestamp(5, 2, 12, 49, 03);
    ph.onNewPowerData(TicEvaluatedPower(100, 100), timestamp, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[0].power);
    EXPECT_EQ(timestamp, result[0].timestamp);
}

TEST(PowerHistory_tests, unWrapOnNewPowerData) {
    PowerHistory ph(PowerHistory::PerSecond);
    PowerHistoryEntry result[5];

    TimeOfDay timestamp(5, 2, 12, 49, 03);
    PowerHistory::unWrapOnNewPowerData(TicEvaluatedPower(100, 100), timestamp, 1, static_cast<void *>(&ph));

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[0].power);
    EXPECT_EQ(timestamp, result[0].timestamp);
}

TEST(PowerHistory_tests, PeriodPerSecondWithThreeSamplesInPeriod) {
    PowerHistory ph(PowerHistory::PerSecond);
    PowerHistoryEntry result[5];

    TimeOfDay timestamp(5, 2, 12, 49, 03);
    ph.onNewPowerData(TicEvaluatedPower(100, 100), timestamp, 1);
    ph.onNewPowerData(TicEvaluatedPower(200, 200), timestamp, 1);
    ph.onNewPowerData(TicEvaluatedPower(300, 300), timestamp, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(200, 200), result[0].power);
    EXPECT_EQ(timestamp, result[0].timestamp);
}

TEST(PowerHistory_tests, PeriodPer10SecondsWithOneSampleInPeriod) {
    PowerHistory ph(PowerHistory::Per10Seconds);
    PowerHistoryEntry result[5];

    TimeOfDay timestamp(5, 2, 12, 49, 03);
    ph.onNewPowerData(TicEvaluatedPower(100, 100), timestamp, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[0].power);
    EXPECT_EQ(timestamp, result[0].timestamp);
}

TEST(PowerHistory_tests, PeriodPer10SecondsWithThreeSamplesInPeriod) {
    PowerHistory ph(PowerHistory::Per10Seconds);
    PowerHistoryEntry result[5];

    TimeOfDay tod1(5, 10, 0, 0, 1);
    TimeOfDay tod2(5, 10, 0, 0, 7);
    TimeOfDay tod3(5, 10, 0, 0, 9);

    ph.onNewPowerData(TicEvaluatedPower(100, 100), tod1, 1);
    ph.onNewPowerData(TicEvaluatedPower(200, 200), tod2, 1);
    ph.onNewPowerData(TicEvaluatedPower(300, 300), tod3, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(200, 200), result[0].power);
    EXPECT_EQ(tod3, result[0].timestamp);
}

TEST(PowerHistory_tests, PeriodPer10SecondsWithOneThenTwoSamplesInPeriod) {
    PowerHistory ph(PowerHistory::Per10Seconds);
    PowerHistoryEntry result[5];

    TimeOfDay tod1(5, 10, 0, 0, 1);
    TimeOfDay tod2(5, 10, 0, 0, 17);
    TimeOfDay tod3(5, 10, 0, 0, 19);

    ph.onNewPowerData(TicEvaluatedPower(100, 100), tod1, 1);
    ph.onNewPowerData(TicEvaluatedPower(200, 200), tod2, 1);
    ph.onNewPowerData(TicEvaluatedPower(300, 300), tod3, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(2, nb);
    EXPECT_EQ(TicEvaluatedPower(250, 250), result[0].power);
    EXPECT_EQ(tod3, result[0].timestamp);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[1].power);
    EXPECT_EQ(tod1, result[1].timestamp);
}

TEST(PowerHistory_tests, PeriodPer5SecondsWith9SuccessiveSamples) {
    PowerHistoryEntry result[5];

    typedef struct {
        std::string horodateAsStr;
        int minPower;
        int maxPower;
    } TestPowerData;

    TestPowerData samplePowerData[] = {
        {.horodateAsStr = "e230510141801", .minPower=-500, .maxPower=-250 }, /* First sampled period */
        {.horodateAsStr = "e230510141801", .minPower=-500, .maxPower=-250 },
        {.horodateAsStr = "e230510141802", .minPower=-500, .maxPower=-250 },
        {.horodateAsStr = "e230510141803", .minPower=-500, .maxPower=-250 },

        {.horodateAsStr = "e230510141805", .minPower=1000, .maxPower=1000 }, /* Second sampled period */
        {.horodateAsStr = "e230510141806", .minPower=500, .maxPower=500 },
        {.horodateAsStr = "e230510141807", .minPower=250, .maxPower=250 },
        {.horodateAsStr = "e230510141807", .minPower=-250, .maxPower=-250 },
        {.horodateAsStr = "e230510141809", .minPower=-500, .maxPower=-500 },

        {.horodateAsStr = "e230510141810", .minPower=0, .maxPower=0 }, /* Third sampled period */
    };

    PowerHistory ph(PowerHistory::Per5Seconds);

    for (unsigned int idx = 0; idx < sizeof(samplePowerData)/sizeof(samplePowerData[0]); idx++) {
        TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<const uint8_t*>(samplePowerData[idx].horodateAsStr.c_str()), samplePowerData[idx].horodateAsStr.length());
        TicEvaluatedPower evaluatedPower(samplePowerData[idx].minPower, samplePowerData[idx].maxPower);
        ph.onNewPowerData(evaluatedPower, horodate, 1);
    }

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);
    EXPECT_EQ(3, nb);
    if (result[2].power.minValue != -500) {
        FAIL() << "Error on power min boundary: got " << result[2].power.minValue << ", expected " << -500;
    }
    if (result[2].power.maxValue != -250) {
        FAIL() << "Error on power max boundary: got " << result[2].power.maxValue << ", expected " << -250;
    }
    int expectedAvgMin = 200;
    int expectedAvgMax = 200;
    if (result[1].power.minValue < expectedAvgMin - 2 || result[1].power.minValue > expectedAvgMin + 2) { /* We tolereate +/-2 errors due to cumulative rounding */
        FAIL() << "Error on power min boundary: got " << result[1].power.minValue << ", expected " << expectedAvgMin << "+/-2";
    }
    if (result[1].power.maxValue < expectedAvgMax - 2 || result[1].power.maxValue > expectedAvgMax + 2) { /* We tolereate +/-2 errors due to cumulative rounding */
        FAIL() << "Error on power max boundary: got " << result[1].power.maxValue << ", expected " << expectedAvgMax << "+/-2";
    }
    EXPECT_EQ(TicEvaluatedPower(0, 0), result[0].power);
}

TEST(PowerHistory_tests, getPowerRecordsPerHour) {
    EXPECT_EQ(60 * 60, PowerHistory(PowerHistory::PerSecond).getPowerRecordsPerHour());
    EXPECT_EQ(6 * 60, PowerHistory(PowerHistory::Per10Seconds).getPowerRecordsPerHour());
    EXPECT_EQ(60, PowerHistory(PowerHistory::PerMinute).getPowerRecordsPerHour());
    EXPECT_EQ(60 / 5, PowerHistory(PowerHistory::Per5Minutes).getPowerRecordsPerHour());
}

TEST(PowerHistory_tests, AveragingPeriodPer10Seconds) {
    PowerHistory ph(PowerHistory::Per10Seconds);

    EXPECT_EQ(PowerHistory::Per10Seconds, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerTimeOfDay.isValid);
    EXPECT_EQ(10, ph.getAveragingPeriodInSeconds());
}

TEST(PowerHistory_tests, AveragingPeriodPerMinute) {
    PowerHistory ph(PowerHistory::PerMinute);

    EXPECT_EQ(PowerHistory::PerMinute, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerTimeOfDay.isValid);
    EXPECT_EQ(60, ph.getAveragingPeriodInSeconds());
}
