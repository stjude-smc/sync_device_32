#define UNIT_TEST

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

#include "sync_device_32/src/globals.h"
#include "sync_device_32/src/events.h"
#include "sync_device_32/src/uart_comm.h"

/************************************************************************/
/*                    BOILERPLATE                                       */
/************************************************************************/
// from events.h// Process the event metadatastatic inline bool _update_event(Event *event)
{
	if (event->interv_cts >= MIN_EVENT_INTERVAL) // repeating event	{		event->ts_cts += event->interv_cts;		if (event->N == 0){  // infinite event - reschedule			return true;		}		// if (N == 1), it was a last call, and we drop it		if (event->N > 1) {  // reschedule the event			event->N--;			return true;		}	}	return false;
}


/************************************************************************/
/*                 TEST TIME CONVERSION FUNCTIONS                       */
/************************************************************************/

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



/************************************************************************/
/*                 TEST EVENT PROCESSING FUNCTIONS                      */
/************************************************************************/

TEST_GROUP(EventProcessing) {};
	
// Run event once because N==1
TEST(EventProcessing, check_once_N)
{
	Event e;
	e.N = 1;
	e.interv_cts = 100;
	CHECK_FALSE(_update_event(&e));
}

// Run event once because the interval is too short. N > 1
TEST(EventProcessing, check_once_interv1)
{
	Event e;
	e.N = 151;
	e.interv_cts = 0;
	CHECK_FALSE(_update_event(&e));
	e.interv_cts = MIN_EVENT_INTERVAL - 1;
	CHECK_FALSE(_update_event(&e));
}

// Run event once because the interval is too short. N == 0
TEST(EventProcessing, check_once_interv2)
{
	Event e;
	e.N = 0;
	e.interv_cts = 0;
	CHECK_FALSE(_update_event(&e));
	e.interv_cts = MIN_EVENT_INTERVAL - 1;
	CHECK_FALSE(_update_event(&e));
}

// Reschedule the event because N > 1 and interval given
TEST(EventProcessing, check_reschedule)
{
	Event e0;
	e0.ts_cts = 712;
	e0.N = 15;
	e0.interv_cts = 10*MIN_EVENT_INTERVAL;
	
	Event e1 = e0;
	CHECK_TRUE(_update_event(&e1));
	CHECK_EQUAL(e1.ts_cts, e0.ts_cts + e0.interv_cts);
	CHECK_EQUAL(e1.N, e0.N - 1);
}

// Reschedule the event because N == 0 and interval given
TEST(EventProcessing, check_infinite)
{
	Event e0;
	e0.ts_cts = 13;
	e0.N = 0;
	e0.interv_cts = MIN_EVENT_INTERVAL;
	
	Event e1 = e0;
	CHECK_TRUE(_update_event(&e1));
	CHECK_EQUAL(e1.ts_cts, e0.ts_cts + e0.interv_cts);
	CHECK_EQUAL(e1.N, 0);
}



/************************************************************************/
/*                        ENTRY POINT                                   */
/************************************************************************/

int main(int argc, char** argv)
{
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
