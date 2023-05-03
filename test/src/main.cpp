extern void runTicUnframerAllUnitTests();
extern void runTicDatasetExtractorAllUnitTests();
extern void runFixedSizeRingBufferAllUnitTests();
extern void runTicDatasetViewAllUnitTests();

int main(void) {
    runTicUnframerAllUnitTests();
    runTicDatasetExtractorAllUnitTests();
    runTicDatasetViewAllUnitTests();
}