#include "TestHarness.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <string>
#include <iterator>

#include "Tools.h"
#include "TIC/DatasetView.h"
#include "TIC/DatasetExtractor.h"
#include "TIC/Unframer.h"

TEST_GROUP(TicDatasetView_tests) {
};

class DatasetDecoderStub {
public:
	DatasetDecoderStub() : decodedDatasetList() { }

	void onDatasetExtractedCallback(const uint8_t* buf, size_t cnt) {
		this->decodedDatasetList.push_back(std::vector<uint8_t>(buf, buf+cnt));
	}

	std::string toString() const {
		std::stringstream result;
		result << this->decodedDatasetList.size() << " dataset(s):\n";
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
static void datasetDecoderStubUnwrapInvoke(const uint8_t* buf, std::size_t cnt, void* context) {
    if (context == NULL)
        return; /* Failsafe, discard if no context */
    DatasetDecoderStub* stub = static_cast<DatasetDecoderStub*>(context);
    stub->onDatasetExtractedCallback(buf, cnt);
}

/**
 * @brief Utility function to unwrap and invoke a TIC::DatasetExtractor instance's pushBytes() from a callback call from TIC::Unframer
 * 
 * @param buf A buffer containing the full TIC frame bytes
 * @param len The number of bytes stored inside @p buf
 * @param context A context as provided by TIC::Unframer, used to retrieve the wrapped TIC::DatasetExtractor instance
 */
static void datasetExtractorUnwrapForwardFullFrameBytes(const uint8_t* buf, std::size_t cnt, void* context) {
	if (context == NULL)
		return; /* Failsafe, discard if no context */
	TIC::DatasetExtractor* de = static_cast<TIC::DatasetExtractor*>(context);
	de->pushBytes(buf, cnt);
	/* We have a full frame byte in our buf, so once bytes have been pushed, if there is an open dataset, we should discard it and start over at the following frame */
	de->reset();
}

static void onDatasetExtracted(const uint8_t* buf, size_t cnt) {
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

TEST(TicDatasetView_tests, TicDatasetView_correct_sample_typical_historical_dataset) {
	const char dataset[] = "ADCO 012345678901 E";

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Incorrect parsing of dataset");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'A', 'D', 'C', 'O'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataBuffer == nullptr) {
		FAILF("NULL dataBuffer");
	}
	std::vector<uint8_t> dataBufferVec(dv.dataBuffer, dv.dataBuffer + dv.dataSz);
	if (dataBufferVec != std::vector<uint8_t>({'0','1','2','3','4','5','6','7','8','9','0','1'})) {
		FAILF("Wrong dataset data: %s", vectorToHexString(dataBufferVec).c_str());
	}
	if (dv.decodedType != TIC::DatasetView::DatasetType::ValidHistorical) {
		FAILF("Expected valid historical type detection");
	}
}

TEST(TicDatasetView_tests, TicDatasetView_correct_sample_typical_standard_dataset) {
	const char dataset[] = "ADSC\t012345678901\t2";

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Incorrect parsing of dataset");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'A', 'D', 'S', 'C'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataBuffer == nullptr) {
		FAILF("NULL dataBuffer");
	}
	std::vector<uint8_t> dataBufferVec(dv.dataBuffer, dv.dataBuffer + dv.dataSz);
	if (dataBufferVec != std::vector<uint8_t>({'0','1','2','3','4','5','6','7','8','9','0','1'})) {
		FAILF("Wrong dataset data: %s", vectorToHexString(dataBufferVec).c_str());
	}

	if (dv.decodedType != TIC::DatasetView::DatasetType::ValidStandard) {
		FAILF("Expected valid standard type detection");
	}
}

TEST(TicDatasetView_tests, TicDatasetView_extra_leading_start_marker) {
	char dataset[] = 	{ "*ADCO 012345678901 E"};
	dataset[0] = TIC::DatasetExtractor::START_MARKER; /* Replace the * with our start marker */

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Incorrect parsing of dataset");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'A', 'D', 'C', 'O'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataBuffer == nullptr) {
		FAILF("NULL dataBuffer");
	}
	std::vector<uint8_t> dataBufferVec(dv.dataBuffer, dv.dataBuffer + dv.dataSz);
	if (dataBufferVec != std::vector<uint8_t>({'0','1','2','3','4','5','6','7','8','9','0','1'})) {
		FAILF("Wrong dataset data: %s", vectorToHexString(dataBufferVec).c_str());
	}
}

