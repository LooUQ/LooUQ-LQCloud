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
// #include <lq-network.h>

#include <ltemc.h>              // *** TODO remove these dependencies ***
#include <ltemc-mqtt.h>         // ***
#include <lqc-ntwk.h>
//#include <lqc-proto.h>


enum lqcloud_constants
{
    lqc__msg_nameSz = 40 + 1,
    lqc__msg_summarySz = 80 + 1,
    lqc__msg_overheadSz = 12,
    lqc__msg_bodySz = (mqtt__messageSz - lqc__msg_summarySz - lqc__msg_overheadSz),   // mqtt__messageSz from ltemc-mqtt.h

    lqc__defaults_recoveryWaitSecs = 120,
    lqc__magicFlagSz = 4,

    lqc__identity_packageIdSz = 4, 
    lqc__identity_deviceIdSz = 40,
    lqc__identity_deviceLabelSz = 12,
    lqc__identity_userIdSz = 110,
    lqc__identity_hostUrlSz = 50,
    lqc__identity_sasTokenSz = 180,
    lqc__identity_signatureSz = 80,
    lqc__identity_deviceKeySz = 16,

    lqc__network_typeSz = 20,                               /// name@brief description of ntwk type\technology
    lqc__network_nameSz = 20,                               /// name of network carrier

    lqc__connection_hostPort = 8883,
    lqc__connection_onDemand_connDurationSecs = 120,        /// period in seconds an on-demand connection stays open 
    lqc__connection_continous_retryIntrvlSecs = 60,         /// period in seconds between connection attemps
    LQC__connection_retryIntervalSecs = 60,

    LQC__send_resetAtConsecutiveFailures = 2,

    LQC__actionCnt = 12,                                    /// number of application actions, change to needs (lower to save memory)
    LQC__action_MsgIdSz =  37,                              /// size of message Id field (incl NULL)
    LQC__action_nameSz = 17,                                /// Max length of an action name (incl NULL)
    LQC__action_paramsListSz = 40                           /// Max length of an action parameter list, LQ Cloud registered parameter names/types
};


#pragma region LQC Actions

/** 
 * LQ Cloud ACTIONS
 * ============================================================================================= */

//#define LQCACTN_PARAMS_CNT 4          /// Max parameters in application action, change to increase/decrease
// #define LQCACTN_CNT 12                  /// number of application actions, change to needs (lower to save memory)
// #define LQCACTN_NAME_SZ 16              /// Max length of an action name
// #define LQCACTN_PARAMLIST_SZ 40         /// Max length of an action parameter list, LQ Cloud registered parameter names/types

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
//  *  @brief Struct exposing action's parameters collection (names and values as c-strings).
//  * 
//  *  NOTE: This struct maps key\value pairs in an existing HTTP query string formated char array. The array is parsed
//  *  using the lqc_parseQueryStringDict() function. Parsing MUTATES the original char array. The original char array
//  *  must stay in scope as it contains the keys and the values. The keyValueDict struct only provides a higher level
//  *  map of the data stored in the char array and a utility function lqc_getDictValue(key) to access the value.
// */
// typedef struct keyValueDict_tag
// {
//     uint8_t count;                 /// During parsing, how many properties (name/value pairs) were found.
//     char *names[KEYVALUE_DICT_SZ];   /// Array of property names.
//     char *values[KEYVALUE_DICT_SZ];  /// Array of property values (as c-strings). Application is responsible for any type conversion.
// } keyValueDict_t;

#pragma endregion

// use __attribute__((__packed__)) on instances for no padding

typedef struct lqcDeviceConfig_tag 
{
    char magicFlag[SET_PROPLEN(lqc__magicFlagSz)];
    char packageId[SET_PROPLEN(lqc__identity_packageIdSz)];
    char deviceLabel[SET_PROPLEN(lqc__identity_deviceLabelSz)];         // the short name of the device, not globally unique (org unique suggested)
    char deviceId[SET_PROPLEN(lqc__identity_deviceIdSz)];               // the LQCloud unique identifier for the device, this is registered with the cloud to get credentials
                                                                        // typically this is IMEA or MAC address of the device
    char hostUrl[SET_PROPLEN(lqc__identity_hostUrlSz)];                 // full URI of the LQCloud access host 
    uint16_t hostPort;
    char signature[SET_PROPLEN(lqc__identity_signatureSz)];   // security token assigned by LQCloud for the deviceID (includes expiry timestamp)
    // char validationKey[SET_PROPLEN(lqc__identity_validationKeySz)];
} lqcDeviceConfig_t;


/* function definition for LQCloud to get a device configuration from the user application
 * --------------------------------------------------------------------------------------------- */
typedef lqcDeviceConfig_t *(*lqcStartNetwork_func)(bool invokedFromLQC);      /// application event notification and action/info request callback

typedef resultCode_t (*lqcSendMessage_func)(const char *topic, const char* message, uint8_t timeoutSec);


