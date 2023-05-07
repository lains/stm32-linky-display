#include "gmock/gmock.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdint.h>

#include "FixedSizeRingBuffer.h"

TEST(FixedSizeRingBuffer_tests, instanciation) {
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

TEST(FixedSizeRingBuffer_tests, isEmpty) {
    FixedSizeRingBuffer<uint16_t, 128> rbuf;

    for(unsigned int i = 0; i < rbuf.getCapacity(); i++) {
        rbuf.push(i && static_cast<uint16_t>(-1));
        if (rbuf.isEmpty())
            FAIL() << "FixedSizeRingBuffer should not be empty after one element has been inserted";
    }
}

TEST(FixedSizeRingBuffer_tests, reset) {
    FixedSizeRingBuffer<uint16_t, 128> rbuf;

    rbuf.push(0);
    rbuf.push(0);
    rbuf.push(0);

    EXPECT_EQ(rbuf.getCount(), static_cast<std::size_t>(3));

    rbuf.reset();

    EXPECT_EQ(rbuf.getCount(), static_cast<std::size_t>(0));

    rbuf.push(3);

    EXPECT_EQ(rbuf.pop(), 3);
}

TEST(FixedSizeRingBuffer_tests, isFull) {
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

TEST(FixedSizeRingBuffer_tests, fill_then_flush) {
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

TEST(FixedSizeRingBuffer_tests, pop_non_existing_element) {
    FixedSizeRingBuffer<uint8_t, 128> rbuf;

    rbuf.push(0xff);
    if (rbuf.pop() != 0xff)
        FAIL() << "Expecting popped value 0xff";
    if (rbuf.pop() != 0x00)
        FAIL() << "Expecting empty popped value 0x00";
}

TEST(FixedSizeRingBuffer_tests, on_overflow) {
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

TEST(FixedSizeRingBuffer_tests, getReverse) {
    FixedSizeRingBuffer<uint16_t, 256> rbuf;
    rbuf.push(0x01);
    rbuf.push(0x02);
    rbuf.push(0x03);
    rbuf.push(0x04);

    EXPECT_EQ(rbuf.getCount(), 4);

    /* Now check we the last 4 elements */
    EXPECT_EQ(rbuf.getReverse(0), 0x04);
    EXPECT_EQ(rbuf.getReverse(0), 0x04);
    EXPECT_EQ(rbuf.getReverse(1), 0x03);
    EXPECT_EQ(rbuf.getReverse(1), 0x03);
    EXPECT_EQ(rbuf.getReverse(2), 0x02);
    EXPECT_EQ(rbuf.getReverse(2), 0x02);
    EXPECT_EQ(rbuf.getReverse(3), 0x01);
    EXPECT_EQ(rbuf.getReverse(3), 0x01);

    EXPECT_EQ(rbuf.getCount(), 4);

    rbuf.pop();
    rbuf.pop();
    rbuf.pop();
    rbuf.pop();

    /* On an empty buffer, we return the default value */
    rbuf.reset();
    EXPECT_EQ(rbuf.getReverse(1), 0);
    EXPECT_EQ(rbuf.getReverse(2), 0);
    EXPECT_EQ(rbuf.getReverse(257), 0);
}

TEST(FixedSizeRingBuffer_tests, getPtrToLast) {
    FixedSizeRingBuffer<uint16_t, 256> rbuf;

    rbuf.push(1);
    rbuf.push(2);
    rbuf.push(3);
    rbuf.push(4);

    uint16_t* last = rbuf.getPtrToLast();
    
    EXPECT_EQ(*last, 4);
    *last = 0xa5a5;

    EXPECT_EQ(rbuf.pop(), 1);
    EXPECT_EQ(rbuf.pop(), 2);
    EXPECT_EQ(rbuf.pop(), 3);
    EXPECT_EQ(rbuf.pop(), 0xa5a5);
}

TEST(FixedSizeRingBuffer_tests, getPtrToLast_empty) {
    FixedSizeRingBuffer<uint16_t, 256> rbuf;

    EXPECT_EQ(rbuf.getPtrToLast(), nullptr);
}

TEST(FixedSizeRingBuffer_tests, getPtrToLast_full_wrap) {
    FixedSizeRingBuffer<uint16_t, 2> rbuf;

    rbuf.push(1);
    rbuf.push(2);

    EXPECT_EQ(*(rbuf.getPtrToLast()), 2);
}

TEST(FixedSizeRingBuffer_tests, toVector) {
    FixedSizeRingBuffer<uint16_t, 256> rbuf;

    rbuf.push(1);
    rbuf.push(2);
    rbuf.push(3);
    rbuf.push(4);

    EXPECT_EQ(rbuf.toVector(), std::vector<uint16_t>({1, 2, 3, 4}));
}

TEST(FixedSizeRingBuffer_tests, toVector_empty) {
    FixedSizeRingBuffer<uint16_t, 256> rbuf;

    EXPECT_EQ(rbuf.toVector(), std::vector<uint16_t>());
}