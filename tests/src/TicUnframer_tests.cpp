#include "TestHarness.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <string>
#include <iterator>

#include "Tools.h"
#include "TIC/Unframer.h"

TEST_GROUP(TicUnframer_tests) {
};

class FrameDecoderStub {
public:
	FrameDecoderStub() : decodedFramesList() { }

	void onDecodeCallback(const uint8_t* buf, size_t cnt) {
		this->decodedFramesList.push_back(std::vector<uint8_t>(buf, buf+cnt));
	}

	std::string toString() const {
		std::stringstream result;
		result << this->decodedFramesList.size() << " frame(s):\n";
		for (auto &it : this->decodedFramesList) {
			result << "[" << vectorToHexString(it) << "]\n";
		}
		return result.str();
	}

public:
	std::vector<std::vector<uint8_t> > decodedFramesList;
};

/**
 * @brief Utility function to unwrap and invoke a FrameDecoderStub instance's onDecodeCallback() from a callback call from TIC::Unframer
 * 
 * @param buf A buffer containing the full TIC frame bytes
 * @param len The number of bytes stored inside @p buf
 * @param context A context as provided by TICUnframer, used to retrieve the wrapped FrameDecoderStub instance
 */
void frameDecoderStubUnwrapInvoke(const uint8_t* buf, std::size_t cnt, void* context) {
    if (context == NULL)
        return; /* Failsafe, discard if no context */
    FrameDecoderStub* stub = static_cast<FrameDecoderStub*>(context);
    stub->onDecodeCallback(buf, cnt);
}

void onFrameDecode(const uint8_t* buf, size_t cnt) {
	printf("Received frame (%zu bytes):\n", cnt);
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
	uint8_t buffer[] = { TIC::Unframer::TIC_STX, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, TIC::Unframer::TIC_ETX };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapInvoke, &stub);
	tu.pushBytes(buffer, sizeof(buffer));
	if (stub.decodedFramesList.size() != 1) {
		FAILF("Wrong frame count: %ld", stub.decodedFramesList.size());
	}
	if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	}
}

TEST(TicUnframer_tests, TicUnframer_test_one_pure_stx_etx_frame_standalone_markers_10bytes) {
	uint8_t start_marker = TIC::Unframer::TIC_STX;
	uint8_t end_marker = TIC::Unframer::TIC_ETX;
	uint8_t buffer[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapInvoke, &stub);
	tu.pushBytes(&start_marker, 1);
	tu.pushBytes(buffer, sizeof(buffer));
	tu.pushBytes(&end_marker, 1);
	if (stub.decodedFramesList.size() != 1) {
		FAILF("Wrong frame count: %ld\nFrames received:\n%s", stub.decodedFramesList.size(), stub.toString().c_str());
	}
	if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	}
}

TEST(TicUnframer_tests, TicUnframer_test_one_pure_stx_etx_frame_standalone_bytes) {
	uint8_t start_marker = TIC::Unframer::TIC_STX;
	uint8_t end_marker = TIC::Unframer::TIC_ETX;
	uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapInvoke, &stub);
	tu.pushBytes(&start_marker, 1);
	for (unsigned int pos = 0; pos < sizeof(buffer); pos++) {
		tu.pushBytes(buffer + pos, 1);
	}
	tu.pushBytes(&end_marker, 1);
	if (stub.decodedFramesList.size() != 1) {
		FAILF("Wrong frame count: %ld\nFrames received:\n%s", stub.decodedFramesList.size(), stub.toString().c_str());
	}
	if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	}
}

