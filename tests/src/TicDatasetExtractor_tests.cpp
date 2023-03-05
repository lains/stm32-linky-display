#include "TestHarness.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <string>
#include <iterator>

#include "Tools.h"
#include "TIC/DatasetExtractor.h"

TEST_GROUP(TicDatasetExtractor_tests) {
};

class DatasetDecoderStub {
public:
	DatasetDecoderStub() : decodedDatasetList() { }

	void onDatasetExtractedCallback(const uint8_t* buf, size_t cnt) {
		this->decodedDatasetList.push_back(std::vector<uint8_t>(buf, buf+cnt));
	}

	std::string toString() const {
		std::stringstream result;
		result << this->decodedDatasetList.size() << " frame(s):\n";
		for (auto &it : this->decodedDatasetList) {
			result << "[" << vectorToHexString(it) << "]\n";
		}
		return result.str();
	}

public:
	std::vector<std::vector<uint8_t> > decodedDatasetList;
};

/**
 * @brief Utility function to unwrap and invoke a FrameDecoderStub instance's onDecodeCallback() from a callback call from TIC::DatasetExtractor
 * 
 * @param buf A buffer containing the full TIC dataset bytes
 * @param len The number of bytes stored inside @p buf
 * @param context A context as provided by TIC::DatasetExtractor, used to retrieve the wrapped DatasetDecoderStub instance
 */
void datasetDecoderStubUnwrapInvoke(const uint8_t* buf, std::size_t cnt, void* context) {
    if (context == NULL)
        return; /* Failsafe, discard if no context */
    DatasetDecoderStub* stub = static_cast<DatasetDecoderStub*>(context);
    stub->onDatasetExtractedCallback(buf, cnt);
}

