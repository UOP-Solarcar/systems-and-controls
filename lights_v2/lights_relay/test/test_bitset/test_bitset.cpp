#include <unity.h>
#include "bitset.h"

void setUp(void) {}
void tearDown(void) {}

void test_initial_zero(void) {
    Bitset b{0};
    TEST_ASSERT_TRUE(b.none());
    TEST_ASSERT_FALSE(b.any());
    TEST_ASSERT_EQUAL_UINT8(0, b.count());
}

void test_set_and_test(void) {
    Bitset b{0};
    b.set(3);
    TEST_ASSERT_TRUE(b.test(3));
    TEST_ASSERT_FALSE(b.test(0));
    TEST_ASSERT_TRUE(b.any());
}

void test_reset(void) {
    Bitset b{0};
    b.set(5);
    b.reset(5);
    TEST_ASSERT_FALSE(b.test(5));
    TEST_ASSERT_TRUE(b.none());
}

void test_flip(void) {
    Bitset b{0};
    b.flip(2);
    TEST_ASSERT_TRUE(b.test(2));
    b.flip(2);
    TEST_ASSERT_FALSE(b.test(2));
}

void test_all(void) {
    Bitset b{0xFF};
    TEST_ASSERT_TRUE(b.all());
    TEST_ASSERT_TRUE(b.any());
    TEST_ASSERT_EQUAL_UINT8(8, b.count());
}

void test_count(void) {
    Bitset b{0};
    b.set(0);
    b.set(3);
    b.set(7);
    TEST_ASSERT_EQUAL_UINT8(3, b.count());
}

void test_multiple_set_reset(void) {
    Bitset b{0};
    for (int i = 0; i < 8; i++) {
        b.set(i);
    }
    TEST_ASSERT_TRUE(b.all());

    b.reset(4);
    TEST_ASSERT_FALSE(b.all());
    TEST_ASSERT_EQUAL_UINT8(7, b.count());
    TEST_ASSERT_FALSE(b.test(4));
    TEST_ASSERT_TRUE(b.test(3));
}

void test_set_idempotent(void) {
    Bitset b{0};
    b.set(1);
    b.set(1);
    TEST_ASSERT_EQUAL_UINT8(1, b.count());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_zero);
    RUN_TEST(test_set_and_test);
    RUN_TEST(test_reset);
    RUN_TEST(test_flip);
    RUN_TEST(test_all);
    RUN_TEST(test_count);
    RUN_TEST(test_multiple_set_reset);
    RUN_TEST(test_set_idempotent);
    return UNITY_END();
}
