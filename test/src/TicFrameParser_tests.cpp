#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>

#include "TicFrameParser.h"

TEST(TicEvaluatedPower_tests, DefaultInstanciation) {
    TicEvaluatedPower tep;

    EXPECT_FALSE(tep.isValid);
}

TEST(TicEvaluatedPower_tests, InstanciationFromExactPositiveValue) {
    TicEvaluatedPower tep(100, 100);

    EXPECT_TRUE(tep.isValid);
    EXPECT_TRUE(tep.isExact);
    EXPECT_EQ(100, tep.minValue);
    EXPECT_EQ(100, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, InstanciationFromExactNegativeValue) {
    TicEvaluatedPower tep(-100, -100);

    EXPECT_TRUE(tep.isValid);
    EXPECT_TRUE(tep.isExact);
    EXPECT_EQ(-100, tep.minValue);
    EXPECT_EQ(-100, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, InstanciationFromOrderedStrictlyPositiveRange) {
    TicEvaluatedPower tep(100, 200);

    EXPECT_TRUE(tep.isValid);
    EXPECT_FALSE(tep.isExact);
    EXPECT_EQ(100, tep.minValue);
    EXPECT_EQ(200, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, InstanciationFromDisorderedStrictlyPositiveRange) {
    TicEvaluatedPower tep(200, 100);

    EXPECT_TRUE(tep.isValid);
    EXPECT_FALSE(tep.isExact);
    EXPECT_EQ(100, tep.minValue);
    EXPECT_EQ(200, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, InstanciationFromOrderedStrictlyNegativeRange) {
    TicEvaluatedPower tep(-200, -100);

    EXPECT_TRUE(tep.isValid);
    EXPECT_FALSE(tep.isExact);
    EXPECT_EQ(-200, tep.minValue);
    EXPECT_EQ(-100, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, InstanciationFromDisorderedStrictlyNegativeRange) {
    TicEvaluatedPower tep(-100, -200);

    EXPECT_TRUE(tep.isValid);
    EXPECT_FALSE(tep.isExact);
    EXPECT_EQ(-200, tep.minValue);
    EXPECT_EQ(-100, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, InstanciationFromOrderedRangeAround0) {
    TicEvaluatedPower tep(-200, 300);

    EXPECT_TRUE(tep.isValid);
    EXPECT_FALSE(tep.isExact);
    EXPECT_EQ(-200, tep.minValue);
    EXPECT_EQ(300, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, InstanciationFromDisorderedRangeAround0) {
    TicEvaluatedPower tep(300, -200);

    EXPECT_TRUE(tep.isValid);
    EXPECT_FALSE(tep.isExact);
    EXPECT_EQ(-200, tep.minValue);
    EXPECT_EQ(300, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, setMinMax) {
    TicEvaluatedPower tep(-1000, -1000);

    tep.setMinMax(500, 500);
    EXPECT_TRUE(tep.isValid);
    EXPECT_TRUE(tep.isExact);
    EXPECT_EQ(500, tep.minValue);
    EXPECT_EQ(500, tep.maxValue);

    tep.setMinMax(1000, -10);
    EXPECT_TRUE(tep.isValid);
    EXPECT_FALSE(tep.isExact);
    EXPECT_EQ(-10, tep.minValue);
    EXPECT_EQ(1000, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, set) {
    TicEvaluatedPower tep(-1000, 1000);

    tep.set(-10);
    EXPECT_TRUE(tep.isValid);
    EXPECT_TRUE(tep.isExact);
    EXPECT_EQ(-10, tep.minValue);
    EXPECT_EQ(-10, tep.maxValue);
}

TEST(TicEvaluatedPower_tests, swapWith) {
    TicEvaluatedPower tepWide(-1000, 1000);
    TicEvaluatedPower tepEmpty;

    tepEmpty.swapWith(tepWide);

    TicEvaluatedPower& newTepWide = tepEmpty;
    TicEvaluatedPower& newTepEmpty = tepWide;

    EXPECT_FALSE(newTepEmpty.isValid);
    EXPECT_TRUE(newTepWide.isValid);
    EXPECT_FALSE(newTepWide.isExact);
    EXPECT_EQ(-1000, newTepWide.minValue);
    EXPECT_EQ(1000, newTepWide.maxValue);

    /* Swap back but using std::swap() */
    std::swap(newTepWide, tepWide);

    EXPECT_FALSE(tepEmpty.isValid);
    EXPECT_TRUE(tepWide.isValid);
    EXPECT_FALSE(tepWide.isExact);
    EXPECT_EQ(-1000, tepWide.minValue);
    EXPECT_EQ(1000, tepWide.maxValue);
}

TEST(TicMeasurements_tests, DefaultInstanciation) {
    //FIXME: add UT here
}

TEST(TicFrameParser_tests, DefaultInstanciation) {
    //FIXME: add UT here
}