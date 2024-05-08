#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <stdint.h>

#include "Timestamp.h"
#include "TIC/DatasetView.h"

TEST(Timestamp_tests, DefaultInstanciation) {
    Timestamp ts;

    EXPECT_FALSE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(Timestamp_tests, ConstructFromHMS) {
    Timestamp ts(23, 59, 25);

    EXPECT_TRUE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(59, ts.minute);
    EXPECT_FALSE(ts.knownMilliseconds);
    EXPECT_EQ(25, ts.second);
}

TEST(Timestamp_tests, ConstructFromHMSMs) {
    Timestamp ts(23, 59, 25, 256);

    EXPECT_TRUE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(25, ts.second);
    EXPECT_TRUE(ts.knownMilliseconds);
    EXPECT_EQ(256, ts.millisecond);
}

TEST(Timestamp_tests, ConstructFromMDHMS) {
    Timestamp ts(11, 7, 23, 59, 25);

    EXPECT_TRUE(ts.isValid);
    EXPECT_TRUE(ts.knownDate);
    EXPECT_EQ(11, ts.month);
    EXPECT_EQ(7, ts.day);
    EXPECT_EQ(23, ts.hour);
    EXPECT_EQ(59, ts.minute);
    EXPECT_EQ(25, ts.second);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(Timestamp_tests, ConstructFromMDHMSMs) {
    Timestamp ts(11, 7, 23, 59, 25, 256);

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

TEST(Timestamp_tests, ImpossibleHour) {
	Timestamp ts(24, 0, 0);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (hour 24)";
	}
}

TEST(Timestamp_tests, ImpossibleMinute) {
	Timestamp ts(23, 60, 0);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (minute 60)";
	}
}

TEST(Timestamp_tests, ImpossibleSecond) {
	Timestamp ts(23, 0, 60);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (second 60)";
	}
}

TEST(Timestamp_tests, ImpossibleMillisecond) {
	Timestamp ts(23, 0, 0, 1000);

	if (ts.isValid) {
		FAIL() << "Expected invalid timestamp (millisecond 1000)";
	}
}

TEST(Timestamp_tests, ImpossibleMonth13) {
	Timestamp ts(13, 1, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (month 13)";
	}
}

TEST(Timestamp_tests, ImpossibleMonth0) {
	Timestamp ts(0, 1, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (month 0)";
	}
}

TEST(Timestamp_tests, ImpossibleDay0) {
	Timestamp ts(1, 0, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (day 0)";
	}
}

TEST(Timestamp_tests, ImpossibleDay30Februrary) {
	Timestamp ts(2, 30, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (30th of February)";
	}
}

TEST(Timestamp_tests, ImpossibleDay31June) {
	Timestamp ts(6, 31, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (31st of June)";
	}
}

TEST(Timestamp_tests, ImpossibleDay31November) {
	Timestamp ts(11, 31, 23, 0, 0);

	if (ts.isValid || ts.knownDate) {
		FAIL() << "Expected invalid timestamp (31st of November)";
	}
}

TEST(Timestamp_tests, AllowedDay31August) {
	Timestamp ts(8, 31, 23, 0, 0);

	if (!ts.isValid || !ts.knownDate) {
		FAIL() << "Expected valid timestamp (31st of August)";
	}
}

TEST(Timestamp_tests, AllowedDay29February) {
	Timestamp ts(2, 29, 23, 0, 0);

	if (!ts.isValid || !ts.knownDate) {
		FAIL() << "Expected valid timestamp (29th of Februrary)";
	}
}

