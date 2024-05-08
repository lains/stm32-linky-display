#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <stdint.h>

#include "TimeOfDay.h"
#include "TIC/DatasetView.h"

TEST(TimeOfDay_tests, DefaultInstanciation) {
    TimeOfDay ts;

    EXPECT_FALSE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(TimeOfDay_tests, ConstructFromHMS) {
    TimeOfDay ts(23, 59, 25);

    EXPECT_TRUE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(59, ts.minute);
    EXPECT_FALSE(ts.knownMilliseconds);
    EXPECT_EQ(25, ts.second);
}

TEST(TimeOfDay_tests, ConstructFromHMSMs) {
    TimeOfDay ts(23, 59, 25, 256);

    EXPECT_TRUE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(25, ts.second);
    EXPECT_TRUE(ts.knownMilliseconds);
    EXPECT_EQ(256, ts.millisecond);
}

TEST(TimeOfDay_tests, ConstructFromMDHMS) {
    TimeOfDay ts(11, 7, 23, 59, 25);

    EXPECT_TRUE(ts.isValid);
    EXPECT_TRUE(ts.knownDate);
    EXPECT_EQ(11, ts.month);
    EXPECT_EQ(7, ts.day);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(25, ts.second);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(TimeOfDay_tests, ConstructFromMDHMSMs) {
    TimeOfDay ts(11, 7, 23, 59, 25, 256);

    EXPECT_TRUE(ts.isValid);
    EXPECT_TRUE(ts.knownDate);
    EXPECT_EQ(11, ts.month);
    EXPECT_EQ(7, ts.day);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(25, ts.second);
    EXPECT_TRUE(ts.knownMilliseconds);
    EXPECT_EQ(256, ts.millisecond);
}

TEST(TimeOfDay_tests, ImpossibleHour) {
	TimeOfDay ts(24, 0, 0);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (hour 24)";
	}
}

TEST(TimeOfDay_tests, ImpossibleMinute) {
	TimeOfDay ts(23, 60, 0);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (minute 60)";
	}
}

TEST(TimeOfDay_tests, ImpossibleSecond) {
	TimeOfDay ts(23, 0, 60);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (second 60)";
	}
}

TEST(TimeOfDay_tests, ImpossibleMillisecond) {
	TimeOfDay ts(23, 0, 0, 1000);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (millisecond 1000)";
	}
}

TEST(TimeOfDay_tests, ImpossibleMonth13) {
	TimeOfDay ts(13, 1, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (month 13)";
	}
}

TEST(TimeOfDay_tests, ImpossibleMonth0) {
	TimeOfDay ts(0, 1, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (month 0)";
	}
}

TEST(TimeOfDay_tests, ImpossibleDay0) {
	TimeOfDay ts(1, 0, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (day 0)";
	}
}

TEST(TimeOfDay_tests, ImpossibleDay30Februrary) {
	TimeOfDay ts(2, 30, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (30th of February)";
	}
}

TEST(TimeOfDay_tests, ImpossibleDay31June) {
	TimeOfDay ts(6, 31, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (31st of June)";
	}
}

TEST(TimeOfDay_tests, ImpossibleDay31November) {
	TimeOfDay ts(11, 31, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (31st of November)";
	}
}

TEST(TimeOfDay_tests, AllowedDay31August) {
	TimeOfDay ts(8, 31, 23, 0, 0);

	if (!ts.isValid || !ts.knownDate) {
		FAIL() << "Expected valid timestamp (31st of August)";
	}
}

TEST(TimeOfDay_tests, AllowedDay29February) {
	TimeOfDay ts(2, 29, 23, 0, 0);

	if (!ts.isValid || !ts.knownDate) {
		FAIL() << "Expected valid timestamp (29th of Februrary)";
	}
}

