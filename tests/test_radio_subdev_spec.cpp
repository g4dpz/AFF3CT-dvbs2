#include <iostream>
#include <string>
#include <cassert>

#include "Factory/Module/Radio/Radio.hpp"

// Simple test framework macros
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        std::cout << "  TEST: " << name << " ... "; \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        std::cout << "PASSED" << std::endl; \
    } while(0)

#define FAIL(msg) \
    do { \
        std::cout << "FAILED: " << msg << std::endl; \
    } while(0)

// =============================================================================
// Test: B200 + empty rx_subdev_spec → returns "A:A"
// =============================================================================
void test_b200_empty_rx_subdev()
{
    TEST("B200 + empty rx_subdev_spec -> returns A:A");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.rx_subdev_spec = "";

    std::string result = radio.get_effective_rx_subdev_spec();

    if (result == "A:A")
        PASS();
    else
        FAIL("Expected 'A:A', got: '" + result + "'");
}

// =============================================================================
// Test: B200 + empty tx_subdev_spec → returns "A:A"
// =============================================================================
void test_b200_empty_tx_subdev()
{
    TEST("B200 + empty tx_subdev_spec -> returns A:A");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.tx_subdev_spec = "";

    std::string result = radio.get_effective_tx_subdev_spec();

    if (result == "A:A")
        PASS();
    else
        FAIL("Expected 'A:A', got: '" + result + "'");
}

// =============================================================================
// Test: B200 + user-provided rx_subdev_spec "B:0" → returns "B:0" (not overwritten)
// =============================================================================
void test_b200_user_rx_subdev()
{
    TEST("B200 + user-provided rx_subdev_spec B:0 -> returns B:0");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.rx_subdev_spec = "B:0";

    std::string result = radio.get_effective_rx_subdev_spec();

    if (result == "B:0")
        PASS();
    else
        FAIL("Expected 'B:0', got: '" + result + "'");
}

// =============================================================================
// Test: B200 + user-provided tx_subdev_spec "B:0" → returns "B:0" (not overwritten)
// =============================================================================
void test_b200_user_tx_subdev()
{
    TEST("B200 + user-provided tx_subdev_spec B:0 -> returns B:0");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.tx_subdev_spec = "B:0";

    std::string result = radio.get_effective_tx_subdev_spec();

    if (result == "B:0")
        PASS();
    else
        FAIL("Expected 'B:0', got: '" + result + "'");
}

// =============================================================================
// Test: Non-B200 + empty rx_subdev_spec → returns "" (no defaulting)
// =============================================================================
void test_non_b200_empty_rx_subdev()
{
    TEST("Non-B200 + empty rx_subdev_spec -> returns empty");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "n310";
    radio.rx_subdev_spec = "";

    std::string result = radio.get_effective_rx_subdev_spec();

    if (result.empty())
        PASS();
    else
        FAIL("Expected empty string, got: '" + result + "'");
}

// =============================================================================
// Test: Non-B200 + empty tx_subdev_spec → returns "" (no defaulting)
// =============================================================================
void test_non_b200_empty_tx_subdev()
{
    TEST("Non-B200 + empty tx_subdev_spec -> returns empty");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "n310";
    radio.tx_subdev_spec = "";

    std::string result = radio.get_effective_tx_subdev_spec();

    if (result.empty())
        PASS();
    else
        FAIL("Expected empty string, got: '" + result + "'");
}

// =============================================================================
// Test: Non-B200 + user-provided rx_subdev_spec "A:0" → returns "A:0"
// =============================================================================
void test_non_b200_user_rx_subdev()
{
    TEST("Non-B200 + user-provided rx_subdev_spec A:0 -> returns A:0");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "n310";
    radio.rx_subdev_spec = "A:0";

    std::string result = radio.get_effective_rx_subdev_spec();

    if (result == "A:0")
        PASS();
    else
        FAIL("Expected 'A:0', got: '" + result + "'");
}

// =============================================================================
// Test: Non-B200 + user-provided tx_subdev_spec "A:0" → returns "A:0"
// =============================================================================
void test_non_b200_user_tx_subdev()
{
    TEST("Non-B200 + user-provided tx_subdev_spec A:0 -> returns A:0");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "n310";
    radio.tx_subdev_spec = "A:0";

    std::string result = radio.get_effective_tx_subdev_spec();

    if (result == "A:0")
        PASS();
    else
        FAIL("Expected 'A:0', got: '" + result + "'");
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "=== Subdevice Spec Defaulting Tests ===" << std::endl;
    std::cout << std::endl;

    test_b200_empty_rx_subdev();
    test_b200_empty_tx_subdev();
    test_b200_user_rx_subdev();
    test_b200_user_tx_subdev();
    test_non_b200_empty_rx_subdev();
    test_non_b200_empty_tx_subdev();
    test_non_b200_user_rx_subdev();
    test_non_b200_user_tx_subdev();

    std::cout << std::endl;
    std::cout << "=== Results: " << tests_passed << "/" << tests_run << " tests passed ===" << std::endl;

    if (tests_passed != tests_run)
    {
        std::cerr << "FAILURE: " << (tests_run - tests_passed) << " test(s) failed!" << std::endl;
        return 1;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
