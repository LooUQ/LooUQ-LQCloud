/******************************************************************************
 *  \file lqCloud.h
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2020 LooUQ Incorporated.
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
 ******************************************************************************
 * Master header for LooUQ Cloud (C99) Client
 *****************************************************************************/

#ifndef __LQCLOUD_H__
#define __LQCLOUD_H__

#include <ltem1c.h>
#include "lqc_collections.h"

// #include "lqc_telemetry.h"
// #include "lqc_actions.h"
// #include "lqc_alerts.h"
// #include "lqc_wrkSched.h"
// #include "lqc_json.h"

#define KEYVALUE_DICT_SZ 15

#define LQC_EVENT_NAME_SZ 41
#define LQC_EVENT_SUMMARY_SZ 81
#define LQC_EVENT_BODY_SZ 601
#define LQC_RECOVERY_WAIT_SECONDS 120
#define ACTION_RESULT_SUCCESS 200
#define ACTION_RESULT_NOTFOUND 404
#define ACTION_RESULT_FAILURE 500

// #define BODY_SZ 601
// #define TOPIC_SZ 201
// #define PROPS_SZ 201
// #define DSTAT_SZ 61

// #define IOTHUB_URL_SZ 48
// #define IOTHUB_USERID_SZ 120
// #define IOTHUB_SASTOKEN_SZ 200
// #define IOTHUB_CORRELATIONID_SZ 38
// #define IOTHUB_PORT 8883
// #define LQCLOUD_DEVICEID_SZ 40
// #define LQCLOUD_DVCSHORTNAME_SZ 12

// #define LQCLOUD_ACTIONS_MAX 6
// #define LQCLOUD_APPLKEY_SZ 12


#define ASSERT(trueResult, failMsg)  if(!(trueResult))  LQC_faultHandler(failMsg)
#define ASSERT_NOTEMPTY(string, failMsg)  if(string[0] == '\0') LQC_faultHandler(failMsg)

typedef void (*faultHndlr_func)(const char* faultMsg);
typedef void (*reconnectHndlr_func)();
typedef double (*pwrStatus_func)();
typedef double (*battStatus_func)();
typedef int (*memStatus_func)();
typedef int (*ntwkSignal_func)();


typedef enum 
{
    networkType_lte = 0,
    networkType_wifi = 1
} networkType_t;

typedef enum lqcState_tag
{
    lqcState_closed = 0,
    lqcState_open = 1,
    lqcState_connected,
    lqcState_badHost = 255
} lqcState_t;

typedef enum lqcEventType_tag
{
    lqcEventType_telemetry = 0,
    lqcEventType_alert = 1,
    lqcEventType_actnResp = 2
} lqcEventType_t;

typedef enum lqcEventClass_tag
{
    lqcEventClass_lqcloud = 0,
    lqcEventClass_application = 1
} lqcEventClass_t;


/* Work Schedule 
------------------------------------------------------------------------------------------------ */

#define PERIOD_FROM_SECONDS(period)  (period * 1000)
#define PERIOD_FROM_MINUTES(period)  (period * 1000 * 60)
#define PERIOD_FROM_HOURS(period)  (period * 1000 * 60 * 60)

typedef unsigned long millisTime_t, millisDuration_t;


/** 
 *  \brief Enum describing the behavior of a workSchedule object.
*/
typedef enum workScheduleType_tag
{   
    workScheduleType_periodic = 0,      ///< Periodic workSchedule to reaccure at consitent prescribed intervals
    workScheduleType_timer = 1          ///< Single shot (1 time) workSchedule timer 
} workScheduleType_t;


/** 
 *  \brief typedef of workSchedule object to coordinate the timing of application work items.
*/
typedef struct workSchedule_tag
{
    workScheduleType_t schedType;       ///< type of schedule; periodic (recurring), timer (one-shot). Controls reset of enabled
    millisTime_t period;               ///< Time period in milliseconds 
    millisTime_t beginAtMillis;        ///< Tick (system millis()) when the workSchedule timer was created
    millisTime_t lastAtMillis;         ///< Tick (system millis()) when the workSchedule timer last signaled in workSched_doNow()
    uint8_t enabled;                    ///< Reset on timer sched objects, when doNow() (ie: timer is queried) following experation.
    uint8_t userState;                  ///< User definable information about a workSchedule
} workSchedule_t;


/** 
 *  \brief typedef of MQTT subscription receiver function (required signature for appl callback).
*/
//typedef void (*lqcRecv_func_t)(char *topic, char *props, char *message);

// extern lqCloudDevice_t g_lqCloud;



/** 
 * LQ Cloud ACTIONS
 * ============================================================================================= */

#define LQCACTN_PARAMS_CNT 4        ///< Max parameters in application action, change to increase/decrease
#define LQCACTN_CNT 6               ///< number of application actions, change to needs (lower to save memory)
#define LQCACTN_NAME_SZ 20          ///< Max length of an action name
#define LQCACTN_AKEY_SZ 20          ///< Max length of an action key (optional)
#define LQCACTN_PARAMLIST_SZ 80     ///< Max length of an action parameter list, LQ Cloud registered parameter names/types

/* To be called in application space for each action to make accessible via LooUQ Cloud
*/
#define REGISTER_ACTION(aName, aFunc, paramList)  lqcActn_regApplAction(aName, &aFunc, paramList)

