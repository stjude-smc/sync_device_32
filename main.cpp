#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

#define UNIT_TEST
#include "sync_device_32/src/globals.h"

// Simple test group
TEST_GROUP(TimeConversions) {};
	
#define TIME_CONVERSION_VALUES {0, 1, 2, 3, 4, 5, 10, 13, 41, 100, 500, 883, 2731, 7879, 1000000, 84000000}

// A simple test
TEST(TimeConversions, cts2us2cts)
{
	uint32_t counts[] = TIME_CONVERSION_VALUES;
	for (int i=0; i<sizeof(counts)/sizeof(int); i++)
	{
		CHECK_COMPARE(counts[i] - us2cts(cts2us(counts[i])), <=, 84/SYS_TC_PRESCALER + 1);
	}
}

TEST(TimeConversions, us2cts2us)
{
	uint32_t us[] = TIME_CONVERSION_VALUES;
	for (int i=0; i<sizeof(us)/sizeof(int); i++)
	{
		CHECK_COMPARE(us[i] - cts2us(us2cts(us[i])), <=, SYS_TC_PRESCALER/84 + 1);
	}
}

TEST(TimeConversions, prescaler)
{
	CHECK_EQUAL(8400000 / SYS_TC_PRESCALER, SYS_TC_CONVERSION_MULTIPLIER);
}

int main(int argc, char** argv)
{
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
