#include <gtest/gtest.h>
#include "ring_buffer.h"

using namespace musicplayer;

TEST(RingBuffer, CapacityIsPowerOfTwo) {
    RingBuffer<float> rb(100);
    EXPECT_EQ(rb.capacity(), 128u);
    RingBuffer<float> rb2(128);
    EXPECT_EQ(rb2.capacity(), 128u);
}

TEST(RingBuffer, WriteReadSingle) {
    RingBuffer<float> rb(16);
    float data[] = {1.0f, 2.0f, 3.0f};
    EXPECT_EQ(rb.write(data, 3), 3u);
    EXPECT_EQ(rb.available(), 3u);

    float out[3] = {};
    EXPECT_EQ(rb.read(out, 3), 3u);
    EXPECT_FLOAT_EQ(out[0], 1.0f);
    EXPECT_FLOAT_EQ(out[1], 2.0f);
    EXPECT_FLOAT_EQ(out[2], 3.0f);
}

TEST(RingBuffer, WriteReadPartial) {
    RingBuffer<float> rb(8);
    float data[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    EXPECT_EQ(rb.write(data, 5), 5u);
    EXPECT_EQ(rb.available(), 5u);

    float out[3] = {};
    EXPECT_EQ(rb.read(out, 3), 3u);
    EXPECT_EQ(rb.available(), 2u);
}

TEST(RingBuffer, FullBufferRejectsWrite) {
    RingBuffer<float> rb(8);
    float data[16] = {};
    size_t wrote = rb.write(data, 8);
    EXPECT_EQ(wrote, 8u);
    EXPECT_TRUE(rb.full());

    wrote = rb.write(data, 1);
    EXPECT_EQ(wrote, 0u);
}

TEST(RingBuffer, WrapAround) {
    RingBuffer<float> rb(8);
    float data[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    EXPECT_EQ(rb.write(data, 8), 8u);

    float out[4] = {};
    EXPECT_EQ(rb.read(out, 4), 4u);
    EXPECT_EQ(rb.available(), 4u);

    float data2[] = {9.0f, 10.0f, 11.0f, 12.0f};
    EXPECT_EQ(rb.write(data2, 4), 4u);

    float out2[8] = {};
    EXPECT_EQ(rb.read(out2, 8), 8u);
    EXPECT_FLOAT_EQ(out2[0], 5.0f);
    EXPECT_FLOAT_EQ(out2[3], 8.0f);
    EXPECT_FLOAT_EQ(out2[4], 9.0f);
    EXPECT_FLOAT_EQ(out2[7], 12.0f);
}

TEST(RingBuffer, Clear) {
    RingBuffer<float> rb(16);
    float data[] = {1.0f, 2.0f, 3.0f};
    rb.write(data, 3);
    EXPECT_EQ(rb.available(), 3u);

    rb.clear();
    EXPECT_EQ(rb.available(), 0u);
    EXPECT_TRUE(rb.empty());
}

TEST(RingBuffer, FreeSlots) {
    RingBuffer<float> rb(8);
    EXPECT_EQ(rb.freeSlots(), 8u);
    float data[3] = {};
    rb.write(data, 3);
    EXPECT_EQ(rb.freeSlots(), 5u);
}
