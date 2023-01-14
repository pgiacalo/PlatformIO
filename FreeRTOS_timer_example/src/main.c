/*

From what I understand, the minimum time between callbacks with the FreeRTOS timer
is 1 millisecond. Certainly NOT enough resolution to make this worthwhile, since it 
is very difficult to make this program work. So far, impossible! 
---------
This code compiles.
HOWEVER, when loaded onto my ESP32-WROOM-32D, the core crashes at runtime with the following error message:

␛[0;32mI (0) cpu_start: Starting scheduler on APP CPU.␛[0m

assert failed: prvInitialiseNewTimer timers.c:369 (( xTimerPeriodInTicks > 0 ))

Backtrace: 0x400818ea:0x3ffb5880 0x400852c1:0x3ffb58a0 0x4008ab59:0x3ffb58c0 0x40087ffa:0x3ffb59e0 0x400880a1:0x3ffb5a00 0x400d0ba5:0x3ffb5a20 0x400e4757:0x3ffb5a40 0x400884d5:0x3ffb5a60
---------
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define NUM_TIMERS 2

// An array to hold handles to the created timers.
TimerHandle_t xTimers[ NUM_TIMERS ];

// An array to hold a count of the number of times each timer expires.
int32_t lExpireCounters[ NUM_TIMERS ] = { 0 };

// Define a callback function that will be used by multiple timer instances.
// The callback function does nothing but count the number of times the
// associated timer expires, and stop the timer once the timer has expired
// 10 times.
void vTimerCallback( TimerHandle_t pxTimer )
{
	int32_t lArrayIndex;
	const int32_t xMaxExpiryCountBeforeStopping = 10;

    // Optionally do something if the pxTimer parameter is NULL.
    configASSERT( pxTimer );

    // Which timer expired?
    lArrayIndex = ( int32_t ) pvTimerGetTimerID( pxTimer );

    // Increment the number of times that pxTimer has expired.
    lExpireCounters[ lArrayIndex ] += 1;

	printf("Counter=%d\n", lExpireCounters[ lArrayIndex ]);

    // If the timer has expired 10 times then stop it from running.
    if( lExpireCounters[ lArrayIndex ] == xMaxExpiryCountBeforeStopping )
    {
        // Do not use a block time if calling a timer API function from a
        // timer callback function, as doing so could cause a deadlock!
        xTimerStop( pxTimer, 0 );
    }
}

void app_main()
{
int32_t x;

    // Create then start some timers.  Starting the timers before the scheduler
    // has been started means the timers will start running immediately that
    // the scheduler starts.
    for( x = 0; x < NUM_TIMERS; x++ )
    {
        xTimers[ x ] = xTimerCreate(    "Timer",       // Just a text name, not used by the kernel.
                                        ( 5 * x ),   // The timer period in ticks.
                                        pdTRUE,        // The timers will auto-reload themselves when they expire.
                                        ( void * ) x,  // Assign each timer a unique id equal to its array index.
                                        vTimerCallback // Each timer calls the same callback when it expires.
                                    );

        if( xTimers[ x ] == NULL )
        {
            // The timer was not created.
        }
        else
        {
            // Start the timer.  No block time is specified, and even if one was
            // it would be ignored because the scheduler has not yet been
            // started.
            if( xTimerStart( xTimers[ x ], 0 ) != pdPASS )
            {
                // The timer could not be set into the Active state.
            }
        }
    }

    // ...
    // Create tasks here.
    // ...

    // Starting the scheduler will start the timers running as they have already
    // been set into the active state.
    vTaskStartScheduler();

    // Should not reach here.
    for( ;; ){
		//do nothing
	}

	return;
}