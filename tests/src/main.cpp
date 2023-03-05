extern void runTicUnframerAllUnitTests();
extern void runTicDatasetExtractorAllUnitTests();
extern void runFixedSizeRingBufferAllUnitTests();

int main(void) {
    runTicUnframerAllUnitTests();
    runTicDatasetExtractorAllUnitTests();
    runFixedSizeRingBufferAllUnitTests();
}