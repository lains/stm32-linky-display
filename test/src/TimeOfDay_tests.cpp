#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <stdint.h>

#include "TimeOfDay.h"
#include "TIC/DatasetView.h"

TEST(TimeOfDay_tests, DefaultInstanciation) {
    TimeOfDay tod;

    EXPECT_FALSE(tod.isValid);
    EXPECT_FALSE(tod.knownMilliseconds);
}

TEST(TimeOfDay_tests, ConstructFromHMS) {
    TimeOfDay tod(23, 59, 25);

    EXPECT_TRUE(tod.isValid);
    EXPECT_EQ(23, tod.hour);
    EXPECT_EQ(59, tod.minute);
    EXPECT_FALSE(tod.knownMilliseconds);
    EXPECT_EQ(25, tod.second);
}

TEST(TimeOfDay_tests, ConstructFromHMSMs) {
    TimeOfDay tod(23, 59, 25, 256);

    EXPECT_TRUE(tod.isValid);
    EXPECT_EQ(23, tod.hour);
    EXPECT_EQ(59, tod.minute);
    EXPECT_EQ(25, tod.second);
    EXPECT_TRUE(tod.knownMilliseconds);
    EXPECT_EQ(256, tod.millisecond);
}

TEST(TimeOfDay_tests, ImpossibleHour) {
	TimeOfDay tod(24, 0, 0);

	if (tod.isValid) {
		FAIL() << "Expected invalid time of day (hour 24)";
	}
}

TEST(TimeOfDay_tests, ImpossibleMinute) {
	TimeOfDay tod(23, 60, 0);

	if (tod.isValid) {
		FAIL() << "Expected invalid time of day (minute 60)";
	}
}

TEST(TimeOfDay_tests, ImpossibleSecond) {
	TimeOfDay tod(23, 0, 60);

	if (tod.isValid) {
		FAIL() << "Expected invalid time of day (second 60)";
	}
}

TEST(TimeOfDay_tests, ImpossibleMillisecond) {
	TimeOfDay tod(23, 0, 0, 1000);

	if (tod.isValid) {
		FAIL() << "Expected invalid time of day (millisecond 1000)";
	}
}

TEST(TimeOfDay_tests, ConstructFromGoodHorodate) {
    char sampleHorodateAsCString[] = "E090714074553";
	TIC::Horodate h = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));
    TimeOfDay tod(h);

    EXPECT_TRUE(tod.isValid);
    EXPECT_EQ(7, tod.hour);
    EXPECT_EQ(45, tod.minute);
    EXPECT_EQ(53, tod.second);
    EXPECT_FALSE(tod.knownMilliseconds);
}

TEST(TimeOfDay_tests, ConstructFromVoidHorodate) {
	TIC::Horodate h;
    TimeOfDay tod(h);

    EXPECT_FALSE(tod.isValid);
    EXPECT_FALSE(tod.knownMilliseconds);
}

TEST(TimeOfDay_tests, PureEquality) {
    TimeOfDay tod1(23, 59, 25, 256);
    TimeOfDay tod2(23, 59, 25, 256);

	if (!(tod1 == tod2)) {
		FAIL() << "Expected time of day equality";
	}
	if (tod1 != tod2) {
		FAIL() << "Expected time of day equality";
	}
}

TEST(TimeOfDay_tests, CopyConstructionLeadsToEquality) {
    TimeOfDay tod1(23, 59, 25, 256);
    TimeOfDay tod2(tod1);

	if (!(tod1 == tod2)) {
		FAIL() << "Expected time of day equality";
	}
	if (tod1 != tod2) {
		FAIL() << "Expected time of day equality";
	}
}

TEST(TimeOfDay_tests, EqualityIgnoresUnknownMilliseconds) {
    TimeOfDay tod1(23, 59, 25);
    TimeOfDay tod2(23, 59, 25, 256);

	if (!(tod1 == tod2)) {
		FAIL() << "Expected time of day equality";
	}
	if (tod1 != tod2) {
		FAIL() << "Expected time of day equality";
	}
}