void onDatasetExtracted(const uint8_t* buf, size_t cnt) {
	printf("Received dataset (%zu bytes):\n", cnt);
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

TEST(TicDatasetExtractor_tests, TicDatasetExtractor_test_one_pure_dataset_10bytes) {
	uint8_t buffer[] = { 0x0d, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x0a };
	DatasetDecoderStub stub;
	TIC::DatasetExtractor de(datasetDecoderStubUnwrapInvoke, &stub);
	de.pushBytes(buffer, sizeof(buffer));
	if (stub.decodedDatasetList.size() != 1) {
		FAILF("Wrong dataset count: %ld", stub.decodedDatasetList.size());
	}
	if (stub.decodedDatasetList[0] != std::vector<uint8_t>({0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong dataset decoded: %s", vectorToHexString(stub.decodedDatasetList[0]).c_str());
	}
}

TEST(TicDatasetExtractor_tests, TicDatasetExtractor_test_one_pure_stx_etx_frame_standalone_markers_10bytes) {
	uint8_t start_marker = TIC::DatasetExtractor::LF;
	uint8_t end_marker = TIC::DatasetExtractor::CR;
	uint8_t buffer[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	DatasetDecoderStub stub;
	TIC::DatasetExtractor de(datasetDecoderStubUnwrapInvoke, &stub);
	de.pushBytes(&start_marker, 1);
	de.pushBytes(buffer, sizeof(buffer));
	de.pushBytes(&end_marker, 1);
	if (stub.decodedDatasetList.size() != 1) {
		FAILF("Wrong dataset count: %ld\nDatasets received:\n%s", stub.decodedDatasetList.size(), stub.toString().c_str());
	}
	if (stub.decodedDatasetList[0] != std::vector<uint8_t>({0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong dataset decoded: %s", vectorToHexString(stub.decodedDatasetList[0]).c_str());
	}
}

TEST(TicDatasetExtractor_tests, TicDatasetExtractor_test_one_pure_stx_etx_frame_standalone_bytes) {
	// uint8_t start_marker = TICUnframer::TIC_STX;
	// uint8_t end_marker = TICUnframer::TIC_ETX;
	// uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	// FrameDecoderStub stub;
	// TICUnframer tu(frameDecoderStubUnwrapInvoke, &stub);
	// tu.pushBytes(&start_marker, 1);
	// for (unsigned int pos = 0; pos < sizeof(buffer); pos++) {
	// 	tu.pushBytes(buffer + pos, 1);
	// }
	// tu.pushBytes(&end_marker, 1);
	// if (stub.decodedFramesList.size() != 1) {
	// 	FAILF("Wrong frame count: %ld\nFrames received:\n%s", stub.decodedFramesList.size(), stub.toString().c_str());
	// }
	// if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
	// 	FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	// }
}

TEST(TicDatasetExtractor_tests, TicDatasetExtractor_test_one_pure_stx_etx_frame_two_256bytes_halves) {
	// uint8_t buffer[514];

	// buffer[0] = TICUnframer::TIC_STX;
	// for (unsigned int pos = 1; pos < sizeof(buffer) - 1 ; pos++) {
	// 	buffer[pos] = (uint8_t)(pos & 0xff);
	// 	if (buffer[pos] == TICUnframer::TIC_ETX || buffer[pos] == TICUnframer::TIC_STX) {
	// 		buffer[pos] = 0x00;	/* Remove any STX or ETX */
	// 	}
	// }
	// buffer[sizeof(buffer) - 1] = TICUnframer::TIC_ETX;
	// FrameDecoderStub stub;
	// TICUnframer tu(frameDecoderStubUnwrapInvoke, &stub);
	// tu.pushBytes(buffer, sizeof(buffer) / 2);
	// tu.pushBytes(buffer + sizeof(buffer) / 2, sizeof(buffer) - sizeof(buffer) / 2);
	// if (stub.decodedFramesList.size() != 1) {
	// 	FAILF("Wrong frame count: %ld\nFrames received:\n%s", stub.decodedFramesList.size(), stub.toString().c_str());
	// }
	// if (stub.decodedFramesList[0] != std::vector<uint8_t>(buffer+1, buffer+sizeof(buffer)-1)) {	/* Compare with buffer, but leave out first (STX) and last (ETX) bytes */
	// 	FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	// }
}

TEST(TicDatasetExtractor_tests, TicDatasetExtractor_test_one_pure_stx_etx_frame_two_halves) {
	// uint8_t start_marker = TICUnframer::TIC_STX;
	// uint8_t end_marker = TICUnframer::TIC_ETX;
	// uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	// FrameDecoderStub stub;
	// TICUnframer tu(frameDecoderStubUnwrapInvoke, &stub);
	// tu.pushBytes(&start_marker, 1);
	// for (uint8_t pos = 0; pos < sizeof(buffer); pos++) {
	// 	tu.pushBytes(buffer + pos, 1);
	// }
	// tu.pushBytes(&end_marker, 1);
	// if (stub.decodedFramesList.size() != 1) {
	// 	FAILF("Wrong frame count: %ld\nFrames received:\n%s", stub.decodedFramesList.size(), stub.toString().c_str());
	// }
	// if (stub.decodedFramesList[0] != std::vector<uint8_t>({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
	// 	FAILF("Wrong frame decoded: %s", vectorToHexString(stub.decodedFramesList[0]).c_str());
	// }
}

/**
 * @brief Send the content of a file to a ticUnframer, cutting it into chunks
 * 
 * @param ticData A buffer containing the byte sequence to inject
 * @param chunkSize The size of each chunks (except the last one, that may be smaller)
 * @param ticUnframer The TICUnframer object in which we will inject chunks
 */
// void TicDatasetExtractor_test_file_sent_by_chunks(const std::vector<uint8_t>& ticData, size_t chunkSize, TICUnframer& ticUnframer) {

// 	for (size_t bytesRead = 0; bytesRead < ticData.size();) {
// 		size_t nbBytesToRead = ticData.size() - bytesRead;
// 		if (nbBytesToRead > chunkSize) {
// 			nbBytesToRead = chunkSize;
// 		}
// 		ticUnframer.pushBytes(&(ticData[bytesRead]), nbBytesToRead);
// 		bytesRead += nbBytesToRead;
// 	}
// }

TEST(TicDatasetExtractor_tests, TicDatasetExtractor_test_sample_frames_chunked) {

	// std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_3P_historical_TIC_sample.bin");

	// for (size_t chunkSize = 1; chunkSize <= TICUnframer::MAX_FRAME_SIZE; chunkSize++) {
	// 	FrameDecoderStub stub;
	// 	TICUnframer tu(frameDecoderStubUnwrapInvoke, &stub);

	// 	TicDatasetExtractor_test_file_sent_by_chunks(ticData, TICUnframer::MAX_FRAME_SIZE, tu);

	// 	if (stub.decodedFramesList.size() != 6) {
	// 		FAILF("When using chunk size %zu: Wrong frame count: %ld\nFrames received:\n%s", chunkSize, stub.decodedFramesList.size(), stub.toString().c_str());
	// 	}
	// 	for (auto it : stub.decodedFramesList) {
	// 		if (it.size() != 233)
	// 			FAILF("When using chunk size %zu: Wrong frame decoded: %s", chunkSize, vectorToHexString(it).c_str());
	// 	}
	// }
}

#ifndef USE_CPPUTEST
void runTicDatasetExtractorAllUnitTests() {
	TicDatasetExtractor_test_one_pure_dataset_10bytes();
	TicDatasetExtractor_test_one_pure_stx_etx_frame_standalone_markers_10bytes();
}
#endif	// USE_CPPUTEST