TEST(TicDatasetView_tests, TicDatasetView_extra_trailing_end_marker) {
	char dataset[] = 	{ "ADCO 012345678901 E*"};
	dataset[sizeof(dataset)-1-1] = TIC::DatasetExtractor::END_MARKER; /* Replace the * with our end marker (-1 to get inside the buffer, -1 again to move before the terminating '\0') */

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Incorrect parsing of dataset");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'A', 'D', 'C', 'O'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataBuffer == nullptr) {
		FAILF("NULL dataBuffer");
	}
	std::vector<uint8_t> dataBufferVec(dv.dataBuffer, dv.dataBuffer + dv.dataSz);
	if (dataBufferVec != std::vector<uint8_t>({'0','1','2','3','4','5','6','7','8','9','0','1'})) {
		FAILF("Wrong dataset data: %s", vectorToHexString(dataBufferVec).c_str());
	}
}

TEST(TicDatasetView_tests, TicDatasetView_wrong_crc) {
	const char dataset[] = "ADSC\t012345678901\tJ";
	DatasetDecoderStub stub;

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
	if (dv.isValid()) {
		FAILF("Expecting invalid dataset");
	}

	if (dv.decodedType != TIC::DatasetView::DatasetType::WrongCRC) {
		FAILF("Expecting incorrect CRC");
	}
}

TEST(TicDatasetView_tests, TicDatasetView_very_long) {
	const char dataset[] = "PJOURF+1\t00008001 NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE\t9";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Incorrect parsing of dataset");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'P', 'J', 'O', 'U', 'R', 'F', '+', '1'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataBuffer == nullptr) {
		FAILF("NULL dataBuffer");
	}
	if (dv.dataBuffer != &(datasetBuf[8])) {
		FAILF("Wrong dataset data, expected it to point to adress dataset+8, instead, it points to dataset+%zu", dv.dataBuffer - datasetBuf);
	}
	if (dv.dataSz != 98) {
		FAILF("Wrong dataset data size: %zu, expected it contain our long value (98 chars)", dv.dataSz);
	}
}

TEST(TicDatasetView_tests, TicDatasetView_too_short) {
	const char dataset[] = "L V 9";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	for (std::size_t datasetTestLength = 0; datasetTestLength <= (std::size_t)(strlen(dataset)); datasetTestLength++) {
		TIC::DatasetView dv((const uint8_t*)datasetBuf, datasetTestLength);
		
		if (dv.isValid() && dv.decodedType != TIC::DatasetView::DatasetType::WrongCRC) {
			FAILF("Expecting invalid dataset (but not on CRC errors, rather because it's too short to be parsed)");
		}
	}
}

