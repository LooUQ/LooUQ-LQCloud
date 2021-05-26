/******************************************************************************
 *  \file lqCloud.h
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
 ******************************************************************************
 * Master header for LooUQ Cloud (C99) Client
 *****************************************************************************/

#ifndef __LQCLOUD_H__
#define __LQCLOUD_H__

#include <ltemc.h>                          // TODO remove this dependency

#include "lqTypes.h"
#include "lqc-collections.h"
#include "lqDiagnostics.h"

// #include "lqc_telemetry.h"
// #include "lqc_actions.h"
// #include "lqc_alerts.h"
// #include "lqc_wrkTime.h"
// #include "lqc_json.h"

#define LQiBLOCK_MAGIC 0x100c
#define LQiBLOCK_ID_LQCDEVICE 201


#define KEYVALUE_DICT_CNT 15

/* LQCloud EVENT size defs are for application (LQC consumers) originating content buffers
 * Applicable to telemetry, alert messages and action reponses
 ----------------------------------------------------------------------------------------------- */
#define LQC_EVENT_NAME_SZ 41
#define LQC_EVENT_SUMMARY_SZ 81
#define LQC_EVENT_OVRHD_SZ 12
#define LQC_EVENT_BODY_SZ (LQMQ_MSG_MAXSZ - LQC_EVENT_SUMMARY_SZ - LQC_EVENT_OVRHD_SZ)

#define LQC_RECOVERY_WAIT_SECONDS 120
#define ACTION_RESULT_SUCCESS 200
#define ACTION_RESULT_NOTFOUND 404
#define ACTION_RESULT_FAILURE 500

#define LQC_DEVICEID_SZ 41
#define LQC_DEVICENAME_SZ 13
#define LQC_URL_SZ 49
#define LQC_SASTOKEN_SZ 171
#define LQC_APPKEY_SZ 9

#define LQNTWK_TYPE 20                                      ///< name\brief description of ntwk type\technology
#define LQNTWK_NAME 20                                      ///< name of network carrier

#define LQCCONN_ONDEMAND_CONNDURATION 120                   ///< period in seconds an on-demand connection stays open 
#define LCQCONN_REQUIRED_RETRYINTVL 10000                   ///< period in millis between required connectivity connect attemps



/* FROM LTEm1c:mqtt.h
 * --------------------------------------------------------------------------------------------- */
#define LQMQ_MSG_MAXSZ 1548                                 ///< largest MQTT send message for BGx
#define LQMQ_TOPIC_SUB_MAXSZ 90                             ///< total buffer size to construct topic for pub\sub AT actions
#define LQMQ_TOPIC_PUB_MAXSZ 437                            ///< total buffer size to construct topic for pub\sub AT actions

#define LQMQ_SEND_QUEUE_SZ 2
#define LQMQ_SEND_RETRY_DELAY 5000
#define LQMQ_SEND_RETRY_CONNECTIONRESETRETRIES 3

//#define ASSERT(trueResult, failMsg)  if(!(trueResult))  LQC_faultHandler(failMsg)
//#define ASSERT_NOTEMPTY(string, failMsg)  if(string[0] == '\0') LQC_faultHandler(failMsg)


#pragma region wrkTime

/* Work Time 
------------------------------------------------------------------------------------------------ */

#define PERIOD_FROM_SECONDS(period)  (period * 1000)
#define PERIOD_FROM_MINUTES(period)  (period * 1000 * 60)
#define PERIOD_FROM_HOURS(period)  (period * 1000 * 60 * 60)

typedef unsigned long millisTime_t, millisDuration_t;   // aka uint32_t 


/** 
 *  \brief typedef of workSchedule object to coordinate the timing of application work items.
*/
typedef struct wrkTime_tag
{
    millisTime_t period;            ///< Time period in milliseconds 
    millisTime_t lastAtMillis;      ///< Tick (system millis()) when the workSchedule timer last signaled in workSched_doNow()
    uint8_t enabled;                ///< Reset on timer sched objects, when doNow() (ie: timer is queried) following experation.
} wrkTime_t;


/** 
 *  \brief typedef of MQTT subscription receiver function (required signature for appl callback).
*/
//typedef void (*lqcRecv_func_t)(char *topic, char *props, char *message);

// extern lqCloudDevice_t g_lqCloud;

#pragma endregion



#pragma region LQC Actions

/** 
 * LQ Cloud ACTIONS
 * ============================================================================================= */

//#define LQCACTN_PARAMS_CNT 4          ///< Max parameters in application action, change to increase/decrease
#define LQCACTN_CNT 12                  ///< number of application actions, change to needs (lower to save memory)
#define LQCACTN_NAME_SZ 16              ///< Max length of an action name
#define LQCACTN_PARAMLIST_SZ 40         ///< Max length of an action parameter list, LQ Cloud registered parameter names/types

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

#pragma endregion


typedef enum networkType_tag
{
    networkType_lte = 0,
    networkType_wifi = 1
} networkType_t;


typedef enum lqcConnectode_tag
{
    lqcConnectMode_onDemand = 0,           ///< device stays offline most of the time and connects to send or at prescribed intervals 
    lqcConnectMode_continuous = 1,         ///< device is connected all of the time, but will not block local processing if offline
    lqcConnectMode_required = 2            ///< device is connected all of the time and blocks local processing while disconnected
} lqcConnectMode_t;