typedef enum lqcDeviceType_tag
{
    lqcDeviceType_sensor,
    lqcDeviceType_ctrllr
} lqcDeviceType_t;


typedef enum lqcDeviceState_tag
{
    lqcDeviceState_offline = 0,
    lqcDeviceState_online = 1,
    lqcDeviceState_running = 2
} lqcDeviceState_t;


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


typedef enum lqcMessagingProto_tag
{
    lqcMessagingProto_mqtt = 1,
    lqcMessagingProto_http,
    lqcMessagingProto_coap
} lqcMessagingProto_t;


typedef enum lqcSendQoS_tag
{
    lqcSendQoS_bestEffort = 0,
    lqcSendQoS_required,
    lqcSendQoS_acknowledged
} lqcSendQoS_t;


// typedef enum lqcConnect_tag
// {
//     lqcConnect_mqttDetached = 0,            /// device is send only using HTTP and not connected to LQCloud
//     lqcConnect_mqttOnDemand = 1,            /// device stays offline most of the time and connects to send or at prescribed intervals 
//     lqcConnect_mqttContinuous = 2,          /// device is connected all of the time, but will not block local processing if offline
//     lqcConnect_httpSensor = 3,
//     lqcConnect__invalid
// } lqcConnect_t;


typedef enum lqcConnectState_tag                
{
    lqcConnectState_idleClosed = 0,

    lqcConnectState_providerReady,
    lqcConnectState_networkReady,

    lqcConnectState_messagingOpen = 10,
    lqcConnectState_messagingConnected,
    lqcConnectState_messagingReady,             /// connected and SUBSCRIBED to receive topics

    lqcConnectState_sendFault = 254,
    lqcConnectState_connFault = 255
} lqcConnectState_t;


// typedef enum lqcSendResult_tag
// {
//     lqcSendResult_sent = 0,
//     lqcSendResult_queued = 1,
//     lqcSendResult_dropped = 2
// } lqcSendResult_t;

// typedef struct lqcSendResult_tag
// {
//     uint16_t msgId;
//     bool sent;
//     bool queued;
// } lqcSendResult_t;


typedef enum lqcSendResult_tag
{
    lqcSendResult_sent = 1,
    lqcSendResult_queued,
    lqcSendResult_dropped
} lqcSendResult_t;


typedef enum lqcQOS_tag
{
    lqcQOS_basic = 0,
    lqcQOS_important
} lqcQOS_t;


// typedef struct lqcConnectInfo_tag
// {
//     lqcMessagingProto_t msgProto;
//     lqcConnect_t connectMode;
//     lqcConnectState_t state;
//     uint32_t stateEnteredAt;
// } lqcConnectInfo_t;


// typedef struct lqcOdConnectInfo_tag
// {
//     uint32_t odc_interConnectTicks;
//     uint32_t odc_holdConnectTicks;
//     uint32_t odc_connectAt;
//     uint32_t odc_disconnectAt;
// } lqcOdConnectInfo_t;


// typedef struct lqcSensor_tag
// {
//     int id;
//     uint32_t sampleLastAt;                  /// last sample taken instance (milli count)
//     uint32_t samplePeriod;                  /// the planned interval between samples
//     uint16_t sampleCount;                   /// the number of invokes for the sampleCallback, cleared with each alert/report
//     void *sampleCallback;                   /// user defined function to perform sample acquisition and report formatting

//     uint32_t rptLastAt;                     /// last reporting instance (milli count)
//     uint32_t rptPeriod;                     /// the planned interval between device to LQCloud sensor samples reporting
//     char *rptBuffer;                        /// character buffer to store sample event output
//     uint16_t rptBufferSz;                   /// buffer size in characters, includes the intended /0 char string terminating symbol
//     uint32_t alertAt;                       /// source millis cnt at event sourcing
// } lqcSensor_t;


/*  LQCloud/device time syncronization; at each LQCloud interaction cloud will send new epochRef (Unix time of cloud)
 *  Device will record this value along with the current millis count. This information is sent to cloud with 
 *  (epochRef and elapsed, where elapsed = now (millis) - epochAt) alert/report connection to allow LQCloud to correlate
 *  device information at a specific date/time.
 * 
 *  Timing derived from millisecond counter, such as Arduini millis(); max at 49.7 days 
 */
typedef struct lqcEpoch_tag
{
    char epochRef[12];                      /// unix epoch reported at last alert/report interaction with LQCloud
    uint32_t epochAt;                       /// millis counter at the last alert/report interaction with LQCloud
} lqcEpoch_t;


// /*  Application Callbacks
//  ------------------------------------------------------------------------------------------------------------------- */
// /* UNR - Application unsolicited notification and request callback (UNR)
//  * Callback to application from LQCloud to notify it of significant events (ex: cloud connected) or request action/information from the app
//  * All returns are const char* to static application values (LQCloud safely checks for null pointer).
// */
// //typedef const char* (*unr_func)(uint8_t unrCode, const char *unrOrigin, const char *unrMessage);        /// application unsolicited notification and request callback (UNR)

