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

class DatasetContent {
public :
	DatasetContent() : label(), data(), horodate(), decodeResult(TIC::DatasetView::DatasetType::Malformed) {}
	DatasetContent(const std::string& label, const std::string& data, const TIC::Horodate& horodate, const TIC::DatasetView::DatasetType& decodeResult) :
		label(label),
		data(data),
		horodate(horodate),
		decodeResult(decodeResult) {}

	std::string toString() const {
		std::stringstream result;
		result << this->decodedResultToString() << " dataset with label=\"" << this->label << "\", data=\"" << this->data << "\"";
		if (this->horodate.isValid) {
			result << ", horodate=" << this->horodate.toString();
		}
		return result.str();
	}

	std::string decodedResultToString() const {
		switch (this->decodeResult) {
			case TIC::DatasetView::DatasetType::Malformed: return "malformed";
			case TIC::DatasetView::DatasetType::ValidHistorical: return "historical";
			case TIC::DatasetView::DatasetType::ValidStandard: return "standard";
			case TIC::DatasetView::DatasetType::WrongCRC: return "bad crc";
			default:
				return "unknown";
		}
	}

	std::string label;
	std::string data;
	TIC::Horodate horodate;
	TIC::DatasetView::DatasetType decodeResult;
};

class DatasetViewStub {
public:
	DatasetViewStub() : datasetContentList() { }

	void onDatasetExtractedCallback(const uint8_t* buf, unsigned int cnt) {
		/* Once dataset has been extracted, run a dataset view on it */
		TIC::DatasetView dv(buf, cnt);
		/* Now, store the data content of dataset view rather than just keeping the view on a dataset buffer, because that buffer may be destroyed once extracted */
		std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
		std::string label(labelBufferVec.begin(), labelBufferVec.end());
		std::vector<uint8_t> dataBufferVec(dv.dataBuffer, dv.dataBuffer + dv.dataSz);
		std::string data(dataBufferVec.begin(), dataBufferVec.end());
		TIC::Horodate horodate = dv.horodate;

		this->datasetContentList.push_back(DatasetContent(label, data, horodate, dv.decodedType));
	}

	std::string toString() const {
		std::stringstream result;
		result << this->datasetContentList.size() << " decoded dataset(s):\n";
		for (auto &it : this->datasetContentList) {
			result << "[" << it.toString() << "]\n";
		}
		return result.str();
	}

public:
	std::vector<DatasetContent> datasetContentList;
};

/**
 * @brief Utility function to unwrap and invoke a !!!!!!!!!!!!!!!!!!!!!!!!! Stub instance's onDecodeCallback() from a callback call from TIC::DatasetExtractor
 * 
 * @param buf A buffer containing the full TIC dataset bytes
 * @param len The number of bytes stored inside @p buf
 * @param context A context as provided by TIC::DatasetExtractor, used to retrieve the wrapped DatasetViewStub instance
 */
static void datasetViewStubUnwrapInvoke(const uint8_t* buf, unsigned int cnt, void* context) {
    if (context == NULL)
        return; /* Failsafe, discard if no context */
    DatasetViewStub* stub = static_cast<DatasetViewStub*>(context);
    stub->onDatasetExtractedCallback(buf, cnt);
}

/**
 * @brief Utility function to unwrap and invoke a TIC::DatasetExtractor instance's pushBytes() from a callback call from TIC::Unframer
 * 
 * @param buf A buffer containing new TIC frame bytes
 * @param len The number of bytes stored inside @p buf
 * @param context A context as provided by TIC::Unframer, used to retrieve the wrapped TIC::DatasetExtractor instance
 */
static void datasetExtractorUnwrapForwardFrameBytes(const uint8_t* buf, unsigned int cnt, void* context) {
	if (context == NULL)
		return; /* Failsafe, discard if no context */
	TIC::DatasetExtractor* de = static_cast<TIC::DatasetExtractor*>(context);
	de->pushBytes(buf, cnt);
}

