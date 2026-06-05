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

void test_empty_rx_subdev_defaults_aa()
{
    TEST("empty rx_subdev_spec -> returns A:A");
    aff3ct::factory::Radio radio;
    radio.rx_subdev_spec = "";
    if (radio.get_effective_rx_subdev_spec() == "A:A") PASS();
    else FAIL("Expected 'A:A'");
}

void test_empty_tx_subdev_defaults_aa()
{
    TEST("empty tx_subdev_spec -> returns A:A");
    aff3ct::factory::Radio radio;
    radio.tx_subdev_spec = "";
    if (radio.get_effective_tx_subdev_spec() == "A:A") PASS();
    else FAIL("Expected 'A:A'");
}

void test_user_rx_subdev_preserved()
{
    TEST("user-provided rx_subdev_spec preserved");
    aff3ct::factory::Radio radio;
    radio.rx_subdev_spec = "B:0";
    if (radio.get_effective_rx_subdev_spec() == "B:0") PASS();
    else FAIL("Expected 'B:0'");
}

void test_user_tx_subdev_preserved()
{
    TEST("user-provided tx_subdev_spec preserved");
    aff3ct::factory::Radio radio;
    radio.tx_subdev_spec = "B:0";
    if (radio.get_effective_tx_subdev_spec() == "B:0") PASS();
    else FAIL("Expected 'B:0'");
}

int main()
{
    std::cout << "=== Subdevice Spec Defaulting Tests (B200-only) ===" << std::endl;
    std::cout << std::endl;

    test_empty_rx_subdev_defaults_aa();
    test_empty_tx_subdev_defaults_aa();
    test_user_rx_subdev_preserved();
    test_user_tx_subdev_preserved();

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
