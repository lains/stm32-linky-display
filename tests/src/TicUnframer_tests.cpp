#include "TestHarness.h"
#include <iostream>
#include <iomanip>
#include <stdint.h>

#include "../src/TicUnframer.h"

TEST_GROUP(TicUnframer_tests) {
};

void onFrameDecode(const uint8_t* buf, size_t cnt) {
	printf("Hello on buffer size %zu\n", cnt);
	for (size_t pos = 0; pos < cnt; pos++) {
		if ((pos & 0xf) == 0) {
			printf("\n");
		}
		else {
			printf(" ");
		}
		printf("%02hhx", buf[pos]);
	}
	printf("\n");
}

TEST(TicUnframer_tests, TicUnframer_test_basic) {
	constexpr unsigned int BUFFERSIZE = 10 * 1024;
	const char *inputFilename = "./samples/continuous_linky_3P_historical_TIC_sample.bin";
	FILE* ticFile = fopen(inputFilename, "rb" );
	if (!ticFile) {
		FAILF("Error: could not open file %s", inputFilename);
	}

	uint8_t* buffer = new uint8_t[BUFFERSIZE];
	TICUnframer tu(onFrameDecode);
	int bytesRead = 0;
	do {
		int bytesRead = fread(buffer, sizeof(char), BUFFERSIZE, ticFile);
		printf("Read %d bytes from dump\n", bytesRead);
		tu.pushBytes(buffer, bytesRead);
	}
	while (bytesRead > 0 );

	delete[] buffer;

	// Done and close.
	fclose(ticFile);
}

#ifndef USE_CPPUTEST
void runTicUnframerAllUnitTests() {
	TicUnframer_test_basic();
}
#endif	// USE_CPPUTEST