/**
 * @brief Utility function to unwrap a TIC::DatasetExtractor instance and invoke reset() on it, from a callback call from TIC::Unframer
 * 
 * @param context A context as provided by TIC::Unframer, used to retrieve the wrapped TIC::DatasetExtractor instance
 */
static void datasetExtractorUnWrapFrameFinished(void *context) {
	if (context == NULL)
		return; /* Failsafe, discard if no context */
	TIC::DatasetExtractor* de = static_cast<TIC::DatasetExtractor*>(context);
	/* We have finished parsing a frame, if there is an open dataset, we should discard it and start over at the following frame */
	de->reset();
}

TEST(TicDatasetView_tests, TicDatasetView_correct_sample_typical_historical_dataset) {
	const char dataset[] = "ADCO 012345678901 E";

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
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
	const char dataset[] = "ADSC\t012345678901\t;";

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
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

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
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

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
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

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
	if (dv.isValid()) {
		FAILF("Expecting invalid dataset");
	}

	if (dv.decodedType != TIC::DatasetView::DatasetType::WrongCRC) {
		FAILF("Expecting incorrect CRC");
	}
}

TEST(TicDatasetView_tests, TicDatasetView_very_short) {
	const char dataset[] = "VTIC\t02\tJ";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Incorrect parsing of dataset");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'V', 'T', 'I', 'C'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataBuffer == nullptr) {
		FAILF("NULL dataBuffer");
	}
	if (dv.dataBuffer != &(datasetBuf[5])) {
		FAILF("Wrong dataset data, expected it to point to adress dataset+8, instead, it points to dataset+%u", static_cast<unsigned int>(dv.dataBuffer - datasetBuf));
	}
	if (dv.dataSz != 2) {
		FAILF("Wrong dataset data size: %u, expecting it to contain our short value (2 chars)", dv.dataSz);
	}
}

TEST(TicDatasetView_tests, TicDatasetView_very_long) {
	const char dataset[] = "PJOURF+1\t00008001 NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE\t9";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
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
	if (dv.dataBuffer != &(datasetBuf[9])) {
		FAILF("Wrong dataset data, expected it to point to adress dataset+8, instead, it points to dataset+%u", static_cast<unsigned int>(dv.dataBuffer - datasetBuf));
	}
	if (dv.dataSz != 98) {
		FAILF("Wrong dataset data size: %u, expecting it to contain our long value (98 chars)", dv.dataSz);
	}
}

TEST(TicDatasetView_tests, TicDatasetView_too_short) {
	const char dataset[] = "L V ";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	for (unsigned int datasetTestLength = 0; datasetTestLength <= static_cast<unsigned int>(strlen(dataset)); datasetTestLength++) {
		TIC::DatasetView dv((const uint8_t*)datasetBuf, datasetTestLength);
		
		if (dv.isValid() || dv.decodedType != TIC::DatasetView::DatasetType::Malformed) {
			FAILF("Expecting invalid dataset (with type Malformed, because it's too short to be parsed)");
		}
	}
}

TEST(TicDatasetView_tests, TicDatasetView_valid_with_horodate_without_value) {
	const char dataset[] = "DATE\tH101112010203\t\t-";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Dataset shoud be valid");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'D', 'A', 'T', 'E'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataSz != 0) {
		FAILF("Wrong dataset data size: %u, expecting it to contain an empty string", dv.dataSz);
	}

	if (!dv.horodate.isValid) {
		FAILF("Expected valid horodate");
	}
	{
		if (dv.horodate.season != TIC::Horodate::Season::Winter) {
			FAILF("Wrong horodate season");
		}
		if (dv.horodate.day != 12 ||
			dv.horodate.month != 11 ||
			dv.horodate.year != 2010 ||
			dv.horodate.hour != 1 ||
			dv.horodate.minute != 2 ||
			dv.horodate.second != 3) {
			FAILF("Wrong horodate date");
		}
	}
}

