#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>

#include "FixedSizeRingBuffer.h"

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_instanciation) {
	FixedSizeRingBuffer<uint16_t, 128> rbuf;
	if (rbuf.getCount() != 0)
        FAIL() << "FixedSizeRingBuffer.getSize() should be 0 at construction";
	if (rbuf.getCapacity() != 128)
        FAIL() << "FixedSizeRingBuffer.getCapacity() should match capacity provided at construction";
	if (rbuf.isFull() != false)
        FAIL() << "FixedSizeRingBuffer should not be full at construction";
	if (rbuf.isEmpty() != true)
        FAIL() << "FixedSizeRingBuffer should be empty at construction";
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_isEmpty) {
    FixedSizeRingBuffer<uint16_t, 128> rbuf;

    for(unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        rbuf.push(i && static_cast<uint16_t>(-1));
        if (rbuf.isEmpty())
            FAIL() << "FixedSizeRingBuffer should not be empty after one element has been inserted";
    }
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_isFull) {
    FixedSizeRingBuffer<uint16_t, 128> rbuf;

    for(unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        if (rbuf.isFull())
            FAIL() << "FixedSizeRingBuffer should not yet be full";
        rbuf.push(i && static_cast<uint16_t>(-1));
    }

    if (!rbuf.isFull())
        FAIL() << "FixedSizeRingBuffer should now be full";

    if (rbuf.getCount() != rbuf.getCapacity())
        FAIL() << "FixedSizeRingBuffer full buffer should reach maximum count";
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_fill_then_flush) {
    FixedSizeRingBuffer<uint8_t, 0x80> rbuf;
    for (unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        rbuf.push(i | 0x80);
    }

    for (unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        uint8_t elt = rbuf.pop();
        if (elt != (i | 0x80))
            FAIL() << "FixedSizeRingBuffer pop should return elements in FIFO order. At pos " << i << ", expected " << static_cast<unsigned int>(i | 0x80) << ", got " << static_cast<unsigned int>(elt);
    }
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_pop_empty_element) {
    FixedSizeRingBuffer<uint8_t, 128> rbuf;

    rbuf.push(0xff);
    if (rbuf.pop() != 0xff)
        FAIL() << "Expecting popped value 0xff";
    if (rbuf.pop() != 0x00)
        FAIL() << "Expecting empty popped value 0x00";
}

TEST(FixedSizeRingBuffer_tests, FixedSizeRingBuffer_overflow) {
    FixedSizeRingBuffer<uint16_t, 256> rbuf;
    for (unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        rbuf.push((i & 0xffff) + 1);
    }
    rbuf.push(static_cast<uint16_t>(-1)); /* Overflow the buffer */

    /* Now check we have lost the very fist element */
    for (unsigned int i = 0; i < rbuf.getCapacity()-1; i++) {
        uint16_t elt = rbuf.pop();
        if (elt != ((i & 0xffff) + 2))
            FAIL() << "Unexpected popped element. At pop #" << i << ", got " << static_cast<unsigned int>(elt);
    }
    uint16_t elt = rbuf.pop();
    if (elt != static_cast<uint16_t>(-1))
        FAIL() << "Unexpected popped element. At final pop, got " << static_cast<unsigned int>(elt) << ", expected " << static_cast<unsigned int>(static_cast<uint16_t>(-1));
}
