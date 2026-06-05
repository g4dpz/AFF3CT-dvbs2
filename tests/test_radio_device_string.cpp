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
// Test: B200 + empty addr → "type=b200" only (no addr=, no serial=)
// =============================================================================
void test_b200_empty_addr()
{
    TEST("B200 + empty addr -> type=b200 only");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.usrp_addr = "";
    radio.clk_rate = 0;

    std::string result = radio.build_device_string();

    bool pass = true;
    if (result.find("type=b200") == std::string::npos)
    {
        FAIL("Expected 'type=b200' in device string, got: " + result);
        pass = false;
    }
    else if (result.find("addr=") != std::string::npos)
    {
        FAIL("Should NOT contain 'addr=' in device string, got: " + result);
        pass = false;
    }
    else if (result.find("serial=") != std::string::npos)
    {
        FAIL("Should NOT contain 'serial=' in device string, got: " + result);
        pass = false;
    }
    else if (result != "type=b200")
    {
        FAIL("Expected exactly 'type=b200', got: " + result);
        pass = false;
    }

    if (pass) PASS();
}

// =============================================================================
// Test: B200 + addr "ABC123" → contains "type=b200" and "serial=ABC123", no "addr="
// =============================================================================
void test_b200_with_serial()
{
    TEST("B200 + addr ABC123 -> serial=ABC123, no addr=");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.usrp_addr = "ABC123";
    radio.clk_rate = 0;

    std::string result = radio.build_device_string();

    bool pass = true;
    if (result.find("type=b200") == std::string::npos)
    {
        FAIL("Expected 'type=b200' in device string, got: " + result);
        pass = false;
    }
    else if (result.find("serial=ABC123") == std::string::npos)
    {
        FAIL("Expected 'serial=ABC123' in device string, got: " + result);
        pass = false;
    }
    else if (result.find("addr=") != std::string::npos)
    {
        FAIL("Should NOT contain 'addr=' in device string, got: " + result);
        pass = false;
    }

    if (pass) PASS();
}

// =============================================================================
// Test: Non-B200 (n310) + addr "192.168.20.2" → contains "addr=192.168.20.2", no "serial="
// =============================================================================
void test_non_b200_with_addr()
{
    TEST("Non-B200 (n310) + addr 192.168.20.2 -> addr=192.168.20.2, no serial=");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "n310";
    radio.usrp_addr = "192.168.20.2";
    radio.clk_rate = 0;

    std::string result = radio.build_device_string();

    bool pass = true;
    if (result.find("addr=192.168.20.2") == std::string::npos)
    {
        FAIL("Expected 'addr=192.168.20.2' in device string, got: " + result);
        pass = false;
    }
    else if (result.find("serial=") != std::string::npos)
    {
        FAIL("Should NOT contain 'serial=' in device string, got: " + result);
        pass = false;
    }

    if (pass) PASS();
}

// =============================================================================
// Test: B200 + clk_rate 30.72e6 → contains "master_clock_rate="
// =============================================================================
void test_b200_with_clk_rate()
{
    TEST("B200 + clk_rate 30.72e6 -> contains master_clock_rate=");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.usrp_addr = "";
    radio.clk_rate = 30.72e6;

    std::string result = radio.build_device_string();

    bool pass = true;
    if (result.find("master_clock_rate=") == std::string::npos)
    {
        FAIL("Expected 'master_clock_rate=' in device string, got: " + result);
        pass = false;
    }
    else if (result.find("type=b200") == std::string::npos)
    {
        FAIL("Expected 'type=b200' in device string, got: " + result);
        pass = false;
    }

    if (pass) PASS();
}

// =============================================================================
// Test: B200 + clk_rate 0 → does NOT contain "master_clock_rate"
// =============================================================================
void test_b200_clk_rate_zero()
{
    TEST("B200 + clk_rate 0 -> no master_clock_rate");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "b200";
    radio.usrp_addr = "";
    radio.clk_rate = 0;

    std::string result = radio.build_device_string();

    bool pass = true;
    if (result.find("master_clock_rate") != std::string::npos)
    {
        FAIL("Should NOT contain 'master_clock_rate' when clk_rate is 0, got: " + result);
        pass = false;
    }

    if (pass) PASS();
}

// =============================================================================
// Test: Empty type + empty addr + clk_rate 0 → empty string
// =============================================================================
void test_all_empty()
{
    TEST("Empty type + empty addr + clk_rate 0 -> empty string");
    aff3ct::factory::Radio radio;
    radio.usrp_type = "";
    radio.usrp_addr = "";
    radio.clk_rate = 0;

    std::string result = radio.build_device_string();

    if (result.empty())
        PASS();
    else
        FAIL("Expected empty string, got: " + result);
}

// =============================================================================
// main
// =============================================================================
int main()
{
    std::cout << "=== Device String Construction Tests ===" << std::endl;
    std::cout << std::endl;

    test_b200_empty_addr();
    test_b200_with_serial();
    test_non_b200_with_addr();
    test_b200_with_clk_rate();
    test_b200_clk_rate_zero();
    test_all_empty();

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
