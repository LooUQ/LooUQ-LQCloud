/******************************************************************************
 *  \file lqc-wrkTime.c
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2020, 2021 LooUQ Incorporated.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 * LooUQ LQCloud Client Work-Time Services
 *****************************************************************************/

#include <jlinkRtt.h>                   // if you have issues with Intellisense on PRINTF() or DBGCOLOR_ try temporarily placing jlinkRtt.h outside conditional
                                        // or add "_DEBUG = 2" to c_cpp_properties.json "defines" section list

#define _DEBUG 2                        // set to non-zero value for PRINTF debugging output, 
// debugging output options             // LTEm1c will satisfy PRINTF references with empty definition if not already resolved
#if defined(_DEBUG) && _DEBUG > 0
    asm(".global _printf_float");       // forces build to link in float support for printf
    #if _DEBUG == 1
    #define SERIAL_DBG 1                // enable serial port output using devl host platform serial, 1=wait for port
    #elif _DEBUG == 2
    #include <jlinkRtt.h>               // output debug PRINTF macros to J-Link RTT channel
    #endif
#else
#define PRINTF(c_, f_, ...) ;
#endif


#include <lqcloud.h>
#include "lqc-internal.h"


#define MILLIS() millis()

/**
 *	\brief Initialize a workSchedule object (struct) to track periodic events at regular intervals.
 * 
 *  \param intervalMillis [in] - Interval period in milliseconds.
 */
wrkTime_t wrkTime_create(unsigned long intervalMillis)
{
    wrkTime_t schedObj;
    schedObj.enabled = true;
    schedObj.period = intervalMillis;
    schedObj.lastAtMillis = MILLIS();
    return schedObj;
}


/**
 *	\brief Reset a workSchedule object's internal timekeeping to now. If invoked mid-interval, sets the next event to a full-interval duration.
 * 
 *  \param schedObj [in] - workSchedule object (struct) to reset.
 */
void wrkTime_start(wrkTime_t *schedObj)
{
    schedObj->enabled = true;
    schedObj->lastAtMillis = MILLIS();
}


/**
 *	\brief Reset a workSchedule object's internal timekeeping to now. If invoked mid-interval, sets the next event to a full-interval duration.
 * 
 *  \param schedObj [in] - workSchedule object (struct) to reset.
 */
void wrkTime_stop(wrkTime_t *schedObj)
{
    schedObj->enabled = false;
    schedObj->lastAtMillis = 0;
}


/**
 *	\brief Query wrkTime object to see if running.
 * 
 *  \param schedObj [in] - workSchedule object (struct) to reset.
 * 
 *  \return true if timer running
 */
bool wrkTime_isRunning(wrkTime_t *schedObj)
{
    return schedObj->enabled;
}


/**
 *	\brief Tests if the internal timing for a workSchedule object has completed and it is time to do the work.
 * 
 *  \param schedObj [in] - workSchedule object (struct) to test.
 */
bool wrkTime_doNow(wrkTime_t *schedObj)
{
    if (schedObj->enabled)
    {
        unsigned long now = MILLIS();
        if (now - schedObj->lastAtMillis > schedObj->period)
        {
            // schedObj->enabled = (schedObj->schedType != wrkTimeType_timer);
            schedObj->lastAtMillis = now;
            return true;
        }
    }
    return false;
}


/**
 *	\brief Simple helper for testing a start time to see if a duration has been satisfied.
 * 
 *  \param startTime [in] - A point in time (measured by millis)
 *  \param reqdDuration [in] - A target elapsed time in milliseconds
 * 
 *  \returns true if the reqdDuration has been reached, returns false if the startTime has not been set (=0)
 */
bool wrkTime_isElapsed(millisTime_t startTime, millisDuration_t reqdDuration)
{
    return (startTime) ? millis() - startTime > reqdDuration : false;
}

