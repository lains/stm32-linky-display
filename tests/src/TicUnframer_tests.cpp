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
	FrameDecoderStub() :
		currentFrame(),
		decodedFramesList() { }

	void onNewBytesCallback(const uint8_t* buf, size_t cnt) {
		std::vector<uint8_t> newBytes(buf, buf+cnt);
		/* Append the new bytes at the end of the previously existing ones */
		this->currentFrame.insert(this->currentFrame.end(), newBytes.begin(), newBytes.end());
	}

	void onFrameCompleteCallback() {
		this->decodedFramesList.push_back(this->currentFrame);
		this->currentFrame.clear();
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
	std::vector<uint8_t> currentFrame;
	std::vector<std::vector<uint8_t> > decodedFramesList;
};

/**
 * @brief Utility function to unwrap and invoke a FrameDecoderStub instance's onNewBytesCallback() from a callback call from TIC::Unframer when new bytes are parsed
 * 
 * @param buf A buffer containing the full TIC frame bytes
 * @param len The number of bytes stored inside @p buf
 * @param context A context as provided by TICUnframer, used to retrieve the wrapped FrameDecoderStub instance
 */
static void frameDecoderStubUnwrapForwardFrameBytes(const uint8_t* buf, std::size_t cnt, void* context) {
    if (context == NULL)
        return; /* Failsafe, discard if no context */
    FrameDecoderStub* stub = static_cast<FrameDecoderStub*>(context);
    stub->onNewBytesCallback(buf, cnt);
}

/**
 * @brief Utility function to unwrap and invoke a FrameDecoderStub instance's onFrameCompleteCallback() from a callback call from TIC::Unframer when the current frame is finished
 * 
 * @param context A context as provided by TICUnframer, used to retrieve the wrapped FrameDecoderStub instance
 */
static void frameDecoderStubUnwrapFrameFinished(void* context) {
    if (context == NULL)
        return; /* Failsafe, discard if no context */
    FrameDecoderStub* stub = static_cast<FrameDecoderStub*>(context);
    stub->onFrameCompleteCallback();
}

TEST(TicUnframer_tests, TicUnframer_test_one_pure_stx_etx_frame_10bytes) {
	uint8_t buffer[] = { TIC::Unframer::START_MARKER, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, TIC::Unframer::END_MARKER };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapForwardFrameBytes, frameDecoderStubUnwrapFrameFinished, &stub);
	tu.pushBytes(buffer, sizeof(buffer));
	if (stub.decodedFramesList.size() != 1) {
		FAILF("Wrong frame count: %ld", stub.decodedFramesList.size());
	}
	if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	}
}

TEST(TicUnframer_tests, TicUnframer_test_one_pure_stx_etx_frame_standalone_markers_10bytes) {
	uint8_t start_marker = TIC::Unframer::START_MARKER;
	uint8_t end_marker = TIC::Unframer::END_MARKER;
	uint8_t buffer[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapForwardFrameBytes, frameDecoderStubUnwrapFrameFinished, &stub);
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
	uint8_t start_marker = TIC::Unframer::START_MARKER;
	uint8_t end_marker = TIC::Unframer::END_MARKER;
	uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapForwardFrameBytes, frameDecoderStubUnwrapFrameFinished, &stub);
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

	buffer[0] = TIC::Unframer::START_MARKER;
	for (unsigned int pos = 1; pos < sizeof(buffer) - 1 ; pos++) {
		buffer[pos] = (uint8_t)(pos & 0xff);
		if (buffer[pos] == TIC::Unframer::END_MARKER || buffer[pos] == TIC::Unframer::START_MARKER) {
			buffer[pos] = 0x00;	/* Remove any STX or ETX */
		}
	}
	buffer[sizeof(buffer) - 1] = TIC::Unframer::END_MARKER;
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapForwardFrameBytes, frameDecoderStubUnwrapFrameFinished, &stub);
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
	uint8_t start_marker = TIC::Unframer::START_MARKER;
	uint8_t end_marker = TIC::Unframer::END_MARKER;
	uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	FrameDecoderStub stub;
	TIC::Unframer tu(frameDecoderStubUnwrapForwardFrameBytes, frameDecoderStubUnwrapFrameFinished, &stub);
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
 * @brief Send the content of a file to a TIC::Unframer, cutting it into chunks
 * 
 * @param ticData A buffer containing the byte sequence to inject
 * @param maxChunkSize The size of each chunks (except the last one, that may be smaller)
 * @param ticUnframer The TIC::Unframer object in which we will inject chunks
 */
static void TicUnframer_test_file_sent_by_chunks(const std::vector<uint8_t>& ticData, size_t maxChunkSize, TIC::Unframer& ticUnframer) {

	for (size_t bytesRead = 0; bytesRead < ticData.size();) {
		size_t nbBytesToRead = ticData.size() - bytesRead;
		if (nbBytesToRead > maxChunkSize) {
			nbBytesToRead = maxChunkSize; // Limit the number of bytes pushed to the provided max chunkSize
		}
		ticUnframer.pushBytes(&(ticData[bytesRead]), nbBytesToRead);
		bytesRead += nbBytesToRead;
	}
}

TEST(TicUnframer_tests, TicUnframer_chunked_sample_unframe_historical_TIC) {

	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_3P_historical_TIC_sample.bin");

	for (size_t chunkSize = 1; chunkSize <= TIC::Unframer::MAX_FRAME_SIZE; chunkSize++) {
		FrameDecoderStub stub;
		TIC::Unframer tu(frameDecoderStubUnwrapForwardFrameBytes, frameDecoderStubUnwrapFrameFinished, &stub);

		TicUnframer_test_file_sent_by_chunks(ticData, chunkSize, tu);

		std::size_t expectedTotalFramesCount = 6;
		if (stub.decodedFramesList.size() != expectedTotalFramesCount) {
			FAILF("When using chunk size %zu: Wrong frame count: %zu, exepcted %zu\nFrames received:\n%s", chunkSize, stub.decodedFramesList.size(), expectedTotalFramesCount, stub.toString().c_str());
		}
		for (auto it : stub.decodedFramesList) {
			if (it.size() != 233)
				FAILF("When using chunk size %zu: Wrong frame decoded: %s", chunkSize, vectorToHexString(it).c_str());
		}
	}
}

TEST(TicUnframer_tests, TicUnframer_chunked_sample_unframe_standard_TIC) {

	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_1P_standard_TIC_sample.bin");

	for (size_t chunkSize = 1; chunkSize <= TIC::Unframer::MAX_FRAME_SIZE; chunkSize++) {
		FrameDecoderStub stub;
		TIC::Unframer tu(frameDecoderStubUnwrapForwardFrameBytes, frameDecoderStubUnwrapFrameFinished, &stub);

		TicUnframer_test_file_sent_by_chunks(ticData, chunkSize, tu);

		std::size_t expectedTotalFramesCount = 12;
		if (stub.decodedFramesList.size() != expectedTotalFramesCount) {
			FAILF("When using chunk size %zu: Wrong frame count: %zu, exepcted %zu\nFrames received:\n%s", chunkSize, stub.decodedFramesList.size(), expectedTotalFramesCount, stub.toString().c_str());
		}
		for (auto it : stub.decodedFramesList) {
			if (it.size() != 863)
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
	TicUnframer_test_one_pure_stx_etx_frame_two_halves();
	TicUnframer_chunked_sample_unframe_historical_TIC();
	TicUnframer_chunked_sample_unframe_standard_TIC();
}
#endif	// USE_CPPUTEST