TEST(Timestamp_tests, ConstructFromGoodHorodate) {
    char sampleHorodateAsCString[] = "E090714074553";
	TIC::Horodate h = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
    Timestamp ts(h);

    EXPECT_TRUE(ts.isValid);
    EXPECT_TRUE(ts.knownDate);
    EXPECT_EQ(7, ts.month);
    EXPECT_EQ(14, ts.day);
    EXPECT_EQ(7, ts.hour);
    EXPECT_EQ(45, ts.minute);
    EXPECT_EQ(53, ts.second);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(Timestamp_tests, ConstructFromVoidHorodate) {
	TIC::Horodate h;
    Timestamp ts(h);

    EXPECT_FALSE(ts.isValid);
    EXPECT_FALSE(ts.knownDate);
    EXPECT_FALSE(ts.knownMilliseconds);
}

TEST(Timestamp_tests, PureEquality) {
    Timestamp ts1(11, 7, 23, 59, 25, 256);
    Timestamp ts2(11, 7, 23, 59, 25, 256);

	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
}

TEST(Timestamp_tests, CopyConstructionLeadsToEquality) {
    Timestamp ts1(11, 7, 23, 59, 25, 256);
    Timestamp ts2(ts1);

	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
}

TEST(Timestamp_tests, EqualityIgnoresUnknownMilliseconds) {
    Timestamp ts1(11, 7, 23, 59, 25);
    Timestamp ts2(11, 7, 23, 59, 25, 256);

	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
}

TEST(Timestamp_tests, EqualityComparesKnownMilliseconds) {
    Timestamp ts1(11, 7, 23, 59, 25, 255);
    Timestamp ts2(11, 7, 23, 59, 25, 256);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(Timestamp_tests, DifferenceOnSeconds) {
    Timestamp ts1(11, 7, 23, 59, 25);
    Timestamp ts2(11, 7, 23, 59, 26);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(Timestamp_tests, DifferenceOnMinutes) {
    Timestamp ts1(23, 58, 25);
    Timestamp ts2(23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(Timestamp_tests, DifferenceOnHours) {
    Timestamp ts1(11, 7, 22, 59, 25);
    Timestamp ts2(11, 7, 23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(Timestamp_tests, DifferenceOnDays) {
    Timestamp ts1(11, 7, 23, 59, 25);
    Timestamp ts2(11, 6, 23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(Timestamp_tests, DifferenceOnMonths) {
    Timestamp ts1(10, 7, 23, 59, 25);
    Timestamp ts2(11, 7, 23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(Timestamp_tests, DifferenceIfUnknownDateInOneOperand) {
    Timestamp ts1(11, 7, 23, 59, 25);
    Timestamp ts2(23, 59, 25);

	if (ts1 == ts2) {
		FAIL() << "Expected timestamp difference";
	}
	if (!(ts1 != ts2)) {
		FAIL() << "Expected timestamp difference";
	}
}

TEST(Timestamp_tests, EqualityIfUnknownDateInBothOperands) {
    Timestamp ts1(23, 59, 25);
    Timestamp ts2(23, 59, 25);

	if (ts1 != ts2) {
		FAIL() << "Expected timestamp equality";
	}
	if (!(ts1 == ts2)) {
		FAIL() << "Expected timestamp equality";
	}
}

void expect_timestamp2_stricly_greater_than_timestamp1(const Timestamp& horodate1, const Timestamp& horodate2) {
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

TEST(Timestamp_tests, Difference1s) {
    Timestamp ts1(23, 59, 25);
    Timestamp ts2(23, 59, 26);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(Timestamp_tests, Difference1min) {
    Timestamp ts1(23, 58, 25);
    Timestamp ts2(23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(Timestamp_tests, Difference1hour) {
    Timestamp ts1(22, 59, 25);
    Timestamp ts2(23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(Timestamp_tests, Difference1Day) {
    Timestamp ts1(1, 12, 23, 59, 25);
    Timestamp ts2(1, 13, 23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(Timestamp_tests, Difference1Month) {
    Timestamp ts1(1, 12, 23, 59, 25);
    Timestamp ts2(2, 12, 23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(Timestamp_tests, DifferenceUnknownDate) {
    Timestamp ts1(23, 59, 25);
    Timestamp ts2(1, 13, 23, 59, 25);

	expect_timestamp2_stricly_greater_than_timestamp1(ts1, ts2);
}

TEST(Timestamp_tests, addSecondsWrapDayNoWrap) {
    Timestamp ts(1, 13, 22, 58, 20);

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

TEST(Timestamp_tests, addSecondsWrapDayWrapWith1sIncrementWithKnownDate) {
    Timestamp ts(1, 13, 23, 59, 59);
    EXPECT_EQ(1, ts.addSecondsWrapDay(1)); // Now 13/1 00:00:00 (day has not been incremented but wrapped increment is notified with returned value true)
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(0, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(Timestamp_tests, addSecondsWrapDayWrapWith1sIncrementWithUnknownDate) {
    Timestamp ts(23, 59, 59);
    EXPECT_EQ(1, ts.addSecondsWrapDay(1)); // Now 13/1 00:00:00 (day has not been incremented but wrapped increment is notified via returned value)
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(0, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_FALSE(ts.knownDate);
}

TEST(Timestamp_tests, addSecondsWrapDayWrapWith1minIncrementWithKnownDate) {
    Timestamp ts(1, 13, 23, 59, 00);
    EXPECT_EQ(1, ts.addSecondsWrapDay(60)); // Now 13/1 00:00:00 (day has not been incremented but wrapped increment is notified via returned value)
    EXPECT_EQ(0, ts.second);
    EXPECT_EQ(0, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(Timestamp_tests, addSecondsWrapDayWrapWithLargeIncrementWithKnownDate) {
    Timestamp ts(1, 13, 23, 00, 00);
    EXPECT_EQ(1, ts.addSecondsWrapDay(3600 + 61)); // Now 13/1 00:01:01 (day has not been incremented but wrapped increment is notified via returned value)
    EXPECT_EQ(1, ts.second);
    EXPECT_EQ(1, ts.minute);
    EXPECT_EQ(0, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(Timestamp_tests, addSecondsWrapDayWrapMultiDayIncrementWithKnownDate) {
    Timestamp ts(1, 13, 7, 6, 5);
    EXPECT_EQ(3, ts.addSecondsWrapDay(3600*24*3)); // Now 13/1 00:01:01 (day has not been incremented but 3 day-increment is notified via returned value)
    EXPECT_EQ(5, ts.second);
    EXPECT_EQ(6, ts.minute);
    EXPECT_EQ(7, ts.hour);
    EXPECT_EQ(13, ts.day);
    EXPECT_EQ(1, ts.month);
}

TEST(Timestamp_tests, toSecondsFromMidnight) {
    Timestamp ts(0, 0, 0);
    EXPECT_EQ(0, ts.toSeconds());
}

TEST(Timestamp_tests, toSecondsFromMidnightWithMilliseconds) {
    Timestamp ts(0, 0, 0, 0);
    EXPECT_EQ(0, ts.toSeconds());
}

TEST(Timestamp_tests, toSecondsFromTimestamp) {
    Timestamp ts(3, 36, 51);
    EXPECT_EQ(13011, ts.toSeconds());
}

TEST(Timestamp_tests, toSecondsFromTimestampIgnoresMilliseconds) {
    Timestamp ts(3, 36, 51, 999);
    EXPECT_EQ(13011, ts.toSeconds());
}

TEST(Timestamp_tests, toSecondsFromInvalidTimestamp) {
    Timestamp ts;
    EXPECT_EQ(static_cast<unsigned int>(-1), ts.toSeconds());
}

TEST(Timestamp_tests, toSecondsFromMaxTimestamp) {
    Timestamp ts(23, 59, 59, 999);
    EXPECT_EQ(86399, ts.toSeconds());
}

TEST(Timestamp_tests, toSecondsIgnoresDate) {
    Timestamp ts1(5, 5, 7, 6, 5);
    Timestamp ts2(7, 6, 5, 998);
    EXPECT_EQ(ts1.toSeconds(), ts2.toSeconds());
}