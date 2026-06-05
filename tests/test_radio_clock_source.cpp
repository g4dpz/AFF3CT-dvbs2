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
// Test: clock_source "gpsdo" → should_set_time_source_gpsdo() returns true
// =============================================================================
void test_gpsdo_returns_true()
{
    TEST("clock_source \"gpsdo\" -> should_set_time_source_gpsdo() returns true");
    aff3ct::factory::Radio radio;
    radio.clock_source = "gpsdo";

    if (radio.should_set_time_source_gpsdo())
        PASS();
    else
        FAIL("Expected should_set_time_source_gpsdo() to return true for \"gpsdo\"");
}

// =============================================================================
// Test: clock_source "internal" → should_set_time_source_gpsdo() returns false
// =============================================================================
void test_internal_returns_false()
{
    TEST("clock_source \"internal\" -> should_set_time_source_gpsdo() returns false");
    aff3ct::factory::Radio radio;
    radio.clock_source = "internal";

    if (!radio.should_set_time_source_gpsdo())
        PASS();
    else
        FAIL("Expected should_set_time_source_gpsdo() to return false for \"internal\"");
}

// =============================================================================
// Test: clock_source "external" → should_set_time_source_gpsdo() returns false
// =============================================================================
void test_external_returns_false()
{
    TEST("clock_source \"external\" -> should_set_time_source_gpsdo() returns false");
    aff3ct::factory::Radio radio;
    radio.clock_source = "external";

    if (!radio.should_set_time_source_gpsdo())
        PASS();
    else
        FAIL("Expected should_set_time_source_gpsdo() to return false for \"external\"");
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "=== Clock Source Configuration Tests ===" << std::endl;
    std::cout << std::endl;

    test_gpsdo_returns_true();
    test_internal_returns_false();
    test_external_returns_false();

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
