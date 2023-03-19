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

	void onDatasetExtractedCallback(const uint8_t* buf, size_t cnt) {
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
static void datasetViewStubUnwrapInvoke(const uint8_t* buf, std::size_t cnt, void* context) {
    if (context == NULL)
        return; /* Failsafe, discard if no context */
    DatasetViewStub* stub = static_cast<DatasetViewStub*>(context);
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
	const char dataset[] = "ADSC\t012345678901\t;";

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

	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
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

	TIC::DatasetView dv((const uint8_t*)datasetBuf, (std::size_t)(strlen(dataset)));
	
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
		FAILF("Wrong dataset data, expected it to point to adress dataset+8, instead, it points to dataset+%zu", dv.dataBuffer - datasetBuf);
	}
	if (dv.dataSz != 2) {
		FAILF("Wrong dataset data size: %zu, expected it contain our short value (2 chars)", dv.dataSz);
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
	if (dv.dataBuffer != &(datasetBuf[9])) {
		FAILF("Wrong dataset data, expected it to point to adress dataset+8, instead, it points to dataset+%zu", dv.dataBuffer - datasetBuf);
	}
	if (dv.dataSz != 98) {
		FAILF("Wrong dataset data size: %zu, expected it contain our long value (98 chars)", dv.dataSz);
	}
}

TEST(TicDatasetView_tests, TicDatasetView_too_short) {
	const char dataset[] = "L V ";
	const uint8_t* datasetBuf = reinterpret_cast<const unsigned char*>(dataset);

	for (std::size_t datasetTestLength = 0; datasetTestLength <= (std::size_t)(strlen(dataset)); datasetTestLength++) {
		TIC::DatasetView dv((const uint8_t*)datasetBuf, datasetTestLength);
		
		if (dv.isValid() || dv.decodedType != TIC::DatasetView::DatasetType::Malformed) {
			FAILF("Expecting invalid dataset (with type Malformed, because it's too short to be parsed)");
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
		DatasetViewStub stub;
		TIC::DatasetExtractor de(datasetViewStubUnwrapInvoke, &stub);
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
		if (stub.datasetContentList.size() != expectedTotalDatasetCount) {
			FAILF("When using chunk size %zu: Wrong dataset count: %zu, expected %zu\nDatasets received:\n%s", chunkSize, stub.datasetContentList.size(), expectedTotalDatasetCount, stub.toString().c_str());
		}
		// char firstDatasetAsCString[] = "ADCO 056234673197 O";
		// std::vector<uint8_t> expectedFirstDatasetInFrame(firstDatasetAsCString, firstDatasetAsCString+strlen(firstDatasetAsCString));
		// if (stub.datasetContentList[0] != expectedFirstDatasetInFrame) {
		// 	FAILF("Unexpected first dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[0]).c_str(), vectorToHexString(expectedFirstDatasetInFrame).c_str());
		// }
		// char lastDatasetAsCString[] = "PPOT 00 #";
		// std::vector<uint8_t> expectedLastDatasetInFrame(lastDatasetAsCString, lastDatasetAsCString+strlen(lastDatasetAsCString));
		// if (stub.decodedDatasetList[nbExpectedDatasetPerFrame-1] != expectedLastDatasetInFrame) {
		// 	FAILF("Unexpected last dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[nbExpectedDatasetPerFrame-1]).c_str(), vectorToHexString(expectedLastDatasetInFrame).c_str());
		// }
		printf("%zu datasets decoded in total:\n", stub.datasetContentList.size());
		for (std::size_t datasetIndex = 0; datasetIndex < stub.datasetContentList.size(); datasetIndex++) {
			printf("At index %zu: %s\n", datasetIndex, stub.datasetContentList[datasetIndex].toString().c_str());
		}
	}
}

TEST(TicDatasetView_tests, Chunked_sample_unframe_dsextract_decode_standard_TIC) {

	std::vector<uint8_t> ticData = readVectorFromDisk("./samples/continuous_linky_1P_standard_TIC_sample.bin");

	for (size_t chunkSize = 1; chunkSize <= TIC::DatasetExtractor::MAX_DATASET_SIZE; chunkSize++) {
		DatasetViewStub stub;
		TIC::DatasetExtractor de(datasetViewStubUnwrapInvoke, &stub);
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
		if (stub.datasetContentList.size() != expectedTotalDatasetCount) {
			FAILF("When using chunk size %zu: Wrong dataset count: %zu, expected %zu\nDatasets received:\n%s", chunkSize, stub.datasetContentList.size(), expectedTotalDatasetCount, stub.toString().c_str());
		}
		// char firstDatasetAsCString[] = "ADSC\t064468368739\tJ";
		// std::vector<uint8_t> expectedFirstDatasetInFrame(firstDatasetAsCString, firstDatasetAsCString+strlen(firstDatasetAsCString));
		// if (stub.decodedDatasetList[0] != expectedFirstDatasetInFrame) {
		// 	FAILF("Unexpected first dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[0]).c_str(), vectorToHexString(expectedFirstDatasetInFrame).c_str());
		// }
		// char lastDatasetAsCString[] = "PJOURF+1\t00008001 NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE\t9";
		// std::vector<uint8_t> expectedLastDatasetInFrame(lastDatasetAsCString, lastDatasetAsCString+strlen(lastDatasetAsCString));
		// if (stub.decodedDatasetList[nbExpectedDatasetPerFrame-1] != expectedLastDatasetInFrame) {
		// 	FAILF("Unexpected last dataset in first frame:\nGot:      %s\nExpected: %s\n", vectorToHexString(stub.decodedDatasetList[nbExpectedDatasetPerFrame-1]).c_str(), vectorToHexString(expectedLastDatasetInFrame).c_str());
		// }
		printf("%zu datasets decoded in total:\n", stub.datasetContentList.size());
		for (std::size_t datasetIndex = 0; datasetIndex < stub.datasetContentList.size(); datasetIndex++) {
			printf("At index %zu: %s\n", datasetIndex, stub.datasetContentList[datasetIndex].toString().c_str());
		}
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
	if (horodate.day != 25 ||
	    horodate.month != 12 ||
		horodate.year != 2008 ||
		horodate.hour != 22 ||
		horodate.minute != 35 ||
		horodate.second != 18) {
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
	char sampleHorodateAsCString[] = "H010001130405";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_day) {
	char sampleHorodateAsCString[] = "H010100130405";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_hour) {
	char sampleHorodateAsCString[] = "H010101250101";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_minute) {
	char sampleHorodateAsCString[] = "H010101016001";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate");
	}
}

TEST(TicHorodate_tests, TicHorodate_impossible_second) {
	char sampleHorodateAsCString[] = "H010101010160";
	TIC::Horodate horodate = TIC::Horodate::fromLabelBytes(reinterpret_cast<uint8_t*>(sampleHorodateAsCString), strlen(sampleHorodateAsCString));

	if (horodate.isValid) {
		FAILF("Expected invalid horodate");
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
