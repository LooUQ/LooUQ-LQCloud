
#include "lqc_internal.h"


#define MILLIS() millis()

/**
 *	\brief Initialize a workSchedule object (struct) to track periodic events at regular intervals.
 * 
 *  \param [in] intervalMillis - Interval period in milliseconds.
 */
wrkTime_t wrkTime_create(unsigned long intervalMillis)
{
    wrkTime_t schedObj;
//    schedObj.schedType = wrkTimeType_periodic;
    schedObj.enabled = true;
    schedObj.userState = 0;
    schedObj.period = intervalMillis;
    // schedObj.beginAtMillis = MILLIS();
    schedObj.lastAtMillis = MILLIS();
    return schedObj;
}


// /**
//  *	\brief Initialize a workSchedule object (struct) to track one time duration expired.
//  * 
//  *  \param [in] intervalMillis - Interval period in milliseconds.
//  */
// wrkTime_t wrkTime_createTimer(unsigned long intervalMillis)
// {
//     wrkTime_t schedObj;
//     schedObj.schedType = wrkTimeType_timer;
//     schedObj.enabled = true;
//     schedObj.period = intervalMillis;
//     schedObj.beginAtMillis = MILLIS();
//     schedObj.lastAtMillis = schedObj.beginAtMillis;
//     return schedObj;
// }


/**
 *	\brief Reset a workSchedule object's internal timekeeping to now. If invoked mid-interval, sets the next event to a full-interval duration.
 * 
 *  \param [in] schedObj - workSchedule object (struct) to reset.
 */
void wrkTime_start(wrkTime_t *schedObj)
{
    schedObj->enabled = true;
    schedObj->lastAtMillis = MILLIS();
}


/**
 *	\brief Reset a workSchedule object's internal timekeeping to now. If invoked mid-interval, sets the next event to a full-interval duration.
 * 
 *  \param [in] schedObj - workSchedule object (struct) to reset.
 */
void wrkTime_stop(wrkTime_t *schedObj)
{
    schedObj->enabled = false;
    schedObj->lastAtMillis = 0;
}


/**
 *	\brief Query wrkTime object to see if running.
 * 
 *  \param [in] schedObj - workSchedule object (struct) to reset.
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
 *  \param [in] schedObj - workSchedule object (struct) to test.
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
 *  \param [in] startTime - A point in time (measured by millis)
 *  \param [in] reqdDuration - A target elapsed time in milliseconds
 * 
 *  \returns true if the reqdDuration has been reached
 */
bool wrkTime_isElapsed(millisTime_t startTime, millisDuration_t reqdDuration)
{
    return millis() - startTime > reqdDuration;
}

