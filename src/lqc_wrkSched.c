
#include "lqc_internal.h"


#define MILLIS() millis()

/**
 *	\brief Initialize a workSchedule object (struct) to track periodic events at regular intervals.
 * 
 *  \param [in] intervalMillis - Interval period in milliseconds.
 */
workSchedule_t workSched_createPeriodic(unsigned long intervalMillis)
{
    workSchedule_t schedObj;
    schedObj.schedType = workScheduleType_periodic;
    schedObj.enabled = true;
    schedObj.userState = 0;
    schedObj.period = intervalMillis;
    schedObj.beginAtMillis = MILLIS();
    schedObj.lastAtMillis = schedObj.beginAtMillis;
    return schedObj;
}


/**
 *	\brief Initialize a workSchedule object (struct) to track one time duration expired.
 * 
 *  \param [in] intervalMillis - Interval period in milliseconds.
 */
workSchedule_t workSched_createTimer(unsigned long intervalMillis)
{
    workSchedule_t schedObj;
    schedObj.schedType = workScheduleType_timer;
    schedObj.enabled = true;
    schedObj.period = intervalMillis;
    schedObj.beginAtMillis = MILLIS();
    schedObj.lastAtMillis = schedObj.beginAtMillis;
    return schedObj;
}


/**
 *	\brief Reset a workSchedule object's internal timekeeping to now. If invoked mid-interval, sets the next event to a full-interval duration.
 * 
 *  \param [in] schedObj - workSchedule object (struct) to reset.
 */
void workSched_reset(workSchedule_t *schedObj)
{
    schedObj->enabled = true;
    schedObj->lastAtMillis = MILLIS();
}


/**
 *	\brief Tests if the internal timing for a workSchedule object has completed and it is time to do the work.
 * 
 *  \param [in] schedObj - workSchedule object (struct) to test.
 */
bool workSched_doNow(workSchedule_t *schedObj)
{
    if (schedObj->enabled)
    {
        unsigned long now = MILLIS();
        if (now - schedObj->lastAtMillis > schedObj->period)
        {
            schedObj->enabled = (schedObj->schedType != workScheduleType_timer);
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
 *  \param [in] duration - A target elapsed time in milliseconds
 */
bool workSched_elapsed(millisTime_t startTime, millisDuration_t duration)
{
    return millis() - startTime > duration;
}