TEST(TicDatasetView_tests, TicDatasetView_standard_with_horodate_and_value) {
	const char dataset[] = "UMOY1\tH101112010203\t229\t'";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, static_cast<unsigned int>(strlen(dataset)));
	
	if (!dv.isValid()) {
		FAILF("Dataset shoud be valid");
	}

	if (dv.labelBuffer == nullptr) {
		FAILF("NULL labelBuffer");
	}
	std::vector<uint8_t> labelBufferVec(dv.labelBuffer, dv.labelBuffer + dv.labelSz);
	if (labelBufferVec != std::vector<uint8_t>({'U', 'M', 'O', 'Y', '1'})) {
		FAILF("Wrong dataset label: %s", vectorToHexString(labelBufferVec).c_str());
	}

	if (dv.dataBuffer == nullptr) {
		FAILF("NULL dataBuffer");
	}
	std::vector<uint8_t> dataBufferVec(dv.dataBuffer, dv.dataBuffer + dv.dataSz);
	if (dataBufferVec != std::vector<uint8_t>({'2', '2', '9'})) {
		FAILF("Wrong dataset data: %s", vectorToHexString(dataBufferVec).c_str());
	}

	if (!dv.horodate.isValid) {
		FAILF("Expected valid horodate");
	}
	{
		if (dv.horodate.season != TIC::Horodate::Season::Winter) {
			FAILF("Wrong horodate season");
		}
		if (dv.horodate.day != 12 ||
			dv.horodate.month != 11 ||
			dv.horodate.year != 2010 ||
			dv.horodate.hour != 1 ||
			dv.horodate.minute != 2 ||
			dv.horodate.second != 3) {
			FAILF("Wrong horodate date");
		}
	}
}

/**
 * @brief Send the content of a file to a TIC::Unframer, cutting it into chunks
 * 
 * @param ticData A buffer containing the byte sequence to inject
 * @param maxChunkSize The size of each chunks (except the last one, that may be smaller)
 * @param ticUnframer The TIC::Unframer object in which we will inject chunks
 */
static void TicUnframer_test_file_sent_by_chunks(const std::vector<uint8_t>& ticData, unsigned int maxChunkSize, TIC::Unframer& ticUnframer) {

	for (unsigned int bytesRead = 0; bytesRead < ticData.size();) {
		unsigned int nbBytesToRead = ticData.size() - bytesRead;
		if (nbBytesToRead > maxChunkSize) {
			nbBytesToRead = maxChunkSize; // Limit the number of bytes pushed to the provided max chunkSize
		}
		ticUnframer.pushBytes(&(ticData[bytesRead]), nbBytesToRead);
		bytesRead += nbBytesToRead;
	}
}

TEST(TicDatasetView_tests, Chunked_sample_unframe_dsextract_decode_historical_TIC) {
	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_3P_historical_TIC_sample.bin");

	for (unsigned int chunkSize = 1; chunkSize <= TIC::DatasetExtractor::MAX_DATASET_SIZE; chunkSize++) {
		DatasetViewStub stub;
		TIC::DatasetExtractor de(datasetViewStubUnwrapInvoke, &stub);
		TIC::Unframer tu(datasetExtractorUnwrapForwardFrameBytes, datasetExtractorUnWrapFrameFinished, &de);

		TicUnframer_test_file_sent_by_chunks(ticData, chunkSize, tu);

		/**
		 * @brief Sizes (in bytes) of the successive dataset in each repeated TIC frame
		 */
		std::size_t datasetExpectedSizes[] = { 19, /* ADCO label */
		                                       14, 11, 16, 11,
		                                       12, 12, 12, /* Three times IINST? labels (on each phase) */
		                                       11, 11, 11, /* Three times IMAX? labels (on each phase) */
		                                       12, 12, 9, 17, 9
		                                     };
		unsigned int nbExpectedDatasetPerFrame = sizeof(datasetExpectedSizes)/sizeof(datasetExpectedSizes[0]);

		// printf("%zu datasets decoded in total:\n", stub.datasetContentList.size());
		// for (unsigned int datasetIndex = 0; datasetIndex < stub.datasetContentList.size(); datasetIndex++) {
		// 	printf("At index %u: %s\n", datasetIndex, stub.datasetContentList[datasetIndex].toString().c_str());
		// }
		std::size_t expectedTotalDatasetCount = 6 * nbExpectedDatasetPerFrame; /* 6 frames, each containing the above datasets */
#ifdef __TIC_UNFRAMER_FORWARD_FRAME_BYTES_ON_THE_FLY__
		expectedTotalDatasetCount += 6;	/* 6 more trailing dataset within an unterminated frame */
#endif
		if (stub.datasetContentList.size() != expectedTotalDatasetCount) {
			FAILF("When using chunk size %u: Wrong dataset count: %zu, expected %zu\nDatasets received:\n%s", chunkSize, stub.datasetContentList.size(), expectedTotalDatasetCount, stub.toString().c_str());
		}
		if (stub.datasetContentList[3].label != "BASE") {
			FAILF("Unexpected 4th dataset label");
		}
		if (stub.datasetContentList[3].data != "010151175") {
			FAILF("Unexpected 4th dataset data");
		}
		if (stub.datasetContentList[3].horodate.isValid) {
			FAILF("Unexpected 4th dataset horodate (should be invalid)");
		}
	}
}

