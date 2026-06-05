#include <iostream>
#include <string>
#include <cassert>

#include "Factory/Module/Radio/Radio.hpp"

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

void test_no_serial_no_clk()
{
    TEST("no serial, no clk_rate -> type=b200 only");
    aff3ct::factory::Radio radio;
    radio.serial = "";
    radio.clk_rate = 0;
    std::string result = radio.build_device_string();
    if (result == "type=b200") PASS();
    else FAIL("Expected 'type=b200', got: " + result);
}

void test_with_serial()
{
    TEST("serial ABC123 -> type=b200,serial=ABC123");
    aff3ct::factory::Radio radio;
    radio.serial = "ABC123";
    radio.clk_rate = 0;
    std::string result = radio.build_device_string();
    if (result == "type=b200,serial=ABC123") PASS();
    else FAIL("Expected 'type=b200,serial=ABC123', got: " + result);
}

void test_with_clk_rate()
{
    TEST("clk_rate 30.72e6 -> contains master_clock_rate=");
    aff3ct::factory::Radio radio;
    radio.serial = "";
    radio.clk_rate = 30.72e6;
    std::string result = radio.build_device_string();
    bool pass = result.find("type=b200") != std::string::npos &&
                result.find("master_clock_rate=") != std::string::npos;
    if (pass) PASS();
    else FAIL("Got: " + result);
}

void test_with_serial_and_clk()
{
    TEST("serial + clk_rate -> both present");
    aff3ct::factory::Radio radio;
    radio.serial = "XYZ";
    radio.clk_rate = 15.36e6;
    std::string result = radio.build_device_string();
    bool pass = result.find("type=b200") != std::string::npos &&
                result.find("serial=XYZ") != std::string::npos &&
                result.find("master_clock_rate=") != std::string::npos;
    if (pass) PASS();
    else FAIL("Got: " + result);
}

void test_no_addr_field()
{
    TEST("device string never contains addr=");
    aff3ct::factory::Radio radio;
    radio.serial = "12345";
    radio.clk_rate = 30.72e6;
    std::string result = radio.build_device_string();
    if (result.find("addr=") == std::string::npos) PASS();
    else FAIL("Should not contain 'addr=', got: " + result);
}

int main()
{
    std::cout << "=== Device String Construction Tests (B200-only) ===" << std::endl;
    std::cout << std::endl;

    test_no_serial_no_clk();
    test_with_serial();
    test_with_clk_rate();
    test_with_serial_and_clk();
    test_no_addr_field();

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