TEST(TimeOfDay_tests, EqualityComparesKnownMilliseconds) {
    TimeOfDay tod1(23, 59, 25, 255);
    TimeOfDay tod2(23, 59, 25, 256);

	if (tod1 == tod2) {
		FAIL() << "Expected time of day difference";
	}
	if (!(tod1 != tod2)) {
		FAIL() << "Expected time of day difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnSeconds) {
    TimeOfDay tod1(23, 59, 25);
    TimeOfDay tod2(23, 59, 26);

	if (tod1 == tod2) {
		FAIL() << "Expected time of day difference";
	}
	if (!(tod1 != tod2)) {
		FAIL() << "Expected time of day difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnMinutes) {
    TimeOfDay tod1(23, 58, 25);
    TimeOfDay tod2(23, 59, 25);

	if (tod1 == tod2) {
		FAIL() << "Expected time of day difference";
	}
	if (!(tod1 != tod2)) {
		FAIL() << "Expected time of day difference";
	}
}

TEST(TimeOfDay_tests, DifferenceOnHours) {
    TimeOfDay tod1(22, 59, 25);
    TimeOfDay tod2(23, 59, 25);

	if (tod1 == tod2) {
		FAIL() << "Expected time of day difference";
	}
	if (!(tod1 != tod2)) {
		FAIL() << "Expected time of day difference";
	}
}

void expect_tod2_stricly_greater_than_tod1(const TimeOfDay& tod1, const TimeOfDay& tod2) {
	if (!(tod2 > tod1)) {
		FAIL() << "Expected tod2>tod1";
	}
	if (!(tod2 >= tod1)) {
		FAIL() << "Expected tod2>=tod1";
	}
	if (tod2 <= tod1) {
		FAIL() << "Expected tod2>tod1";
	}
	if (tod2 < tod1) {
		FAIL() << "Expected tod2>tod1";
	}
	if (tod2 == tod1) {
		FAIL() << "Expected tod2!=tod1";
	}
}

TEST(TimeOfDay_tests, Difference1s) {
    TimeOfDay tod1(23, 59, 25);
    TimeOfDay tod2(23, 59, 26);

	expect_tod2_stricly_greater_than_tod1(tod1, tod2);
}

TEST(TimeOfDay_tests, Difference1min) {
    TimeOfDay tod1(23, 58, 25);
    TimeOfDay tod2(23, 59, 25);

	expect_tod2_stricly_greater_than_tod1(tod1, tod2);
}

TEST(TimeOfDay_tests, Difference1hour) {
    TimeOfDay tod1(22, 59, 25);
    TimeOfDay tod2(23, 59, 25);

	expect_tod2_stricly_greater_than_tod1(tod1, tod2);
}

TEST(TimeOfDay_tests, addSecondsNoWrap) {
    TimeOfDay tod(22, 58, 20);

    EXPECT_EQ(0, tod.addSeconds(39)); // Now 22:58:59
    EXPECT_EQ(59, tod.second);
    EXPECT_EQ(0, tod.addSeconds(1)); // Now 22:59:00
    EXPECT_EQ(0, tod.second);
    EXPECT_EQ(59, tod.minute);
    EXPECT_EQ(0, tod.addSeconds(59)); // Now 22:59:59
    EXPECT_EQ(59, tod.second);
    EXPECT_EQ(59, tod.minute);
    EXPECT_EQ(22, tod.hour);
    EXPECT_EQ(0, tod.addSeconds(1)); // Now 23:00:00
    EXPECT_EQ(0, tod.second);
    EXPECT_EQ(0, tod.minute);
    EXPECT_EQ(23, tod.hour);
    EXPECT_EQ(0, tod.addSeconds(60 * 59)); // Now 23:59:00
    EXPECT_EQ(0, tod.second);
    EXPECT_EQ(59, tod.minute);
    EXPECT_EQ(23, tod.hour);
    EXPECT_EQ(0, tod.addSeconds(59)); // Now 23:59:59
    EXPECT_EQ(59, tod.second);
    EXPECT_EQ(59, tod.minute);
    EXPECT_EQ(23, tod.hour);
}

TEST(TimeOfDay_tests, addSecondsWith1sIncrement) {
    TimeOfDay tod(23, 59, 59);
    EXPECT_EQ(1, tod.addSeconds(1)); // Now 00:00:00 (wrapped day is notified via returned value)
    EXPECT_EQ(0, tod.second);
    EXPECT_EQ(0, tod.minute);
    EXPECT_EQ(0, tod.hour);
}

TEST(TimeOfDay_tests, addSecondsWrapWith1minIncrement) {
    TimeOfDay tod(23, 59, 00);
    EXPECT_EQ(1, tod.addSeconds(60)); // Now 00:00:00 (wrapped day is notified via returned value)
    EXPECT_EQ(0, tod.second);
    EXPECT_EQ(0, tod.minute);
    EXPECT_EQ(0, tod.hour);
}

TEST(TimeOfDay_tests, addSecondsWrapWithLargeIncrement) {
    TimeOfDay tod(23, 00, 00);
    EXPECT_EQ(1, tod.addSeconds(3600 + 61)); // Now 00:01:01 (wrapped increment is notified via returned value)
    EXPECT_EQ(1, tod.second);
    EXPECT_EQ(1, tod.minute);
    EXPECT_EQ(0, tod.hour);
}

TEST(TimeOfDay_tests, addSecondsWrapMultiDayIncrement) {
    TimeOfDay tod(7, 6, 5);
    EXPECT_EQ(3, tod.addSeconds(3600*24*3)); // Now 00:01:01 (wrapped 3 day-increment is notified via returned value)
    EXPECT_EQ(5, tod.second);
    EXPECT_EQ(6, tod.minute);
    EXPECT_EQ(7, tod.hour);
}

TEST(TimeOfDay_tests, toSecondsFromMidnight) {
    TimeOfDay tod(0, 0, 0);
    EXPECT_EQ(0, tod.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromMidnightWithMilliseconds) {
    TimeOfDay tod(0, 0, 0, 0);
    EXPECT_EQ(0, tod.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromTimeOfDay) {
    TimeOfDay tod(3, 36, 51);
    EXPECT_EQ(13011, tod.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromTimeOfDayIgnoresMilliseconds) {
    TimeOfDay tod(3, 36, 51, 999);
    EXPECT_EQ(13011, tod.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromInvalidTimeOfDay) {
    TimeOfDay tod;
    EXPECT_EQ(static_cast<unsigned int>(-1), tod.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsFromMaxTimeOfDay) {
    TimeOfDay tod(23, 59, 59, 999);
    EXPECT_EQ(86399, tod.toSeconds());
}

TEST(TimeOfDay_tests, toSecondsIgnoresMilliseconds) {
    TimeOfDay tod1(7, 6, 5);
    TimeOfDay tod2(7, 6, 5, 998);
    EXPECT_EQ(tod1.toSeconds(), tod2.toSeconds());
}