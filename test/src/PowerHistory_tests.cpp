#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>
#include <climits>

#include "PowerHistory.h"

TEST(PowerHistoryEntry_tests, DefaultInstanciation) {
    PowerHistoryEntry phe;

    EXPECT_FALSE(phe.power.isValid);
    EXPECT_FALSE(phe.horodate.isValid);
    EXPECT_EQ(0, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, InstanciationFromPowerHorodate) {
    TicEvaluatedPower power(1000, 1000);
    char sampleHorodateAsCString[] = "e230502124903";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

    PowerHistoryEntry phe(power, horodate);
    EXPECT_EQ(horodate, phe.horodate);
    EXPECT_EQ(power, phe.power);
    EXPECT_EQ(1, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSampleTwoIdenticalValues) {
    char sampleHorodate1AsCString[] = "e230502124903";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));

    PowerHistoryEntry phe(TicEvaluatedPower(1000, 1000), horodate1);

    char sampleHorodate2AsCString[] = "e230502134903";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));

    phe.averageWithPowerSample(TicEvaluatedPower(1000, 1000), horodate2);

    EXPECT_EQ(horodate2, phe.horodate); /* Horodate should be updated to the last value imported to average */
    EXPECT_EQ(TicEvaluatedPower(1000, 1000), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSampleTwoDifferentValues) {
    char sampleHorodate1AsCString[] = "e230502124903";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));

    PowerHistoryEntry phe(TicEvaluatedPower(200, 200), horodate1);

    char sampleHorodate2AsCString[] = "e230502134903";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));

    phe.averageWithPowerSample(TicEvaluatedPower(2200, 2200), horodate2);

    EXPECT_EQ(horodate2, phe.horodate);
    EXPECT_EQ(TicEvaluatedPower(1200, 1200), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSampleTwoDifferentValuesNegativePositive) {
    char sampleHorodate1AsCString[] = "e220502124903";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));

    PowerHistoryEntry phe(TicEvaluatedPower(-2000, -2000), horodate1);

    char sampleHorodate2AsCString[] = "e230502134903";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));

    phe.averageWithPowerSample(TicEvaluatedPower(+1000, +1000), horodate2);

    EXPECT_EQ(horodate2, phe.horodate);
    EXPECT_EQ(TicEvaluatedPower(-500, -500), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSample2NegativePositiveRangeAverage) {
    char sampleHorodate1AsCString[] = "e220502124903";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));

    PowerHistoryEntry phe(TicEvaluatedPower(-2000, -1000), horodate1);

    char sampleHorodate2AsCString[] = "e230502134903";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));

    phe.averageWithPowerSample(TicEvaluatedPower(+1000, +1000), horodate2);

    EXPECT_EQ(horodate2, phe.horodate);
    EXPECT_EQ(TicEvaluatedPower(-500, 0), phe.power);
    EXPECT_EQ(2, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, AverageWithPowerSample7NegativePositiveRangeAverage) {
    PowerHistoryEntry phe;

    char sampleHorodate1AsCString[] = "e220502124903";
    signed int min1 = 500;
    signed int max1 = 500;
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min1, max1), horodate1);

    char sampleHorodate2AsCString[] = "e220502134903";
    signed int min2 = 1000;
    signed int max2 = 1000;
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min2, max2), horodate2);

    char sampleHorodate3AsCString[] = "e220502144903";
    signed int min3 = -1000;
    signed int max3 = -1000;
	TIC::Horodate horodate3 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate3AsCString), strlen(sampleHorodate3AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min3, max3), horodate3);

    signed int min4 = -1000;
    signed int max4 = 0;
    char sampleHorodate4AsCString[] = "e220502154903";
	TIC::Horodate horodate4 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate4AsCString), strlen(sampleHorodate4AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min4, max4), horodate4);

    signed int min5 = -3000;
    signed int max5 = -2000;
    char sampleHorodate5AsCString[] = "e220502164903";
	TIC::Horodate horodate5 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate5AsCString), strlen(sampleHorodate5AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min5, max5), horodate5);

    signed int min6 = -500;
    signed int max6 = -500;
    char sampleHorodate6AsCString[] = "e220502174903";
	TIC::Horodate horodate6 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate6AsCString), strlen(sampleHorodate6AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min6, max6), horodate6);

    signed int min7 = 750;
    signed int max7 = 750;
    char sampleHorodate7AsCString[] = "e220502184903";
	TIC::Horodate horodate7 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate7AsCString), strlen(sampleHorodate7AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min7, max7), horodate7);

    EXPECT_EQ(horodate7, phe.horodate);
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

    char sampleHorodate1AsCString[] = "e220502124903";
    signed int min1 = -2000;
    signed int max1 = -1000;
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min1, max1), horodate1);

    char sampleHorodate2AsCString[] = "e220502134903";
    signed int min2 = 1000;
    signed int max2 = 1000;
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min2, max2), horodate2);

    char sampleHorodate3AsCString[] = "e220502144903";
    signed int min3 = -3000;
    signed int max3 = -2000;
	TIC::Horodate horodate3 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate3AsCString), strlen(sampleHorodate3AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min3, max3), horodate3);

    signed int min4 = -1000;
    signed int max4 = 0;
    char sampleHorodate4AsCString[] = "e220502154903";
	TIC::Horodate horodate4 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate4AsCString), strlen(sampleHorodate4AsCString));
    phe.averageWithPowerSample(TicEvaluatedPower(min4, max4), horodate4);

    EXPECT_EQ(horodate4, phe.horodate);
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
    EXPECT_FALSE(ph.lastPowerHorodate.isValid);
    EXPECT_EQ(1, ph.getAveragingPeriodInSeconds());
}

