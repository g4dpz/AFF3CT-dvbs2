#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <stdexcept>

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

// Helper: expect that calling validate() throws an exception
static bool expect_throws(const aff3ct::factory::Radio& radio, const std::string& expected_substr = "")
{
    try
    {
        radio.validate();
        return false; // Should have thrown
    }
    catch (const std::exception& e)
    {
        if (!expected_substr.empty())
        {
            std::string what(e.what());
            if (what.find(expected_substr) == std::string::npos)
            {
                std::cerr << "    Exception thrown but message does not contain \""
                          << expected_substr << "\". Got: " << what << std::endl;
                return false;
            }
        }
        return true;
    }
}

// Helper: expect that calling validate() does NOT throw
static bool expect_no_throw(const aff3ct::factory::Radio& radio)
{
    try
    {
        radio.validate();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "    Unexpected exception: " << e.what() << std::endl;
        return false;
    }
}

// =============================================================================
// Test: clk_rate > 61.44e6 throws for B200
// =============================================================================
void test_clk_rate_exceeds_max_throws()
{
    TEST("clk_rate > 61.44e6 throws for B200");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.clk_rate = 70e6; // exceeds 61.44 MHz

    if (expect_throws(radio, "61.44"))
        PASS();
    else
        FAIL("Expected exception for clk_rate > 61.44e6");
}

// =============================================================================
// Test: clk_rate <= 61.44e6 does NOT throw for B200
// =============================================================================
void test_clk_rate_within_limit_passes()
{
    TEST("clk_rate <= 61.44e6 passes for B200");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.clk_rate = 61.44e6; // exactly at limit

    if (expect_no_throw(radio))
        PASS();
    else
        FAIL("Should not throw for clk_rate == 61.44e6");
}

// =============================================================================
// Test: clk_rate == 0 does NOT throw for B200 (omit from device string)
// =============================================================================
void test_clk_rate_zero_passes()
{
    TEST("clk_rate == 0 passes for B200 (default/omit)");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.clk_rate = 0;

    if (expect_no_throw(radio))
        PASS();
    else
        FAIL("Should not throw for clk_rate == 0");
}

// =============================================================================
// Test: rx_rate > 56e6 throws for B200
// =============================================================================
void test_rx_rate_exceeds_max_throws()
{
    TEST("rx_rate > 56e6 throws for B200");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.rx_rate = 60e6; // exceeds 56 MHz

    if (expect_throws(radio, "56"))
        PASS();
    else
        FAIL("Expected exception for rx_rate > 56e6");
}

// =============================================================================
// Test: tx_rate > 56e6 throws for B200
// =============================================================================
void test_tx_rate_exceeds_max_throws()
{
    TEST("tx_rate > 56e6 throws for B200");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.tx_rate = 60e6; // exceeds 56 MHz

    if (expect_throws(radio, "56"))
        PASS();
    else
        FAIL("Expected exception for tx_rate > 56e6");
}

// =============================================================================
// Test: clock_source "external" throws for B200
// =============================================================================
void test_external_clock_source_throws()
{
    TEST("clock_source \"external\" throws for B200");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.clock_source = "external";

    if (expect_throws(radio, "external"))
        PASS();
    else
        FAIL("Expected exception for clock_source == external");
}

// =============================================================================
// Test: non-B200 type skips all validation (no throws regardless of values)
// =============================================================================
void test_non_b200_skips_validation()
{
    TEST("non-B200 type skips all validation");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "n310"; // not B200
    radio.clk_rate = 200e6;   // would fail for B200
    radio.rx_rate = 100e6;    // would fail for B200
    radio.tx_rate = 100e6;    // would fail for B200
    radio.clock_source = "external"; // would fail for B200

    if (expect_no_throw(radio))
        PASS();
    else
        FAIL("Non-B200 type should skip all validation");
}

// =============================================================================
// Test: empty usrp_type skips validation
// =============================================================================
void test_empty_type_skips_validation()
{
    TEST("empty usrp_type skips validation");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "";
    radio.clk_rate = 200e6;
    radio.rx_rate = 100e6;
    radio.tx_rate = 100e6;
    radio.clock_source = "external";

    if (expect_no_throw(radio))
        PASS();
    else
        FAIL("Empty usrp_type should skip all validation");
}

// =============================================================================
// Test: default values from Task 1
// =============================================================================
void test_default_rx_pin_core()
{
    TEST("default rx_pin_core == 1");
    aff3ct::factory::Radio radio;
    if (radio.rx_pin_core == 1)
        PASS();
    else
        FAIL("Expected rx_pin_core == 1, got " + std::to_string(radio.rx_pin_core));
}

void test_default_tx_pin_core()
{
    TEST("default tx_pin_core == 3");
    aff3ct::factory::Radio radio;
    if (radio.tx_pin_core == 3)
        PASS();
    else
        FAIL("Expected tx_pin_core == 3, got " + std::to_string(radio.tx_pin_core));
}

void test_default_clock_source()
{
    TEST("default clock_source == \"internal\"");
    aff3ct::factory::Radio radio;
    if (radio.clock_source == "internal")
        PASS();
    else
        FAIL("Expected clock_source == \"internal\", got " + radio.clock_source);
}

// =============================================================================
// Test: valid B200 configuration passes (combined rates within limit)
// =============================================================================
void test_valid_b200_config_passes()
{
    TEST("valid B200 config with rates within limits passes");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.clk_rate = 30.72e6;
    radio.rx_rate = 15.36e6;
    radio.tx_rate = 15.36e6;
    radio.clock_source = "internal";
    radio.rx_antenna = "RX2";
    radio.tx_antenna = "TX/RX";

    if (expect_no_throw(radio))
        PASS();
    else
        FAIL("Valid B200 config should not throw");
}

// =============================================================================
// Test: clock_source "gpsdo" is accepted for B200
// =============================================================================
void test_gpsdo_clock_source_passes()
{
    TEST("clock_source \"gpsdo\" passes for B200");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.clock_source = "gpsdo";

    if (expect_no_throw(radio))
        PASS();
    else
        FAIL("gpsdo clock source should be accepted for B200");
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "=== B200-mini Radio Parameter Validation Tests ===" << std::endl;
    std::cout << std::endl;

    std::cout << "--- Validation Error Tests ---" << std::endl;
    test_clk_rate_exceeds_max_throws();
    test_clk_rate_within_limit_passes();
    test_clk_rate_zero_passes();
    test_rx_rate_exceeds_max_throws();
    test_tx_rate_exceeds_max_throws();
    test_external_clock_source_throws();
    test_non_b200_skips_validation();
    test_empty_type_skips_validation();
    test_valid_b200_config_passes();
    test_gpsdo_clock_source_passes();

    std::cout << std::endl;
    std::cout << "--- Default Value Tests ---" << std::endl;
    test_default_rx_pin_core();
    test_default_tx_pin_core();
    test_default_clock_source();

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