// typedef appRqstResult_t (*appRqst_func)(uint8_t rqstCode, const char *origin, const char *message);
// typedef void (*appNotf_func)(uint8_t notfCode, const char *origin, const char *message);
// /*
//  */
// typedef void (*yield_func)();                                                                           /// callback to allow for app background processingfor extended duration operations to allow for 


/** 
 *  @brief typedef of LQ Cloud application function to perform an action invoked from LQCloud
 *  The action needs to be defined with the below signature. LQCloud provides the xxxxx function to easily
 *  parse the params to key/value dictionary and the xxxxx function to retrieve them (as strings).
*/
typedef void (*lqcAction_func)(keyValueDict_t params);

/**
 * @brief Data received from LQCloud
 * 
 */
//typedef void (*lqcRecvMsg_func)(const char *topic, char *message, uint16_t messageSz);


#ifdef __cplusplus
extern "C"
{
#endif


/* Legacy LQCloud
*/
// void lqc_create(void *protoCtrl, lqcStartNetwork_func startNetworkCB, appEventCallback_func appEventCB, yield_func yieldCB, char *validationKey);
void lqc_create(lqcDeviceType_t deviceType, lqcDeviceConfig_t *deviceConfig, lqcSendMessage_func sendMessageCB, applEvntNotify_func applEvntNotifyCB, applInfoRequest_func applInfoRequestCB, yield_func yieldCB, char *validationKey);

void lqc_setDeviceLabel(const char *label);
void lqc_setDeviceKey(const char *key);
void lqc_enableDiagnostics(diagnosticInfo_t *diagnosticsInfoBlock);

void lqc_start(uint8_t resetCause);

bool lqc_isOnline();

void lqc_receiveMsg(char *message, uint16_t messageSz, const char *props);
void lqc_setEventResponse(uint8_t requestEvent, uint16_t result, const char *response);
void lqc_doWork();

lqcSendResult_t lqc_sendTelemetry(const char *evntName, const char *evntSummary, const char *bodyJson);
lqcSendResult_t lqc_sendAlert(const char *alrtName, const char *alrtSummary, const char *bodyJson);

lqcSendResult_t lqc_diagnosticsCheck(diagnosticInfo_t *diagInfo);

bool lqc_registerApplicationAction(const char *actnName, lqcAction_func applActionCB, const char *paramList);

void LQC_sendActionResponse(uint16_t resultCode, lqcEventClass_t eventClass, const char *bodyJson);

void lqc_sendActionResponse(uint16_t resultCode, const char *bodyJson);

char *lqc_getDeviceId();
char *lqc_getDeviceLabel();
uint8_t lqc_getProtoState();

wrkTime_t wrkTime_create(millisDuration_t intervalMillis);
void wrkTime_start(wrkTime_t *schedObj);
void wrkTime_stop(wrkTime_t *schedObj);
bool wrkTime_isRunning(wrkTime_t *schedObj);
bool wrkTime_doNow(wrkTime_t *schedObj);
bool wrkTime_isElapsed(millisTime_t timerVal, millisDuration_t duration);

void lqc_composeIothUserId(char *userId, uint8_t uidSz, const char* hostUrl, const char *deviceId);
void lqc_composeIothSasToken(char *sasToken, uint8_t sasSz, const char* hostUrl, const char *deviceId, const char* signature);
void lqc_setDeviceConfigFromSASToken(const char* sasToken, lqcDeviceConfig_t *deviceConfig);

/** 
 *  \brief Register a callback to change communications device power profile. Enter/exit sleep/PSM/eDRX/etc.
 *  \param [in] powerSaveCB Enum specifying the LTE LPWAN protocol(s) to scan; 0=LTE M1,1=LTE NB1,2=LTE M1 and NB1
 */
void lqc_registerPowerSaveCallback(powerSaveCallback_func powerSaveCB);


// const char *lqc_parseSasTokenForDeviceId(const char* sasToken);
// const char *lqc_parseSasTokenForUserId(const char* sasToken);
// const char *lqc_parseSasTokenForHostUri(const char* sasToken);

// // helpers
//void sendLqDiagnostics(const char *deviceName, lqDiagInfo_t *diagInfo);


uint8_t lqcProto_send(uint8_t instId, const char *header, const char *body, lqcQOS_t qos, uint8_t timeoutSeconds);

/*
 ------------------------------------------------------------------------------------------------- */

uint8_t lqcProto_connect(uint8_t instId, const char* host, const char *id, const char* secToken);
uint8_t lqcProto_disconnect(uint8_t instId);

uint8_t lqcProto_readState(uint8_t instId);


#ifdef __cplusplus
}
#endif // !__cplusplus

#endif  /* !__LQCLOUD_H__ */