TEST(TimeOfDay_tests, ConstructFromGoodHorodate) {
    char sampleHorodateAsCString[] = "E090714074553";
	TIC::Horodate h = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
    TimeOfDay ts(h);

    EXPECT_TRUE(ts.isValid);
    EXPECT_TRUE(ts.knownDate);
    EXPECT_EQ(7, ts.month);
    EXPECT_EQ(14, ts.day);
    EXPECT_EQ(7, ts.hour);
    EXPECT_EQ(45, ts.minute);
    EXPECT_EQ(53, ts.second);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(TimeOfDay_tests, ConstructFromVoidHorodate) {
	TIC::Horodate h;
    TimeOfDay ts(h);

    EXPECT_FALSE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(TimeOfDay_tests, PureEquality) {
    TimeOfDay ts1(11, 7, 23, 59, 25, 256);
    TimeOfDay ts2(11, 7, 23, 59, 25, 256);

	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
}

TEST(TimeOfDay_tests, CopyConstructionLeadsToEquality) {
    TimeOfDay ts1(11, 7, 23, 59, 25, 256);
    TimeOfDay ts2(ts1);

	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
}

TEST(TimeOfDay_tests, EqualityIgnoresUnknownMilliseconds) {
    TimeOfDay ts1(11, 7, 23, 59, 25);
    TimeOfDay ts2(11, 7, 23, 59, 25, 256);

	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
}

TEST(TimeOfDay_tests, EqualityComparesKnownMilliseconds) {
    TimeOfDay ts1(11, 7, 23, 59, 25, 255);
    TimeOfDay ts2(11, 7, 23, 59, 25, 256);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnSeconds) {
    TimeOfDay ts1(11, 7, 23, 59, 25);
    TimeOfDay ts2(11, 7, 23, 59, 26);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnMinutes) {
    TimeOfDay ts1(23, 58, 25);
    TimeOfDay ts2(23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnHours) {
    TimeOfDay ts1(11, 7, 22, 59, 25);
    TimeOfDay ts2(11, 7, 23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnDays) {
    TimeOfDay ts1(11, 7, 23, 59, 25);
    TimeOfDay ts2(11, 6, 23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnMonths) {
    TimeOfDay ts1(10, 7, 23, 59, 25);
    TimeOfDay ts2(11, 7, 23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(TimeOfDay_tests, DifferenceIfUnknownDateInOneOperand) {
    TimeOfDay ts1(11, 7, 23, 59, 25);
    TimeOfDay ts2(23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(TimeOfDay_tests, EqualityIfUnknownDateInBothOperands) {
    TimeOfDay ts1(23, 59, 25);
    TimeOfDay ts2(23, 59, 25);

	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
}

void expect_timestamp2_stricly_greater_than_timestamp1(const TimeOfDay& horodate1, const TimeOfDay& horodate2) {
	if (!(horodate2 > horodate1)) {
		FAIL() << "Expected horodate2>horodate1";
	}
	if (!(horodate2 >= horodate1)) {
		FAIL() << "Expected horodate2>=horodate1";
	}
	if (horodate2 <= horodate1) {
		FAIL() << "Expected horodate2>horodate1";
	}
	if (horodate2 < horodate1) {
		FAIL() << "Expected horodate2>horodate1";
	}
	if (horodate2 == horodate1) {
		FAIL() << "Expected horodate2!=horodate1";
	}
}

TEST(TimeOfDay_tests, Difference1s) {
    TimeOfDay ts1(23, 59, 25);
    TimeOfDay ts2(23, 59, 26);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(TimeOfDay_tests, Difference1min) {
    TimeOfDay ts1(23, 58, 25);
    TimeOfDay ts2(23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(TimeOfDay_tests, Difference1hour) {
    TimeOfDay ts1(22, 59, 25);
    TimeOfDay ts2(23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(TimeOfDay_tests, Difference1Day) {
    TimeOfDay ts1(1, 12, 23, 59, 25);
    TimeOfDay ts2(1, 13, 23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(TimeOfDay_tests, Difference1Month) {
    TimeOfDay ts1(1, 12, 23, 59, 25);
    TimeOfDay ts2(2, 12, 23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(TimeOfDay_tests, DifferenceUnknownDate) {
    TimeOfDay ts1(23, 59, 25);
    TimeOfDay ts2(1, 13, 23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(TimeOfDay_tests, addSecondsWrapDayNoWrap) {
    TimeOfDay ts(1, 13, 22, 58, 20);

    EXPECT_EQ(0, ts.addSecondsWrapDay(39)); // Now 13/1 22:58:59
    EXPECT_EQ(59, ts.second);
    EXPECT_EQ(0, ts.addSecondsWrapDay(1)); // Now 13/1 22:59:00
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(0, ts.addSecondsWrapDay(59)); // Now 13/1 22:59:59
    EXPECT_EQ(59, ts.second);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(22, ts.hour);
    EXPECT_EQ(0, ts.addSecondsWrapDay(1)); // Now 13/1 23:00:00
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(0, ts.minute);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(0, ts.addSecondsWrapDay(60 * 59)); // Now 13/1 23:59:00
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(0, ts.addSecondsWrapDay(59)); // Now 13/1 23:59:59
    EXPECT_EQ(59, ts.second);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(23, ts.hour);
}

TEST(TimeOfDay_tests, addSecondsWrapDayWrapWith1sIncrementWithKnownDate) {
    TimeOfDay ts(1, 13, 23, 59, 59);
    EXPECT_EQ(1, ts.addSecondsWrapDay(1)); // Now 13/1 00:00:00 (day has not been incremented but wrapped increment is notified with returned value true)
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(0, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(TimeOfDay_tests, addSecondsWrapDayWrapWith1sIncrementWithUnknownDate) {
    TimeOfDay ts(23, 59, 59);
    EXPECT_EQ(1, ts.addSecondsWrapDay(1)); // Now 13/1 00:00:00 (day has not been incremented but wrapped increment is notified via returned value)
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(0, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_FALSE(ts.knownDate);
}

TEST(TimeOfDay_tests, addSecondsWrapDayWrapWith1minIncrementWithKnownDate) {
    TimeOfDay ts(1, 13, 23, 59, 00);
    EXPECT_EQ(1, ts.addSecondsWrapDay(60)); // Now 13/1 00:00:00 (day has not been incremented but wrapped increment is notified via returned value)
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(0, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(TimeOfDay_tests, addSecondsWrapDayWrapWithLargeIncrementWithKnownDate) {
    TimeOfDay ts(1, 13, 23, 00, 00);
    EXPECT_EQ(1, ts.addSecondsWrapDay(3600 + 61)); // Now 13/1 00:01:01 (day has not been incremented but wrapped increment is notified via returned value)
    EXPECT_EQ(1, ts.second);
    EXPECT_EQ(1, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(TimeOfDay_tests, addSecondsWrapDayWrapMultiDayIncrementWithKnownDate) {
    TimeOfDay ts(1, 13, 7, 6, 5);
    EXPECT_EQ(3, ts.addSecondsWrapDay(3600*24*3)); // Now 13/1 00:01:01 (day has not been incremented but 3 day-increment is notified via returned value)
    EXPECT_EQ(5, ts.second);
    EXPECT_EQ(6, ts.minute);
    EXPECT_EQ(7, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(TimeOfDay_tests, toSecondsFromMidnight) {
    TimeOfDay ts(0, 0, 0);
    EXPECT_EQ(0, ts.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromMidnightWithMilliseconds) {
    TimeOfDay ts(0, 0, 0, 0);
    EXPECT_EQ(0, ts.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromTimeOfDay) {
    TimeOfDay ts(3, 36, 51);
    EXPECT_EQ(13011, ts.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromTimeOfDayIgnoresMilliseconds) {
    TimeOfDay ts(3, 36, 51, 999);
    EXPECT_EQ(13011, ts.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromInvalidTimeOfDay) {
    TimeOfDay ts;
    EXPECT_EQ(static_cast<unsigned int>(-1), ts.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromMaxTimeOfDay) {
    TimeOfDay ts(23, 59, 59, 999);
    EXPECT_EQ(86399, ts.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsIgnoresDate) {
    TimeOfDay ts1(5, 5, 7, 6, 5);
    TimeOfDay ts2(7, 6, 5, 998);
    EXPECT_EQ(ts1.toSeconds(), ts2.toSeconds());
}