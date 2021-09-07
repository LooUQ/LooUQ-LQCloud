/******************************************************************************
 *  \file lqcloud.h
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

#include <lq-types.h>           // LooUQ libraries LQCloud uses
#include <lq-collections.h>
#include <lq-wrkTime.h>

#include <ltemc.h>              // *** TODO remove these dependencies ***
#include <ltemc-mqtt.h>         // ***

enum lqcloud_constants
{
    lqc__event_nameSz = 41,
    lqc__event_summarySz = 81,
    lqc__event_overheadSz = 12,
    lqc__event_bodySz = (mqtt__messageSz - lqc__event_summarySz - lqc__event_overheadSz),   // mqtt__messageSz from ltemc-mqtt.h

    lqc__defaults_recoveryWaitSecs = 120,

    lqc__identity_deviceIdSz = 40,
    lqc__identity_deviceLabelSz = 12,
    lqc__identity_userIdSz = 110,
    lqc__identity_hostUriSz = 50,
    lqc__identity_tokenSasSz = 180,
    lqc__identity_tokenSigExpirySz = 80,
    lqc__identity_organizationKeySz = 12,

    lqc__network_typeSz = 20,                               ///< name\brief description of ntwk type\technology
    lqc__network_nameSz = 20,                               ///< name of network carrier

    lqc__connection_hostPort = 8883,
    lqc__connection_onDemand_connDurationSecs = 120,        ///< period in seconds an on-demand connection stays open 
    lqc__connection_required_retryIntrvlSecs = 10,          ///< period in millis between required connectivity connect attemps

    LQC__actionCnt = 12,                                    ///< number of application actions, change to needs (lower to save memory)
    LQC__action_nameSz = 16,                                ///< Max length of an action name
    LQC__action_paramsListSz = 40                           ///< Max length of an action parameter list, LQ Cloud registered parameter names/types
};


#pragma region LQC Actions

/** 
 * LQ Cloud ACTIONS
 * ============================================================================================= */

//#define LQCACTN_PARAMS_CNT 4          ///< Max parameters in application action, change to increase/decrease
// #define LQCACTN_CNT 12                  ///< number of application actions, change to needs (lower to save memory)
// #define LQCACTN_NAME_SZ 16              ///< Max length of an action name
// #define LQCACTN_PARAMLIST_SZ 40         ///< Max length of an action parameter list, LQ Cloud registered parameter names/types

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



// use __attribute__((__packed__)) on instances for no padding

typedef struct lqcDeviceConfig_tag 
{
    magicFlag_t magicFlag;
    fdictKey_t pageKey;
    char deviceLabel[lqc__identity_deviceLabelSz + 1];
    char deviceId[lqc__identity_deviceIdSz + 1];
    char hostUri[lqc__identity_hostUriSz + 1];
    char tokenSigExpiry[lqc__identity_tokenSigExpirySz + 1];
} lqcDeviceConfig_t;

//"iothub-dev-pelogical.azure-devices.net

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
    lqcConnectState_ready = 3,                                              ///< connected and subscribed
    lqcConnectState_sendFault = 201,
    lqcConnectState_connFault = 202
} lqcConnectState_t;


typedef void (*appNotify_func)(uint8_t notifType, const char *notifMsg);    ///< (optional) app notification callback, notifyType casts to notificationType
typedef uint16_t (*pwrStatus_func)();                                       ///< (optional) callback into appl to determine power status, true if power is good
typedef uint16_t (*battStatus_func)();                                      ///< (optional) callback into appl to determine battery status
typedef uint32_t (*memStatus_func)();                                       ///< (optional) callback into appl to determine memory available (between stack and heap)
typedef int16_t (*ntwkSignal_func)();                                       ///< (optional) callback into appl to determine network signal strength

typedef bool (*ntwkStart_func)(bool resetNtwk);                             ///< callback into appl to wake communications hardware, returns true if HW was made ready
typedef void (*ntwkStop_func)(bool disconnect);                             ///< callback into appl to sleep or fully discconnect communications hardware


/** 
 *  \brief typedef of LQ Cloud application define device action function (required signature).
*/
typedef void (*lqc_ActnFunc_t)(keyValueDict_t params);


#ifdef __cplusplus
extern "C"
{
#endif


void lqc_create(const char *organizationKey,
                const char *deviceLabel,
                appNotify_func appNotifyCB, 
                ntwkStart_func ntwkStartCB,
                ntwkStop_func ntwkStopCB,
                pwrStatus_func pwrStatCB, 
                battStatus_func battStatCB, 
                memStatus_func memoryStatCB);

void lqc_configTelemetryCallbacks(pwrStatus_func pwrStatFunc, battStatus_func battStatFunc, memStatus_func memStatFunc);
void lqc_setDeviceLabel(const char *label);

void lqc_start(mqttCtrl_t *mqttCtrl, const char *sasToken);
void lqc_setConnectMode(lqcConnectMode_t connMode, wrkTime_t *schedObj);
lqcConnectMode_t lqc_getConnectMode();
lqcConnectState_t lqc_getConnectState(const char *hostName);

void lqc_doWork();
bool lqc_sendTelemetry(const char *evntName, const char *evntSummary, const char *bodyJson);
bool lqc_sendAlert(const char *alrtName, const char *alrtSummary, const char *bodyJson);

void lqc_diagnosticsCheck(diagnosticInfo_t *diagInfo);

bool lqc_regApplAction(const char *actnName, lqc_ActnFunc_t applActn_func_t, const char *paramList);
void lqc_sendActionResponse(uint16_t resultCode, const char *bodyJson);

char *lqc_getDeviceId();
char *lqc_getDeviceLabel();

wrkTime_t wrkTime_create(millisDuration_t intervalMillis);
void wrkTime_start(wrkTime_t *schedObj);
void wrkTime_stop(wrkTime_t *schedObj);
bool wrkTime_isRunning(wrkTime_t *schedObj);
bool wrkTime_doNow(wrkTime_t *schedObj);
bool wrkTime_isElapsed(millisTime_t timerVal, millisDuration_t duration);

void lqc_composeTokenSas(char *sasToken, uint8_t sasSz, const char* hostUri, const char *deviceId, const char* sigExpiry);
lqcDeviceConfig_t lqc_decomposeTokenSas(const char* sasToken);


// const char *lqc_parseSasTokenForDeviceId(const char* sasToken);
// const char *lqc_parseSasTokenForUserId(const char* sasToken);
// const char *lqc_parseSasTokenForHostUri(const char* sasToken);

// // helpers
//void sendLqDiagnostics(const char *deviceName, lqDiagInfo_t *diagInfo);


#ifdef __cplusplus
}
#endif // !__cplusplus

#endif  /* !__LQCLOUD_H__ */