TEST(TicDatasetView_tests, Chunked_sample_unframe_dsextract_decode_standard_TIC) {

	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_1P_standard_TIC_sample.bin");

	for (unsigned int chunkSize = 1; chunkSize <= TIC::DatasetExtractor::MAX_DATASET_SIZE; chunkSize++) {
		DatasetViewStub stub;
		TIC::DatasetExtractor de(datasetViewStubUnwrapInvoke, &stub);
		TIC::Unframer tu(datasetExtractorUnwrapForwardFrameBytes, datasetExtractorUnWrapFrameFinished, &de);

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
		                                       109 /* Long PJOURF+1 label */
		                                     };
		unsigned int nbExpectedDatasetPerFrame = sizeof(datasetExpectedSizes)/sizeof(datasetExpectedSizes[0]);

		// printf("%zu datasets decoded in total:\n", stub.datasetContentList.size());
		// for (unsigned int datasetIndex = 0; datasetIndex < stub.datasetContentList.size(); datasetIndex++) {
		// 	printf("At index %u: %s\n", datasetIndex, stub.datasetContentList[datasetIndex].toString().c_str());
		// }
		std::size_t expectedTotalDatasetCount = 12 * nbExpectedDatasetPerFrame; /* 12 frames, each containing the above datasets */
		if (stub.datasetContentList.size() != expectedTotalDatasetCount) {
			FAILF("When using chunk size %u: Wrong dataset count: %zu, expected %zu\nDatasets received:\n%s", chunkSize, stub.datasetContentList.size(), expectedTotalDatasetCount, stub.toString().c_str());
		}
		if (stub.datasetContentList[0].label != "ADSC") {
			FAILF("Unexpected 1st dataset label");
		}
		if (stub.datasetContentList[0].data != "064468368739") {
			FAILF("Unexpected 1st dataset data");
		}
		if (stub.datasetContentList[0].horodate.isValid) {
			FAILF("Unexpected 1st dataset horodate (should be invalid)");
		}

		// At index 2: standard dataset with label="DATE", data="", horodate=2023/03/01 09:18:34 (winter)
		if (stub.datasetContentList[2].label != "DATE") {
			FAILF("Unexpected 3rd dataset label");
		}
		if (stub.datasetContentList[2].data != "") {
			FAILF("Unexpected 3rd dataset data");
		}
		if (!stub.datasetContentList[2].horodate.isValid) {
			FAILF("Unexpected 3rd dataset horodate (should be valid)");
		}
		{
			TIC::Horodate& horodate = stub.datasetContentList[2].horodate;
			if (horodate.season != TIC::Horodate::Season::Winter) {
				FAILF("Wrong horodate season associated with 3rd dataset");
			}
			if (horodate.day != 1 ||
				horodate.month != 3 ||
				horodate.year != 2023 ||
				horodate.hour != 9 ||
				horodate.minute != 18 ||
				horodate.second != 34) {
				FAILF("Wrong horodate associated with 3rd dataset");
			}
		}

		// At index 25: standard dataset with label="SMAXSN", data="01024", horodate=2023/03/01 06:52:53 (winter)
		if (stub.datasetContentList[25].label != "SMAXSN") {
			FAILF("Unexpected 26th dataset label");
		}
		if (stub.datasetContentList[25].data != "01024") {
			FAILF("Unexpected 26th dataset data");
		}
		if (!stub.datasetContentList[25].horodate.isValid) {
			FAILF("Unexpected 26th dataset horodate (should be valid)");
		}
		{
			TIC::Horodate& horodate = stub.datasetContentList[25].horodate;
			if (horodate.season != TIC::Horodate::Season::Winter) {
				FAILF("Wrong horodate season associated with 26th dataset");
			}
			if (horodate.day != 1 ||
				horodate.month != 3 ||
				horodate.year != 2023 ||
				horodate.hour != 6 ||
				horodate.minute != 52 ||
				horodate.second != 53) {
				FAILF("Wrong horodate associated with 26th dataset");
			}
		}
	}
}

