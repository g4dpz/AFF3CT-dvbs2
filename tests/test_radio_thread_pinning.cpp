#include <iostream>
#include <string>
#include <thread>

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
// Test: pin_core 0 is valid (always at least 1 core)
// =============================================================================
void test_pin_core_zero_valid()
{
    TEST("pin_core 0 is valid (at least 1 core available)");
    aff3ct::factory::Radio radio;
    if (radio.is_pin_core_valid(0))
        PASS();
    else
        FAIL("pin_core 0 should be valid on any machine");
}

// =============================================================================
// Test: pin_core 1 is valid on multi-core machines
// =============================================================================
void test_pin_core_one_valid()
{
    TEST("pin_core 1 is valid on multi-core machine");
    unsigned int n_cores = std::thread::hardware_concurrency();
    aff3ct::factory::Radio radio;
    if (n_cores >= 2)
    {
        if (radio.is_pin_core_valid(1))
            PASS();
        else
            FAIL("pin_core 1 should be valid on a multi-core machine");
    }
    else
    {
        // On a single-core machine, pin_core 1 would be invalid
        if (!radio.is_pin_core_valid(1))
            PASS();
        else
            FAIL("pin_core 1 should be invalid on a single-core machine");
    }
}

// =============================================================================
// Test: pin_core 99999 is invalid (exceeds available cores)
// =============================================================================
void test_pin_core_large_invalid()
{
    TEST("pin_core 99999 is invalid (exceeds available cores)");
    aff3ct::factory::Radio radio;
    if (!radio.is_pin_core_valid(99999))
        PASS();
    else
        FAIL("pin_core 99999 should exceed available cores");
}

// =============================================================================
// Test: pin_core -1 is invalid
// =============================================================================
void test_pin_core_negative_invalid()
{
    TEST("pin_core -1 is invalid");
    aff3ct::factory::Radio radio;
    if (!radio.is_pin_core_valid(-1))
        PASS();
    else
        FAIL("pin_core -1 should be invalid");
}

// =============================================================================
// Test: default rx_pin_core (1) is valid on typical machines
// =============================================================================
void test_default_rx_pin_core_valid()
{
    TEST("default rx_pin_core (1) is valid on typical machines");
    unsigned int n_cores = std::thread::hardware_concurrency();
    aff3ct::factory::Radio radio;
    if (n_cores >= 2)
    {
        if (radio.is_pin_core_valid(radio.rx_pin_core))
            PASS();
        else
            FAIL("default rx_pin_core should be valid on multi-core machine");
    }
    else
    {
        // Conditionally pass: on single-core, default 1 would be invalid
        std::cout << "SKIPPED (single-core machine)" << std::endl;
        tests_passed++;
    }
}

// =============================================================================
// Test: default tx_pin_core (3) is valid on 4+ core machines
// =============================================================================
void test_default_tx_pin_core_valid()
{
    TEST("default tx_pin_core (3) is valid on 4+ core machines");
    unsigned int n_cores = std::thread::hardware_concurrency();
    aff3ct::factory::Radio radio;
    if (n_cores >= 4)
    {
        if (radio.is_pin_core_valid(radio.tx_pin_core))
            PASS();
        else
            FAIL("default tx_pin_core should be valid on 4+ core machine");
    }
    else
    {
        // Conditionally pass: on <4 core machine, default 3 would be invalid
        std::cout << "SKIPPED (fewer than 4 cores)" << std::endl;
        tests_passed++;
    }
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "=== Radio Thread Pinning Tests ===" << std::endl;
    std::cout << std::endl;

    unsigned int n_cores = std::thread::hardware_concurrency();
    std::cout << "System reports " << n_cores << " hardware cores." << std::endl;
    std::cout << std::endl;

    test_pin_core_zero_valid();
    test_pin_core_one_valid();
    test_pin_core_large_invalid();
    test_pin_core_negative_invalid();
    test_default_rx_pin_core_valid();
    test_default_tx_pin_core_valid();

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
