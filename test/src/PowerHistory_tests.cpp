#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>

#include "PowerHistory.h"

TEST(PowerHistoryEntry_tests, defaultInstanciation) {
    PowerHistoryEntry phe;

    EXPECT_FALSE(phe.power.isValid);
    EXPECT_FALSE(phe.horodate.isValid);
    EXPECT_EQ(0, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, instanciationFromPowerHorodate) {
    TicEvaluatedPower power(1000, 1000);
    char sampleHorodateAsCString[] = "e230502124903";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

    PowerHistoryEntry phe(power, horodate);
    EXPECT_EQ(horodate, phe.horodate);
    EXPECT_EQ(power, phe.power);
    EXPECT_EQ(1, phe.nbSamples);
}

TEST(PowerHistoryEntry_tests, averageWithPowerSampleTwoIdenticalValues) {
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

TEST(PowerHistoryEntry_tests, averageWithPowerSampleTwoDifferentValues) {
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

TEST(PowerHistoryEntry_tests, averageWithPowerSampleTwoDifferentValuesNegativePositive) {
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

TEST(PowerHistoryEntry_tests, averageWithPowerSample2NegativePositiveRangeAverage) {
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

TEST(PowerHistoryEntry_tests, averageWithPowerSample7NegativePositiveRangeAverage) {
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

TEST(PowerHistoryEntry_tests, averageWithPowerSample4NegativePositiveRangeAverage) {
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
