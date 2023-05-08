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

// TEST(PowerHistoryEntry_tests, averageWithPowerSampleTwoDifferentValuesNegativePositive) {
//     char sampleHorodate1AsCString[] = "e220502124903";
// 	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate1AsCString), strlen(sampleHorodate1AsCString));

//     PowerHistoryEntry phe(TicEvaluatedPower(-2000, -2000), horodate1);

//     char sampleHorodate2AsCString[] = "e230502134903";
// 	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodate2AsCString), strlen(sampleHorodate2AsCString));

//     phe.averageWithPowerSample(TicEvaluatedPower(+1000, +1000), horodate2);

//     EXPECT_EQ(horodate2, phe.horodate);
//     EXPECT_EQ(TicEvaluatedPower(-500, -500), phe.power);
//     EXPECT_EQ(2, phe.nbSamples);
// }