TEST(TicDatasetView_tests, TicDatasetView_test_one_pure_stx_etx_frame_standalone_bytes) {
	uint8_t start_marker = TIC::DatasetExtractor::START_MARKER;
	uint8_t end_marker = TIC::DatasetExtractor::END_MARKER;
	uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	DatasetDecoderStub stub;
	TIC::DatasetExtractor de(datasetDecoderStubUnwrapInvoke, &stub);
	de.pushBytes(&start_marker, 1);
	for (unsigned int pos = 0; pos < sizeof(buffer); pos++) {
		de.pushBytes(buffer + pos, 1);
	}
	de.pushBytes(&end_marker, 1);
	if (stub.decodedDatasetList.size() != 1) {
		FAILF("Wrong dataset count: %ld\nDatasets received:\n%s", stub.decodedDatasetList.size(), stub.toString().c_str());
	}
	if (stub.decodedDatasetList[0] != std::vector<uint8_t>({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong dataset decoded: %s", vectorToHexString(stub.decodedDatasetList[0]).c_str());
	}
}

TEST(TicDatasetView_tests, TicDatasetView_test_one_pure_stx_etx_frame_two_halves_max_buffer) {
	uint8_t buffer[128];

	buffer[0] = TIC::DatasetExtractor::START_MARKER;
	for (unsigned int pos = 1; pos < sizeof(buffer) - 1 ; pos++) {
		buffer[pos] = (uint8_t)(pos & 0xff);
		if (buffer[pos] == TIC::DatasetExtractor::START_MARKER || buffer[pos] == TIC::DatasetExtractor::END_MARKER || buffer[pos] == TIC::Unframer::START_MARKER || buffer[pos] == TIC::Unframer::END_MARKER) {
			buffer[pos] = 0x00;	/* Remove any frame of dataset delimiters */
		}
	}
	buffer[sizeof(buffer) - 1] = TIC::DatasetExtractor::END_MARKER;
	DatasetDecoderStub stub;
	TIC::DatasetExtractor de(datasetDecoderStubUnwrapInvoke, &stub);
	de.pushBytes(buffer, sizeof(buffer) / 2);
	de.pushBytes(buffer + sizeof(buffer) / 2, sizeof(buffer) - sizeof(buffer) / 2);
	if (stub.decodedDatasetList.size() != 1) {
		FAILF("Wrong dataset count: %ld\nDatasets received:\n%s", stub.decodedDatasetList.size(), stub.toString().c_str());
	}
	if (stub.decodedDatasetList[0] != std::vector<uint8_t>(buffer+1, buffer+sizeof(buffer)-1)) {	/* Compare with buffer, but leave out first and last bytes (markers) */
		FAILF("Wrong dataset decoded: %s", vectorToHexString(stub.decodedDatasetList[0]).c_str());
	}
}

TEST(TicDatasetView_tests, TicDatasetView_test_one_pure_stx_etx_frame_two_halves) {
	uint8_t start_marker = TIC::DatasetExtractor::START_MARKER;
	uint8_t end_marker = TIC::DatasetExtractor::END_MARKER;
	uint8_t buffer[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
	DatasetDecoderStub stub;
	TIC::DatasetExtractor de(datasetDecoderStubUnwrapInvoke, &stub);
	de.pushBytes(&start_marker, 1);
	for (uint8_t pos = 0; pos < sizeof(buffer); pos++) {
		de.pushBytes(buffer + pos, 1);
	}
	de.pushBytes(&end_marker, 1);
	if (stub.decodedDatasetList.size() != 1) {
		FAILF("Wrong dataset count: %ld\nDatasets received:\n%s", stub.decodedDatasetList.size(), stub.toString().c_str());
	}
	if (stub.decodedDatasetList[0] != std::vector<uint8_t>({0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39})) {
		FAILF("Wrong dataset decoded: %s", vectorToHexString(stub.decodedDatasetList[0]).c_str());
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

TEST(TicDatasetView_tests, Chunked_sample_unframe_dsextract_decode_historical_TIC) {
	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_3P_historical_TIC_sample.bin");

	for (size_t chunkSize = 1; chunkSize <= TIC::DatasetExtractor::MAX_DATASET_SIZE; chunkSize++) {
		DatasetDecoderStub stub;
		TIC::DatasetExtractor de(datasetDecoderStubUnwrapInvoke, &stub);
		TIC::Unframer tu(datasetExtractorUnwrapForwardFullFrameBytes, &de);

		TicUnframer_test_file_sent_by_chunks(ticData, chunkSize, tu);

		/**
		 * @brief Sizes (in bytes) of the successive dataset in each repeated TIC frame
		 */
		std::size_t datasetExpectedSizes[] = { 19, /* ADCO label */
		                                       14, 11, 16, 11,
											   12, 12, 12, /* Three times IINST? labels (on each phase) */
											   11, 11, 11, /* Three times IMAX? labels (on each phase) */
											   12, 12, 9, 17, 9 };
		unsigned int nbExpectedDatasetPerFrame = sizeof(datasetExpectedSizes)/sizeof(datasetExpectedSizes[0]);

		std::size_t expectedTotalDatasetCount = 6 * nbExpectedDatasetPerFrame; /* 6 frames, each containing the above datasets */
		if (stub.decodedDatasetList.size() != expectedTotalDatasetCount) { 
			FAILF("When using chunk size %zu: Wrong dataset count: %zu, expected %zu\nDatasets received:\n%s", chunkSize, stub.decodedDatasetList.size(), expectedTotalDatasetCount, stub.toString().c_str());
		}
		char firstDatasetAsCString[] = "ADCO 056234673197 O";
		std::vector<uint8_t> expectedFirstDatasetInFrame(firstDatasetAsCString, firstDatasetAsCString+strlen(firstDatasetAsCString));
		if (stub.decodedDatasetList[0] != expectedFirstDatasetInFrame) {
			FAILF("Unexpected first dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[0]).c_str(), vectorToHexString(expectedFirstDatasetInFrame).c_str());
		}
		char lastDatasetAsCString[] = "PPOT 00 #";
		std::vector<uint8_t> expectedLastDatasetInFrame(lastDatasetAsCString, lastDatasetAsCString+strlen(lastDatasetAsCString));
		if (stub.decodedDatasetList[nbExpectedDatasetPerFrame-1] != expectedLastDatasetInFrame) {
			FAILF("Unexpected last dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[nbExpectedDatasetPerFrame-1]).c_str(), vectorToHexString(expectedLastDatasetInFrame).c_str());
		}
		for (std::size_t datasetIndex = 0; datasetIndex < stub.decodedDatasetList.size(); datasetIndex++) {
			std::size_t receivedDatasetSize = stub.decodedDatasetList[datasetIndex].size();
			std::size_t expectedDatasetSize = datasetExpectedSizes[datasetIndex % nbExpectedDatasetPerFrame];
			if (receivedDatasetSize != expectedDatasetSize) {
				FAILF("When using chunk size %zu: Wrong dataset decoded at index %zu in frame. Expected %zu bytes, got %zu bytes. Dataset content: %s", chunkSize, datasetIndex, expectedDatasetSize, receivedDatasetSize, vectorToHexString(stub.decodedDatasetList[datasetIndex]).c_str());
			}
		}
	}
}

TEST(TicDatasetView_tests, Chunked_sample_unframe_dsextract_decode_standard_TIC) {

	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_1P_standard_TIC_sample.bin");

	for (size_t chunkSize = 1; chunkSize <= TIC::DatasetExtractor::MAX_DATASET_SIZE; chunkSize++) {
		DatasetDecoderStub stub;
		TIC::DatasetExtractor de(datasetDecoderStubUnwrapInvoke, &stub);
		TIC::Unframer tu(datasetExtractorUnwrapForwardFullFrameBytes, &de);

		TicUnframer_test_file_sent_by_chunks(ticData, chunkSize, tu);

		/**
		 * @brief Sizes (in bytes) of the successive dataset in each repeated TIC frame
		 */
		std::size_t datasetExpectedSizes[] = { 19, /* ADSC label */
		                                       9, 21, 23, 24, 16,
		                                       18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, /* 14 labels with EASF+EASD data */
											   11, 11, 9, 10, 14, 28, 30, 27, 29, 25, 15, 39,
											   20, /* PRM label */
											   12, 10,
											   11, /* NJOURF label */
											   13, /* NJOURF+1 label */
											   109 /* Long PJOURF+1 label */ };
		unsigned int nbExpectedDatasetPerFrame = sizeof(datasetExpectedSizes)/sizeof(datasetExpectedSizes[0]);

		std::size_t expectedTotalDatasetCount = 12 * nbExpectedDatasetPerFrame; /* 12 frames, each containing the above datasets */
		if (stub.decodedDatasetList.size() != expectedTotalDatasetCount) { 
			FAILF("When using chunk size %zu: Wrong dataset count: %zu, expected %zu\nDatasets received:\n%s", chunkSize, stub.decodedDatasetList.size(), expectedTotalDatasetCount, stub.toString().c_str());
		}
		char firstDatasetAsCString[] = "ADSC\t064468368739\tJ";
		std::vector<uint8_t> expectedFirstDatasetInFrame(firstDatasetAsCString, firstDatasetAsCString+strlen(firstDatasetAsCString));
		if (stub.decodedDatasetList[0] != expectedFirstDatasetInFrame) {
			FAILF("Unexpected first dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[0]).c_str(), vectorToHexString(expectedFirstDatasetInFrame).c_str());
		}
		char lastDatasetAsCString[] = "PJOURF+1\t00008001 NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE\t9";
		std::vector<uint8_t> expectedLastDatasetInFrame(lastDatasetAsCString, lastDatasetAsCString+strlen(lastDatasetAsCString));
		if (stub.decodedDatasetList[nbExpectedDatasetPerFrame-1] != expectedLastDatasetInFrame) {
			FAILF("Unexpected last dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[nbExpectedDatasetPerFrame-1]).c_str(), vectorToHexString(expectedLastDatasetInFrame).c_str());
		}
		for (std::size_t datasetIndex = 0; datasetIndex < stub.decodedDatasetList.size(); datasetIndex++) {
			std::size_t receivedDatasetSize = stub.decodedDatasetList[datasetIndex].size();
			std::size_t expectedDatasetSize = datasetExpectedSizes[datasetIndex % nbExpectedDatasetPerFrame];
			if (receivedDatasetSize != expectedDatasetSize) {
				FAILF("When using chunk size %zu: Wrong dataset decoded at index %zu in frame. Expected %zu bytes, got %zu bytes. Dataset content: %s", chunkSize, datasetIndex, expectedDatasetSize, receivedDatasetSize, vectorToHexString(stub.decodedDatasetList[datasetIndex]).c_str());
			}
		}
	}
}

#ifndef USE_CPPUTEST
void runTicDatasetViewAllUnitTests() {
	TicDatasetView_correct_sample_typical_historical_dataset();
	TicDatasetView_correct_sample_typical_standard_dataset();
	TicDatasetView_extra_leading_start_marker();
	TicDatasetView_extra_trailing_end_marker();
	TicDatasetView_wrong_crc();
	TicDatasetView_very_long();
	TicDatasetView_too_short();
	// TicDatasetView_test_one_pure_stx_etx_frame_standalone_markers_10bytes();
	// TicDatasetView_test_one_pure_stx_etx_frame_standalone_bytes();
	// TicDatasetView_test_one_pure_stx_etx_frame_two_halves_max_buffer();
	// TicDatasetView_test_one_pure_stx_etx_frame_two_halves();
	// Chunked_sample_unframe_dsextract_historical_TIC();
	// Chunked_sample_unframe_dsextract_standard_TIC();
}
#endif	// USE_CPPUTEST