TEST(PowerHistory_tests, DefaultInstanciationPer10Seconds) {
    PowerHistory ph(PowerHistory::Per10Seconds);

    EXPECT_EQ(PowerHistory::Per10Seconds, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerHorodate.isValid);
    EXPECT_EQ(10, ph.getAveragingPeriodInSeconds());
}

TEST(PowerHistory_tests, DefaultInstanciationPerMinute) {
    PowerHistory ph(PowerHistory::PerMinute);

    EXPECT_EQ(PowerHistory::PerMinute, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerHorodate.isValid);
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

    char sampleHorodateAsCString[] = "e220502124903";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
    ph.onNewPowerData(TicEvaluatedPower(100, 100), horodate, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[0].power);
    EXPECT_EQ(horodate, result[0].horodate);
}

TEST(PowerHistory_tests, unWrapOnNewPowerData) {
    PowerHistory ph(PowerHistory::PerSecond);
    PowerHistoryEntry result[5];

    char sampleHorodateAsCString[] = "e220502124903";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
    PowerHistory::unWrapOnNewPowerData(TicEvaluatedPower(100, 100), horodate, 1, static_cast<void *>(&ph));

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[0].power);
    EXPECT_EQ(horodate, result[0].horodate);
}

TEST(PowerHistory_tests, PeriodPerSecondWithThreeSamplesInPeriod) {
    PowerHistory ph(PowerHistory::PerSecond);
    PowerHistoryEntry result[5];

    char sampleHorodateAsCString[] = "e220502124903";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
    ph.onNewPowerData(TicEvaluatedPower(100, 100), horodate, 1);
    ph.onNewPowerData(TicEvaluatedPower(200, 200), horodate, 1);
    ph.onNewPowerData(TicEvaluatedPower(300, 300), horodate, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(200, 200), result[0].power);
    EXPECT_EQ(horodate, result[0].horodate);
}

TEST(PowerHistory_tests, PeriodPer10SecondsWithOneSampleInPeriod) {
    PowerHistory ph(PowerHistory::Per10Seconds);
    PowerHistoryEntry result[5];

    char sampleHorodateAsCString[] = "e220502124903";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
    ph.onNewPowerData(TicEvaluatedPower(100, 100), horodate, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[0].power);
    EXPECT_EQ(horodate, result[0].horodate);
}

TEST(PowerHistory_tests, PeriodPer10SecondsWithThreeSamplesInPeriod) {
    PowerHistory ph(PowerHistory::Per10Seconds);
    PowerHistoryEntry result[5];

    char sampleHorodate1AsCString[] = "e230510000001";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));

    char sampleHorodate2AsCString[] = "e230510000007";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));

    char sampleHorodate3AsCString[] = "e230510000009";
	TIC::Horodate horodate3 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate3AsCString), strlen(sampleHorodate3AsCString));

    ph.onNewPowerData(TicEvaluatedPower(100, 100), horodate1, 1);
    ph.onNewPowerData(TicEvaluatedPower(200, 200), horodate2, 1);
    ph.onNewPowerData(TicEvaluatedPower(300, 300), horodate3, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(1, nb);
    EXPECT_EQ(TicEvaluatedPower(200, 200), result[0].power);
    EXPECT_EQ(horodate3, result[0].horodate);
}

TEST(PowerHistory_tests, PeriodPer10SecondsWithOneThenTwoSamplesInPeriod) {
    PowerHistory ph(PowerHistory::Per10Seconds);
    PowerHistoryEntry result[5];

    char sampleHorodate1AsCString[] = "e230510000001";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));

    char sampleHorodate2AsCString[] = "e230510000017";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));

    char sampleHorodate3AsCString[] = "e230510000019";
	TIC::Horodate horodate3 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate3AsCString), strlen(sampleHorodate3AsCString));

    ph.onNewPowerData(TicEvaluatedPower(100, 100), horodate1, 1);
    ph.onNewPowerData(TicEvaluatedPower(200, 200), horodate2, 1);
    ph.onNewPowerData(TicEvaluatedPower(300, 300), horodate3, 1);

    unsigned int nb = static_cast<unsigned int>(sizeof(result)/sizeof(result[0]));
    ph.getLastPower(nb, result);

    EXPECT_EQ(2, nb);
    EXPECT_EQ(TicEvaluatedPower(250, 250), result[0].power);
    EXPECT_EQ(horodate3, result[0].horodate);
    EXPECT_EQ(TicEvaluatedPower(100, 100), result[1].power);
    EXPECT_EQ(horodate1, result[1].horodate);
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
    EXPECT_FALSE(ph.lastPowerHorodate.isValid);
    EXPECT_EQ(10, ph.getAveragingPeriodInSeconds());
}

TEST(PowerHistory_tests, AveragingPeriodPerMinute) {
    PowerHistory ph(PowerHistory::PerMinute);

    EXPECT_EQ(PowerHistory::PerMinute, ph.averagingPeriod);
    EXPECT_EQ(0, ph.data.getCount());
    EXPECT_FALSE(ph.lastPowerHorodate.isValid);
    EXPECT_EQ(60, ph.getAveragingPeriodInSeconds());
}