TEST(TicUnframer_tests, TicUnframer_test_one_pure_stx_etx_frame_two_halves_max_buffer) {
	uint8_t buffer[514];

	buffer[0] = TIC::Unframer::TIC_STX;
	for (unsigned int pos = 1; pos < sizeof(buffer) - 1 ; pos++) {
		buffer[pos] = (uint8_t)(pos & 0xff);
		if (buffer[pos] == TIC::Unframer::TIC_ETX || buffer[pos] == TIC::Unframer::TIC_STX) {
			buffer[pos] = 0x00;	/* Remove any STX or ETX */
		}
	}
	buffer[sizeof(buffer) - 1] = TIC::Unframer::TIC_ETX;
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapInvoke, &stub);
	tu.pushBytes(buffer, sizeof(buffer) / 2);
	tu.pushBytes(buffer + sizeof(buffer) / 2, sizeof(buffer) - sizeof(buffer) / 2);
	if (stub.decodedFramesList.size() != 1) {
		FAILF("Wrong frame count: %ld\nFrames received:\n%s", stub.decodedFramesList.size(), stub.toString().c_str());
	}
	if (stub.decodedFramesList[0] != std::vector<uint8_t>(buffer+1, buffer+sizeof(buffer)-1)) {	/* Compare with buffer, but leave out first (STX) and last (ETX) bytes */
		FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	}
}

TEST(TicUnframer_tests, TicUnframer_test_one_pure_stx_etx_frame_two_halves) {
	uint8_t start_marker = TIC::Unframer::TIC_STX;
	uint8_t end_marker = TIC::Unframer::TIC_ETX;
	uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapInvoke, &stub);
	tu.pushBytes(&start_marker, 1);
	for (uint8_t pos = 0; pos < sizeof(buffer); pos++) {
		tu.pushBytes(buffer + pos, 1);
	}
	tu.pushBytes(&end_marker, 1);
	if (stub.decodedFramesList.size() != 1) {
		FAILF("Wrong frame count: %ld\nFrames received:\n%s", stub.decodedFramesList.size(), stub.toString().c_str());
	}
	if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	}
}

/**
 * @brief Send the content of a file to a ticUnframer, cutting it into chunks
 * 
 * @param ticData A buffer containing the byte sequence to inject
 * @param chunkSize The size of each chunks (except the last one, that may be smaller)
 * @param ticUnframer The TICUnframer object in which we will inject chunks
 */
void TicUnframer_test_file_sent_by_chunks(const std::vector<uint8_t>& ticData, size_t chunkSize, TIC::Unframer& ticUnframer) {

	for (size_t bytesRead = 0; bytesRead < ticData.size();) {
		size_t nbBytesToRead = ticData.size() - bytesRead;
		if (nbBytesToRead > chunkSize) {
			nbBytesToRead = chunkSize;
		}
		ticUnframer.pushBytes(&(ticData[bytesRead]), nbBytesToRead);
		bytesRead += nbBytesToRead;
	}
}

TEST(TicUnframer_tests, TicUnframer_test_sample_frames_chunked) {

	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_3P_historical_TIC_sample.bin");

	for (size_t chunkSize = 1; chunkSize <= TIC::Unframer::MAX_FRAME_SIZE; chunkSize++) {
		FrameDecoderStub stub;
		TIC::Unframer tu(frameDecoderStubUnwrapInvoke, &stub);

		TicUnframer_test_file_sent_by_chunks(ticData, TIC::Unframer::MAX_FRAME_SIZE, tu);

		if (stub.decodedFramesList.size() != 6) {
			FAILF("When using chunk size %zu: Wrong frame count: %ld\nFrames received:\n%s", chunkSize, stub.decodedFramesList.size(), stub.toString().c_str());
		}
		for (auto it : stub.decodedFramesList) {
			if (it.size() != 233)
				FAILF("When using chunk size %zu: Wrong frame decoded: %s", chunkSize, vectorToHexString(it).c_str());
		}
	}
}

#ifndef USE_CPPUTEST
void runTicUnframerAllUnitTests() {
	TicUnframer_test_one_pure_stx_etx_frame_10bytes();
	TicUnframer_test_one_pure_stx_etx_frame_standalone_markers_10bytes();
	TicUnframer_test_one_pure_stx_etx_frame_standalone_bytes();
	TicUnframer_test_one_pure_stx_etx_frame_two_halves_max_buffer();
	TicUnframer_test_sample_frames_chunked();
}
#endif	// USE_CPPUTEST