TEST(TicDatasetView_tests, TicDatasetView_uint32FromValueBuffer) {
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("0"), 1) != 0) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("1"), 1) != 1) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("9"), 1) != 9) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("19"), 2) != 19) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("00"), 2) != 0) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("01"), 2) != 1) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("10"), 2) != 10) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("50"), 2) != 50) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("90"), 2) != 90) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("99"), 2) != 99) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("099"), 3) != 99) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("999"), 3) != 999) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("123"), 3) != 123) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("1234"), 4) != 1234) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("12345"), 5) != 12345) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("123456"), 6) != 123456) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("1234567"), 7) != 1234567) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("12345678"), 8) != 12345678) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("123456789"), 9) != 123456789) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("100000000"), 9) != 100000000) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("1000000000"), 10) != 1000000000) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("0000000001"), 10) != 1) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("0000000010"), 10) != 10) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("4294967294"), 10) != 4294967294) { /* 4294967294 is (uint32_t)(-2), this is the max acceptable value */
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("9999999999"), 10) != (uint32_t)-1) {
		FAILF("Failed parsing value");
	}
	if (TIC::DatasetView::uint32FromValueBuffer((const uint8_t*)("99999999999999"), 14) != (uint32_t)-1) {
		FAILF("Failed parsing value");
	}
}

TEST(TicHorodate_tests, TicHorodate_sample_date1) {
	char sampleHorodateAsCString[] = "H081225223518";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (!horodate.isValid) {
		FAILF("Expected valid horodate");
	}
	if (horodate.day != 25 ||
	    horodate.month != 12 ||
		horodate.year != 2008 ||
		horodate.hour != 22 ||
		horodate.minute != 35 ||
		horodate.second != 18) {
		FAILF("Wrong date extracted from horodate");
	}
	if (horodate.degradedTime) {
		FAILF("Unexpected degraded time");
	}
	if (horodate.season != TIC::Horodate::Winter) {
		FAILF("Expected winter season");
	}
}

TEST(TicHorodate_tests, TicHorodate_sample_date2) {
	char sampleHorodateAsCString[] = "E090714074553";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (!horodate.isValid) {
		FAILF("Expected valid horodate");
	}
	if (horodate.day != 14 ||
	    horodate.month != 7 ||
		horodate.year != 2009 ||
		horodate.hour != 7 ||
		horodate.minute != 45 ||
		horodate.second != 53) {
		FAILF("Wrong date extracted from horodate");
	}
	if (horodate.degradedTime) {
		FAILF("Unexpected degraded time");
	}
	if (horodate.season != TIC::Horodate::Summer) {
		FAILF("Expected summer season");
	}
}

