#include "TestHarness.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>

#include "../src/TicUnframer.h"

TEST_GROUP(TicUnframer_tests) {
};

class FrameDecoderStub {
public:
	FrameDecoderStub() : decodedFramesList() { }

	void onDecodeCallback(const uint8_t* buf, size_t cnt) {
		this->decodedFramesList.push_back(std::vector<uint8_t>(buf, buf+cnt));
	}

public:
	std::vector<std::vector<uint8_t> > decodedFramesList;
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

TEST(TicUnframer_tests, TicUnframer_test_one_pure_stx_etx_frame_10bytes) {
	uint8_t buffer[] = { TICUnframer::TIC_STX, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, TICUnframer::TIC_ETX };
	FrameDecoderStub stub;
	TICUnframer tu([&stub](const uint8_t* buf, size_t cnt) {
		stub.onDecodeCallback(buf, cnt);
	} );
	tu.pushBytes(buffer, sizeof(buffer));
	if (stub.decodedFramesList.size() != 1) {
		FAILF("Wrong frame count");
	}
	if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong frame decoded");
	}
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
	TicUnframer_test_one_pure_stx_etx_frame_10bytes();
	TicUnframer_test_basic();
}
#endif	// USE_CPPUTEST
