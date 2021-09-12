/******************************************************************************
 *  \file lqc_internal.h
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
 * LQ Cloud Internal Functions Declarations
 *****************************************************************************/

#ifndef __LQC_INTERNAL_H__
#define __LQC_INTERNAL_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <lq-types.h>
#include <lq-diagnostics.h>
#include <lqcloud.h>

#include <ltemc-mqtt.h>                             // REQUIRED communications for now!
#include <ltemc-tls.h>
#include "lqc-filecodes.h"


enum 
{
    LQC__sendQueueSz = 2,
    LQC__connectionRetryCnt = 3,
    LQC__messageIdSz = 36,

    LQC__sendRetryDelayMS = 5000,
    LQC__sendRetriesMax = 2,
    LQC__connectionRetryWaitSec = 120,

    LQMQ_MSG_MAXSZ = 1548,                                  ///< largest MQTT send message for BGx
    LQMQ_TOPIC_PUB_MAXSZ = 437,                             ///< total buffer size to construct topic for pub\sub AT actions
    LQMQ_SEND_QUEUE_SZ = 2,
    LOOUQ_FLASHDICTKEY__LQCDEVICECONFIG = 201,
    DVCSTATUS_SZ = 61,
    LQC_EVNTCLASS_SZ = 5,
    LQCACTN_BUF_SZ = 80,                                    ///< good averaged for up to a couple params
    LQCACTN_LQCACTIONS_BODY_SZ = 180                        ///< calculated from actual JSON
};

// #define LQMQ_MSG_MAXSZ 1548                                 ///< largest MQTT send message for BGx
// #define LQMQ_TOPIC_SUB_MAXSZ 90                             ///< total buffer size to construct topic for pub\sub AT actions
// #define LQMQ_TOPIC_PUB_MAXSZ 437                            ///< total buffer size to construct topic for pub\sub AT actions

// #define LQMQ_SEND_QUEUE_SZ 2
// #define LQMQ_SEND_RETRY_DELAY 5000
// #define LQMQ_SEND_RETRY_CONNECTIONRESETRETRIES 3
// #define LQC_RECOVERY_WAIT_SECONDS 120
// #define LOOUQ_FLASHDICTKEY__LQCDEVICECONFIG 201


// #define TOPIC_SZ 201
// #define PROPS_SZ 201
// #define DVCSTATUS_SZ 61
// #define LQC_EVNTCLASS_SZ 5

// #define LQCACTN_BUF_SZ 80                   ///< good averaged for up to a couple params
// #define LQCACTN_LQCACTIONS_BODY_SZ 180      ///< calculated from actual JSON

// #define IOTHUB_URL_SZ 48
// #define IOTHUB_USERID_SZ 120
// #define IOTHUB_SASTOKEN_SZ 200
// #define IOTHUB_MESSAGEID_SZ 38
// #define IOTHUB_PORT 8883
// #define IOTHUB_CONNECT_RETRIES 12
// #define EXPAND_ASSERTS
// #ifdef EXPAND_ASSERTS
// #define ASSERT(true_cond, fail_msg)  ((true_cond) ? 0 : LQC_notifyApp(255, (fail_msg)))
// #else
// #define ASSERT(true_cond, fail_msg)  (0)
// #endif


typedef struct lqcApplAction_tag
{
    char name[LQC__action_nameSz];              ///< Action name, know by LQ Cloud.
    lqc_ActnFunc_t func;                        ///< Action function to be invoked, to perform device action.
    char paramList[LQC__action_paramsListSz];   ///< this is used for registration with LQ Cloud, function will receive a propsDict_t parameter.
} lqcApplAction_t;


typedef struct lqcMetrics_tag           ///< Note: diagnostic counters below can be optionally reset with remote action
{
    uint8_t connectResets;              ///< Number of connection resets initiated by LQCloud connection manager. Reset by system reset.
    uint8_t publishMaxRetries;          ///< Maximum number of publish retries encountered for any message. Reset on connection reset.
    uint16_t publishMaxDuration;        ///< Maximum duration for any mqtt publish operation. Reset on connection reset.
    uint16_t publishLastDuration;       ///< Duration (in millis) of last mqtt publish operation
    uint8_t publishLastFailResult;      ///< If send D2C msg fails, message type. 
} lqcMetrics_t;


typedef enum lqcMetricsType_tag
{
    lqcMetricsType_metrics = 0,
    lqcMetricsType_diagnostics = 1,
    lqcMetricsType_fault = 2,
    lqcMetricsType_all = 3
} lqcMetricsType_t;


typedef enum lqcStartType_tag
{
    lqcStartType_cold = 0,
    lqcStartType_recover = 1
} lqcStartType_t;


typedef struct lqcMqttQueuedMsg_tag
{
    uint32_t queuedAt;
    uint32_t lastTryAt;
    uint8_t retries;
    char topic[mqtt__topic_bufferSz];
    char msg[mqtt__messageSz];
} lqcMqttQueuedMsg_t;


