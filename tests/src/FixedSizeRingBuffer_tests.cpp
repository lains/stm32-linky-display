#include "TestHarness.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>
/*
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iterator>
*/

#include "FixedSizeRingBuffer.h"

TEST_GROUP(FixedSizeRingBuffer_tests) {
};

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_instanciation)
{
	FixedSizeRingBuffer<uint16_t, 128> rbuf;
	if (rbuf.getCount() != 0)
        FAILF("FixedSizeRingBuffer.getSize() should be 0 at construction");
	if (rbuf.getCapacity() != 128)
        FAILF("FixedSizeRingBuffer.getCapacity() should match capacity provided at construction");
	if (rbuf.isFull() != false)
        FAILF("FixedSizeRingBuffer should not be full at construction");
	if (rbuf.isEmpty() != true)
        FAILF("FixedSizeRingBuffer should be empty at construction");
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_isEmpty)
{
    FixedSizeRingBuffer<uint16_t, 128> rbuf;

    for(unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        rbuf.push(i && static_cast<uint16_t>(-1));
        if (rbuf.isEmpty())
            FAILF("FixedSizeRingBuffer should not be empty after one element has been inserted");
    }
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_isFull)
{
    FixedSizeRingBuffer<uint16_t, 128> rbuf;

    for(unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        if (rbuf.isFull())
            FAILF("FixedSizeRingBuffer should not yet be full");
        rbuf.push(i && static_cast<uint16_t>(-1));
    }

    if (!rbuf.isFull())
        FAILF("FixedSizeRingBuffer should now be full");

    if (rbuf.getCount() != rbuf.getCapacity())
        FAILF("FixedSizeRingBuffer full buffer should reach maximum count");
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_fill_then_flush)
{
    FixedSizeRingBuffer<uint8_t, 0x80> rbuf;
    for (unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        rbuf.push(i | 0x80);
    }

    for (unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        uint8_t elt = rbuf.pop();
        if (elt != (i | 0x80))
            FAILF("FixedSizeRingBuffer pop should return elements in FIFO order. At pos %u, expected %02hhx, got %02hhx", i, i | 0x80, elt);
    }
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_pop_empty_element)
{
    FixedSizeRingBuffer<uint8_t, 128> rbuf;

    rbuf.push(0xff);
    if (rbuf.pop() != 0xff)
        FAILF("Expecting popped value 0xff");
    if (rbuf.pop() != 0x00)
        FAILF("Expecting empty popped value 0x00");
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_overflow)
{
    FixedSizeRingBuffer<uint16_t, 256> rbuf;
    for (unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        rbuf.push((i & 0xffff) + 1);
    }
    rbuf.push(static_cast<uint16_t>(-1)); /* Overflow the buffer */

    /* Now check we have lost the very fist element */
    for (unsigned int i = 0; i < rbuf.getCapacity()-1; i++) {
        uint16_t elt = rbuf.pop();
        if (elt != ((i & 0xffff) + 2))
            FAILF("Unexpected popped element. At pop #%u, got %04x", i, elt);
    }
    uint16_t elt = rbuf.pop();
    if (elt != static_cast<uint16_t>(-1))
        FAILF("Unexpected popped element. At final pop, got %04x, expected %04x", elt, static_cast<uint16_t>(-1));
}

#ifndef USE_CPPUTEST
void runFixedSizeRingBufferAllUnitTests() {
	FixedSizeRingBuffer_instanciation();
    FixedSizeRingBuffer_isEmpty();
    FixedSizeRingBuffer_isFull();
    FixedSizeRingBuffer_fill_then_flush();
    FixedSizeRingBuffer_pop_empty_element();
    FixedSizeRingBuffer_overflow();
}
#endif	// USE_CPPUTEST