#define LQCACTN_PARAMTYPE_TEXT   "text"
#define LQCACTN_PARAMTYPE_INT    "int"
#define LQCACTN_PARAMTYPE_FLOAT  "float"
#define LQCACTN_PARAMTYPE_BOOL   "bool"


/* APPLICATION ACTION (function) PARAMETER LISTS
 *
 * LooUQ Cloud validates the action signature in the Cloud at the WEBAPI layer for device actions. 
 * This reduces the possibility of errant or malicious attempts at forcing action requests to the device.
 * WebAPI checks the function name, parameter count, parameter names and parameter types. 
 * 
 * To accomplish this, device actions are registered with LooUQ Cloud client, which then informs the 
 * LooUQ Cloud WebAPI of the actions available on a device. This information is cached in the cloud, but 
 * typically reset at each device start (can be marked to persist device reset).
 * 
 * Steps:
 *  1) create your action function adhering to applActn_func_t signature
 *  2) craft a paramList string for registering your action function with the LooUQ Cloud
 *  3) register your function
 *  4) wait for LooUQ Cloud device services to invoke it and pass in your parameter values (as c-strings).
 * 
 * Action parameter lists are formatted like URL parameters. Each is a "name=value" pair, parameter 
 * pairs are connected with the '&' char.
 * 
 * LIKE: paramName1=paramType1&paramName2=paramType2&paramName3=paramType3
 *  
 * WHERE: paramType is expessed as one of the "LQCACTN_PARAMTYPE_" macro define values.
 * 
 * The easiest way to construct the information needed by LooUQ Cloud to registere your function is to
 * initialize a const char variable with the pattern below. Then pass that into the REGISTER_ACTION macro 
 * for the paramList macro parameter.
 * 
 * EXAMPLE: const char *applFunc_1_params = { "deviceState="  LQCACTN_PARAMTYPE_BOOL  "&"  "level=" LQCACTN_PARAMTYPE_INT };
 *                                             ^name      ^nam\val-conn  ^paramType    ^param-conn
 * -OR-
 *          const char *applFunc_1_params = { "deviceState" "=" LQCACTN_PARAMTYPE_BOOL "&" "level" "=" LQCACTN_PARAMTYPE_INT };
 * 
 * Valid LQCACTN_PARAMTYPE_ are...
 *   -- TEXT
 *   -- INT
 *   -- BOOL
 *   -- FLOAT
 * 
 * So LQCACTN_PARAMTYPE_TEXT and LQCACTN_PARAMTYPE_BOOL are both valid (checked in build process).
 * LQCACTN_PARAMTYPE_NUMBER and LQCACTN_PARAMTYPE_STRING are NOT VALID.
 */


// /** 
//  *  \brief Struct exposing action's parameters collection (names and values as c-strings).
//  * 
//  *  NOTE: This struct maps key\value pairs in an existing HTTP query string formated char array. The array is parsed
//  *  using the lqc_parseQueryStringDict() function. Parsing MUTATES the original char array. The original char array
//  *  must stay in scope as it contains the keys and the values. The keyValueDict struct only provides a higher level
//  *  map of the data stored in the char array and a utility function lqc_getDictValue(key) to access the value.
// */
// typedef struct keyValueDict_tag
// {
//     uint8_t count;                 ///< During parsing, how many properties (name/value pairs) were found.
//     char *names[KEYVALUE_DICT_SZ];   ///< Array of property names.
//     char *values[KEYVALUE_DICT_SZ];  ///< Array of property values (as c-strings). Application is responsible for any type conversion.
// } keyValueDict_t;


/** 
 *  \brief typedef of LQ Cloud application define device action function (required signature).
*/
typedef void (*lqc_ActnFunc_t)(keyValueDict_t params);


#ifdef __cplusplus
extern "C"
{
#endif


void lqc_create(reconnectHndlr_func reconnectHandler, faultHndlr_func faultHandler, pwrStatus_func pwrStatFunc, battStatus_func battStatFunc, memStatus_func memStatFunc);
void lqc_connect(const char *hubAddr, const char *deviceId, const char *sasToken, const char *actnKey);
lqcState_t lqc_getState(const char *hostName);

void lqc_doWork();
void lqc_sendTelemetry(const char *evntName, const char *evntSummary, const char *bodyJson);
void lqc_sendAlert(const char *alrtName, const char *alrtSummary, const char *bodyJson);

bool lqc_regApplAction(const char *actnName, lqc_ActnFunc_t applActn_func_t, const char *paramList);
void lqc_sendActionResponse(uint16_t resultCode, const char *bodyJson);

char *lqc_getDeviceId();
char *lqc_getDeviceName();
char *lqc_getDeviceShortName();

workSchedule_t workSched_createPeriodic(millisDuration_t intervalMillis);
workSchedule_t workSched_createTimer(millisDuration_t intervalMillis);
void workSched_reset(workSchedule_t *schedObj);
bool workSched_doNow(workSchedule_t *schedObj);
bool workSched_elapsed(millisTime_t timerVal, millisDuration_t duration);


// // helpers
// keyValueDict_t lqc_parseQueryStringDict(char *dictSrc);
// char *lqc_getActionParamValue(const char *paramName, keyValueDict_t actnParams);
// lqcJsonProp_t lqc_getJsonProp(const char *jsonSrc, const char *propName);


#ifdef __cplusplus
}
#endif // !__cplusplus



#endif  /* !__LQCLOUD_H__ */