TEST(TicHorodate_tests, TicHorodate_sample_season_NA) {
	char sampleHorodateAsCString[] = " 090714074553";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (!horodate.isValid) {
		FAILF("Expected valid horodate");
	}
	if (horodate.day != 14 ||
	    horodate.month != 7 ||
		horodate.year != 2009 ||
		horodate.hour != 7 ||
		horodate.minute != 45 ||
		horodate.second != 53) {
		FAILF("Wrong date extracted from horodate");
	}
	if (horodate.degradedTime) {
		FAILF("Unexpected degraded time");
	}
	if (horodate.season != TIC::Horodate::Unknown) {
		FAILF("Expected no season");
	}
}

TEST(TicHorodate_tests, TicHorodate_degraded_realtime_clock) {
	char sampleHorodateAsCString[] = "h000102030405";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (!horodate.isValid) {
		FAILF("Expected valid horodate");
	}
	if (horodate.day != 2 ||
	    horodate.month != 1 ||
		horodate.year != 2000 ||
		horodate.hour != 3 ||
		horodate.minute != 4 ||
		horodate.second != 5) {
		FAILF("Wrong date extracted from horodate");
	}
	if (!horodate.degradedTime) {
		FAILF("Expected degraded time");
	}
	if (horodate.season != TIC::Horodate::Winter) {
		FAILF("Expected winter season");
	}
}

TEST(TicHorodate_tests, TicHorodate_wrong_characters) {
	char sampleHorodateAsCString[] = "HA00102030405";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_month) {
	char sampleHorodateAsCString1[] = "H010001130405";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString1), strlen(sampleHorodateAsCString1));

	if (horodate1.isValid) {
		FAILF("Expected invalid horodate (month 0)");
	}

	char sampleHorodateAsCString2[] = "H011301130405";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString2), strlen(sampleHorodateAsCString2));

	if (horodate2.isValid) {
		FAILF("Expected invalid horodate (month 13)");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_day) {
	char sampleHorodateAsCString1[] = "H010100130405";
	TIC::Horodate horodate1 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString1), strlen(sampleHorodateAsCString1));

	if (horodate1.isValid) {
		FAILF("Expected invalid horodate (day 0)");
	}

	char sampleHorodateAsCString2[] = "H010132130405";
	TIC::Horodate horodate2 = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString2), strlen(sampleHorodateAsCString2));

	if (horodate2.isValid) {
		FAILF("Expected invalid horodate (day 32)");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_hour) {
	char sampleHorodateAsCString[] = "H010101250101";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate (hour 25)");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_minute) {
	char sampleHorodateAsCString[] = "H010101016001";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate (minute 60)");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_second) {
	char sampleHorodateAsCString[] = "H010101010160";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate (second 60)");
	}
}

#ifndef USE_CPPUTEST
void runTicDatasetViewAllUnitTests() {
	TicDatasetView_correct_sample_typical_historical_dataset();
	TicDatasetView_correct_sample_typical_standard_dataset();
	TicDatasetView_extra_leading_start_marker();
	TicDatasetView_extra_trailing_end_marker();
	TicDatasetView_wrong_crc();
	TicDatasetView_very_short();
	TicDatasetView_very_long();
	TicDatasetView_too_short();
	TicDatasetView_valid_with_horodate_without_value();
	TicDatasetView_standard_with_horodate_and_value();
	TicDatasetView_uint32FromValueBuffer();
	Chunked_sample_unframe_dsextract_decode_historical_TIC();
	Chunked_sample_unframe_dsextract_decode_standard_TIC();
	TicHorodate_sample_date1();
	TicHorodate_sample_date2();
	TicHorodate_sample_season_NA();
	TicHorodate_degraded_realtime_clock();
	TicHorodate_wrong_characters();
	TicHorodate_impossible_month();
	TicHorodate_impossible_day();
	TicHorodate_impossible_hour();
	TicHorodate_impossible_minute();
	TicHorodate_impossible_second();
}

#endif	// USE_CPPUTEST