// typedef enum batteryWarning_tag
// {
//     batteryStatus_good = 0,
//     batteryStatus_yellow = 1,
//     batteryStatus_red = 2
// }  batteryWarning_t;


/** 
 *  \brief typedef of MQTT subscription receiver function (required signature for appl callback).
*/
typedef void (*lqcRecv_func_t)(char *topic, char *props, char *message);


/** 
 *  \brief Struct the respresents a connection to the LooUQ Cloud - IoT Application Integration Platform.
*/
typedef struct lqCloudDevice_tag
{
    mqttCtrl_t *mqttCtrl;                                   ///< MQTT connection to access LQCloud
    char deviceId[lqc__identity_deviceIdSz];                ///< LQ Cloud device ID, generally expressed as a GUID, but can be any unique string value like a IMEI or ICCID.
    char deviceLabel[lqc__identity_deviceLabelSz +1];       ///< device short name for local display
    char networkType[lqc__network_typeSz +1];               ///<
    char networkName[lqc__network_nameSz +1];               ///<
    char hostUri[lqc__identity_hostUriSz +1 ];              ///< URL of the configured LooUQ Cloud ingress point (Az IoTHub)
    char tokenSigExpiry[lqc__identity_tokenSigExpirySz +1]; ///< Signature and expiry for access token
    char *orgKey;                                           ///< Pointer to optional customer specific 64-hex digit validation key. Used by device actions and OTA package validation.
    lqcConnectMode_t connectMode;                           ///< Device to Cloud connection mode: OnDemand, Continuous, Required.
    wrkTime_t *connectSched;                                ///< If connection mode OnDemand: period timer for when to connect
    lqcConnectState_t connectState;                         ///< Device to Cloud connection state: Closed, Open, Connected, Subscribed
    bool connectionErrors;                                  ///< Set on connection failure or excessive send retries.
    uint8_t onDemandTimeoutMinutes;                         ///< The time the communications channel stays open for onDemand mode.
    lqcMqttQueuedMsg_t mqttSendQueue[LQC__sendQueueSz];     ///< Outgoing messages, only queue if initial send fails
    uint8_t mqttQueuedHead;                                 ///< array index of next message (from queued order)
    uint8_t mqttQueuedTail;                                 ///< array index of next message (from queued order)
    uint8_t mqttSendOverflowCnt;                            ///< counter to track send retry buffer overflows, buffer is fixed at build for deployed stability and kept as small as possible.
    lqcApplAction_t applActions[LQC__actionCnt];            ///< Application invokable public methods (registered with LQ Cloud). LQ Cloud validates requests prior to messaging device.
    char actnMsgId[36];                    ///< Action request mId, will be aCId (correlation ID).
    char actnName[LQC__action_nameSz];                      ///< Last action requested by cloud. Is reset on action request receive.
    uint16_t actnResult;                                    ///< Action result code for last action request. 
    appNotify_func notificationCB;                          ///< (optional) Application notification callback: LQCloud notifies application of status events (connection, disconnect, etc.).
    ntwkStart_func networkStartCB;                          ///< (recommended) Application method to start the network and mqtt
    ntwkStop_func networkStopCB;                            ///< (recommended) Application method to stop the network and mqtt
    pwrStatus_func powerStatusCB;                           ///< (optional) Callback for LQCloud to get power status. 
    battStatus_func batteryStatusCB;                        ///< (optional) Callback for LQCloud to get battery status. 
    memStatus_func memoryStatusCB;                          ///< (optional) Callback for LQCloud to get memory status.
    ntwkSignal_func ntwkSignalCB;                           ///< (optional) Callback for LQCloud to get network signal level\quality status.
    lqcMetrics_t perfMetrics;                               ///< Internal operations tracking counters
} lqCloudDevice_t;


/* LQCloud Internal Use Functions LQC_ prefix
* These are not defined static because they are used across compilation units.
------------------------------------------------------------------------------------------------ */

uint16_t LQC_getMsgId();
bool LQC_mqttTrySend(const char *topic, const char *body, bool fromQueue);
void LQC_faultHandler(const char *faultMsg);
void LQC_notifyApp(uint8_t notifType, const char *notifMsg);

// alerts
void LQC_sendDeviceStarted(uint8_t startType);
bool LQC_sendAlert(lqcEventClass_t alrtClass, const char *alrtName, const char *alrtSummary, const char *bodyJson);
bool LQC_sendDiagAlert(diagnosticInfo_t * diagInfo);

// actions
void LQC_processActionRequest(const char *actnName, keyValueDict_t actnParams, const char *msgBody);


// metrics
void LQC_clearMetrics(lqcMetricsType_t metricType);
void LQC_composeMetricsReport(char *alrtBody, uint8_t bufferSz);
// void LQC_composeDiagnosticsReport(char *alrtBody, uint8_t bufferSz);
// void LQC_composeFaultReport(char *alrtBody, uint8_t bufferSz);

#endif  /* !__LQC_INTERNAL_H__ */
