#include <unity.h>
#include "config.h"

void setUp() {}
void tearDown() {}

void test_config_constants() {
    TEST_ASSERT_EQUAL(17, VOLUME);
    TEST_ASSERT_EQUAL(50, HISTORY_SIZE);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_config_constants);
    return UNITY_END();
}