typedef enum lqcConnectState_tag
{
    lqcConnectState_closed = 0,
    lqcConnectState_opened = 1,
    lqcConnectState_connected = 2,
    lqcConnectState_ready = 3               // aka subscribed
} lqcConnectState_t;


typedef enum batteryWarning_tag
{
    batteryStatus_good = 0,
    batteryStatus_yellow = 1,
    batteryStatus_red = 2
}  batteryWarning_t;


typedef struct lqcDeviceConfig_tag
{
    uint32_t iBlockMagic;
    uint8_t iBlockId;
    char deviceName[LQC_DEVICENAME_SZ];
    char deviceId[LQC_DEVICEID_SZ];
    char url[LQC_URL_SZ];
    char sasToken[LQC_SASTOKEN_SZ];
    char appKey[LQC_APPKEY_SZ];
} lqcDeviceConfig_t;


typedef enum lqcEventClass_tag
{
    lqcEventClass_lqcloud = 0,
    lqcEventClass_application = 1
} lqcEventClass_t;


typedef enum lqcEventType_tag
{
    lqcEventType_telemetry = 1,
    lqcEventType_alert = 2,
    lqcEventType_actnResp = 3
} lqcEventType_t;


typedef enum lqcNotifType_tag
{
    lqcNotifType_info = 0,
    lqcNotifType_alert = 1,
    lqcNotifType_powerFail = 2,
    lqcNotifType__INFO = 100,

    // notifType__NETWORK 100-199
    // __transport 101-109
    // __protocols 111-129
    // __services 131-149

    lqcNotifType__SERVICES = 140,
    lqcNotifType_connect = 141,
    lqcNotifType_disconnect = 142,

    lqcNotifType__CATASTROPHIC = 200,
    lqcNotifType_hardFault = 255
} lqcNotifType_t;


typedef void (*appNotify_func)(uint8_t notifType, const char *notifMsg);    ///< (optional) app notification callback, notifyType casts to notificationType
typedef bool (*pwrStatus_func)();                                           ///< (optional) callback into appl to determine power status, true if power is good
typedef batteryWarning_t (*battStatus_func)();                              ///< (optional) callback into appl to determine battery status
typedef int (*memStatus_func)();                                            ///< (optional) callback into appl to determine memory available (between stack and heap)
typedef int (*ntwkSignal_func)();                                           ///< (optional) callback into appl to determine network signal strength

typedef bool (*ntwkStart_func)();       ///< callback into appl to wake communications hardware, returns true if HW was made ready
typedef void (*ntwkStop_func)();        ///< callback into appl to sleep communications hardware


/** 
 *  \brief typedef of LQ Cloud application define device action function (required signature).
*/
typedef void (*lqc_ActnFunc_t)(keyValueDict_t params);


#ifdef __cplusplus
extern "C"
{
#endif


void lqc_create(lqDiagResetCause_t resetCause,
                appNotify_func appNotifyCB, 
                ntwkStart_func ntwkStartCB,
                ntwkStop_func ntwkStopCB,
                pwrStatus_func pwrStatCB, 
                battStatus_func battStatCB, 
                memStatus_func memoryStatCB);

void lqc_configTelemetryCallbacks(pwrStatus_func pwrStatFunc, battStatus_func battStatFunc, memStatus_func memStatFunc);
void lqc_setDeviceName(const char *shortName);
//void lqc_configMsgLifespans(uint16_t telemetryLifespan, uint16_t alertLifespan, uint16_t actionLifespan);

void lqc_start(const char *hubAddr, const char *deviceId, const char *sasToken, const char *actnKey);
void lqc_setConnectMode(lqcConnectMode_t connMode, wrkTime_t *schedObj);
lqcConnectMode_t lqc_getConnectMode();
lqcConnectState_t lqc_getConnectState(const char *hostName, bool forceRead);

void lqc_doWork();
bool lqc_sendTelemetry(const char *evntName, const char *evntSummary, const char *bodyJson);
bool lqc_sendAlert(const char *alrtName, const char *alrtSummary, const char *bodyJson);

bool lqc_regApplAction(const char *actnName, lqc_ActnFunc_t applActn_func_t, const char *paramList);
void lqc_sendActionResponse(uint16_t resultCode, const char *bodyJson);

char *lqc_getDeviceId();
char *lqc_getDeviceName();
char *lqc_getDeviceShortName();

wrkTime_t wrkTime_create(millisDuration_t intervalMillis);
void wrkTime_start(wrkTime_t *schedObj);
void wrkTime_stop(wrkTime_t *schedObj);
bool wrkTime_isRunning(wrkTime_t *schedObj);
bool wrkTime_doNow(wrkTime_t *schedObj);
bool wrkTime_isElapsed(millisTime_t timerVal, millisDuration_t duration);


// // helpers
void sendLqDiagnostics(const char *deviceName, lqDiagInfo_t *diagInfo);
keyValueDict_t lqc_parseQueryStringDict(char *dictSrc);
// char *lqc_getActionParamValue(const char *paramName, keyValueDict_t actnParams);
// lqcJsonProp_t lqc_getJsonProp(const char *jsonSrc, const char *propName);


#ifdef __cplusplus
}
#endif // !__cplusplus



#endif  /* !__LQCLOUD_H__ */
