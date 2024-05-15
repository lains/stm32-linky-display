#include "gmock/gmock.h"
#include "../tools/Tools.h"
#include "TIC/Unframer.h"
#include "TicProcessingContext.h"
#include "PowerHistory.h"
#include "TicFrameParser.h"
#include <iostream>

void breakp(void) {

}

TEST(EndToEndDecoding_tests, TriphaseSample) {
    breakp();
    struct Stm32SerialDriver dummyTicSerial;

    PowerHistory powerHistory(PowerHistory::Per5Seconds);

    TicFrameParser ticParser(PowerHistory::unWrapOnNewPowerData, (void *)(&powerHistory));

    auto onFrameCompleteBlinkGreenLedAndInvokeHandler = [](void* context) {
        TicFrameParser::unwrapInvokeOnFrameComplete(context);   /* Invoke the frameparser's onFrameComplete handler */
        //std::cout << "Got frame complete... target would toggle green LED\n";
    };

    TIC::Unframer ticUnframer(TicFrameParser::unwrapInvokeOnFrameNewBytes,
                              onFrameCompleteBlinkGreenLedAndInvokeHandler,
                              (void *)(&ticParser));

    TicProcessingContext ticContext(dummyTicSerial, ticUnframer);

    powerHistory.setContext(&ticContext);

	//std::vector<uint8_t> ticData = readVectorFromDisk("./ticdecodecpp/test/samples/continuous_linky_3P_historical_TIC_2024_sample.bin");
    std::vector<uint8_t> ticData = readVectorFromDisk("./ticdecodecpp/test/samples/continuous_linky_3P_historical_TIC_with_rx_errors.bin");

    assert(ticData.size() != 0);

    //std::cout << "TIC sample is " << ticData.size() << " bytes long\n";

	for (unsigned int pos = 0; pos < ticData.size(); pos++) {
        size_t incomingBytesCount = 1;
        uint8_t incomingByte = ticData[pos];
        //std::cerr << "Pushing byte " << vectorToHexString(std::vector<uint8_t>(1, incomingByte)) << "\n";
        std::size_t processedBytesCount = ticUnframer.pushBytes(&incomingByte, incomingBytesCount);
        if (processedBytesCount < incomingBytesCount) {
            size_t lostBytesCount = incomingBytesCount - processedBytesCount;
            if (lostBytesCount != 0) {
                throw std::runtime_error("Bytes lost");
            }
        }
    };
}

TEST(EndToEndDecoding_tests, MonophaseMidnightSample) {
    breakp();
    struct Stm32SerialDriver dummyTicSerial;

    PowerHistory powerHistory(PowerHistory::Per5Seconds);

    TicFrameParser ticParser(PowerHistory::unWrapOnNewPowerData, (void *)(&powerHistory));

    bool newDayDetected = false;

    auto onDayOver = [](void* context) {
        std::cout << "New day detected\n";
        if (context != nullptr) {
            bool* newDayDetected = static_cast<bool*>(context);
            *newDayDetected = true;
        }
    };
    ticParser.onDayOverInvoke(onDayOver, static_cast<void*>(&newDayDetected));

    auto onFrameCompleteBlinkGreenLedAndInvokeHandler = [](void* context) {
        TicFrameParser::unwrapInvokeOnFrameComplete(context);   /* Invoke the frameparser's onFrameComplete handler */
        //std::cout << "Got frame complete... target would toggle green LED\n";
    };

    TIC::Unframer ticUnframer(TicFrameParser::unwrapInvokeOnFrameNewBytes,
                              onFrameCompleteBlinkGreenLedAndInvokeHandler,
                              (void *)(&ticParser));

    TicProcessingContext ticContext(dummyTicSerial, ticUnframer);

    powerHistory.setContext(&ticContext);

    std::vector<uint8_t> ticData = readVectorFromDisk("./ticdecodecpp/test/samples/linky_1P_midnight.bin");

    assert(ticData.size() != 0);

    std::cout << "TIC sample is " << ticData.size() << " bytes long\n";

	for (unsigned int pos = 0; pos < ticData.size(); pos++) {
        size_t incomingBytesCount = 1;
        uint8_t incomingByte = ticData[pos];
        //std::cerr << "Pushing byte " << vectorToHexString(std::vector<uint8_t>(1, incomingByte)) << "\n";
        std::size_t processedBytesCount = ticUnframer.pushBytes(&incomingByte, incomingBytesCount);
        if (processedBytesCount < incomingBytesCount) {
            size_t lostBytesCount = incomingBytesCount - processedBytesCount;
            if (lostBytesCount != 0) {
                throw std::runtime_error("Bytes lost");
            }
        }
    };
    EXPECT_TRUE(newDayDetected);
}
