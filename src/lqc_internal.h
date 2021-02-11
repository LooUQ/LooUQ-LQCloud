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

#include <lqCloud.h>


#define TOPIC_SZ 201
#define PROPS_SZ 201
#define DVCSTATUS_SZ 61

#define IOTHUB_URL_SZ 48
#define IOTHUB_USERID_SZ 120
#define IOTHUB_SASTOKEN_SZ 200
#define IOTHUB_MESSAGEID_SZ 38
#define IOTHUB_PORT 8883
#define IOTHUB_CONNECT_RETRIES 5
// #define LQCLOUD_DEVICEID_SZ 40
// #define LQCLOUD_DVCSHORTNAME_SZ 12
#define LQCLOUD_ACTIONS_MAX 6
// #define LQCLOUD_APPLKEY_SZ 12

/* Azure IoTHub Templates */

/* C2D Receive Topic (subscribe) Template
 * %s = device ID
*/
#define MQTT_IOTHUB_C2D_RECVTOPIC_TMPLT "devices/%s/messages/devicebound/#"

/* UserID Template
 * %s = iothub URL
 * %s = device ID
*/
#define MQTT_IOTHUB_USERID_TMPLT "%s/%s/?api-version=2018-06-30" 

/* Message Properties Template
 * %s = dId : device ID
 * %d = mId : msg ID
 * %s = evT : event\message type (tdat, alrt, etc.)
 * %s = cId : correlation ID (message ID from action request)
 * %s = evC : event class ()
 * %s = evN : event name
*/
#define MQTT_MSG_D2CTOPIC_TELEMETRY_TMPLT "devices/%s/messages/events/mId=~%d&mV=1.0&evT=tdat&evC=%s&evN=%s"
#define MQTT_MSG_D2CTOPIC_ALERT_TMPLT "devices/%s/messages/events/mId=~%d&mV=1.0&evT=alrt&evC=%s&evN=%s"
#define MQTT_MSG_D2CTOPIC_ACTIONRESP_TMPLT "devices/%s/messages/events/mId=~%d&mV=1.0&evT=aRsp&aCId=%s&evC=%s&evN=%s&aRslt=%d"


typedef struct lqcApplAction_tag
{
    char name[LQCACTN_NAME_SZ];             ///< Action name, know by LQ Cloud.
    lqc_ActnFunc_t func;                    ///< Action function to be invoked, to perform device action.
    char paramList[LQCACTN_PARAMLIST_SZ];   ///< this is used for registration with LQ Cloud, function will receive a propsDict_t parameter.
} lqcApplAction_t;

// typedef enum lqcPropType_tag
// {
//     lqcPropType_object = 0,
//     lqcPropType_array = 1,
//     lqcPropType_string = 2,
//     lqcPropType_primitive = 3
// } lqcPropType_t;


// typedef struct lqcProp_tag
// {
//     char *name;
//     lqcPropType_t type;
//     char *value;
//     uint16_t len;
// } lqcProp_t;

typedef enum lqcStartType_tag
{
    lqcStartType_cold = 0,
    lqcStartType_recover = 1
} lqcStartType_t;

typedef struct lqcMqttQueuedMsg_tag
{
    millisTime_t queuedAt;
    char topic[1];
    char msg[1];
    // ??? eventType (for timeout determination)
} lqcMqttQueuedMsg_t;


/** 
 *  \brief typedef of MQTT subscription receiver function (required signature for appl callback).
*/
typedef void (*lqcRecv_func_t)(char *topic, char *props, char *message);


/** 
 *  \brief Struct the respresents a connection to the LooUQ Cloud - IoT Application Integration Platform.
*/
typedef struct lqCloudDevice_tag
{
    char deviceId[LQC_DEVICEID_SZ];                         ///< LQ Cloud device ID, generally expressed as a GUID, but can be any unique string value like a IMEI or ICCID.
    char deviceShortName[LQC_SHORTNAME_SZ];                 ///< device short name for local display
    char networkType[30];                                   ///<
    char networkName[20];                                   ///<
    char iothubAddr[IOTHUB_URL_SZ];                         ///< URL of the configured LooUQ Cloud ingress point (Az IoTHub)
    char sasToken[IOTHUB_SASTOKEN_SZ];                      ///< Device access secret
    char applKey[LQC_APPLKEY_SZ];                           ///< Optional action key, set on user's subscription
    lqcConnectMode_t connectMode;                           ///< Device to Cloud connection mode: OnDemand, Continuous, Required.
    lqcConnectState_t connectState;                         ///< Device to Cloud connection state: Closed, Open, Connected, Subscribed
    lqcResetCause_t resetCause;                             ///< Reset cause for last system start, enum is same as Microchip SAMD51 (superset of SAMD21)
    uint16_t msgNm;                                         ///< LQ Cloud client message number, incr each send. Note: independent from MQTT message ID and IOTHUB mId property.
    uint8_t onDemandTimeoutMinutes;                         ///< The time the communications channel stays open for onDemand mode.
    lqcMqttQueuedMsg_t *queuedMsgs[MQTTSEND_QUEUEDMSG_MAX]; ///< Outgoing messages, only queue if initial send fails
    lqcApplAction_t applActions[LQCACTN_CNT];               ///< Application invokable public methods (registered with LQ Cloud). LQ Cloud validates requests prior to messaging device.
    char actnMsgId[IOTHUB_MESSAGEID_SZ];                    ///< Action request mId, will be aCId (correlation ID).
    char actnName[LQCACTN_NAME_SZ];                         ///< Last action requested by cloud. Is reset on action request receive.
    uint16_t actnResult;                                    ///< Action result code for last action request. 
    millisTime_t sendLastAt;                                ///< Millis count for last send operation.
    uint8_t sendFaultType;                                  ///< If send D2C msg fails, message type. 
    lqcAppNotification_func appNotification_func;           ///< (optional) Application notification callback: LQCloud notifies application of status events (connection, disconnect, etc.).
    pwrStatus_func powerStatus_func;                        ///< (optional) Callback for LQCloud to get power status. 
    battStatus_func batteryStatus_func;                     ///< (optional) Callback for LQCloud to get battery status. 
    memStatus_func memoryStatus_func;                       ///< (optional) Callback for LQCloud to get memory status.
    ntwkSignal_func ntwkSignal_func;                        ///< (optional) Callback for LQCloud to get network signal level\quality status.
} lqCloudDevice_t;


/* LQCloud Internal Use Functions LQC_ prefix
* These are not defined static because they are used across compilation units.
------------------------------------------------------------------------------------------------ */

uint8_t LQC_connect();
void LQC_mqttSender(const char *eventType, const char *summary, const char *topic, const char *body);
void LQC_faultHandler(const char *faultMsg);
void LQC_appNotify(lqcAppNotification_t notificationType, const char *notificationMsg);

// alerts
void LQC_sendDeviceStarted(uint8_t startType);

// actions
void LQC_processActionRequest(const char *actnName, keyValueDict_t actnParams, const char *msgBody);

#endif  /* !__LQC_INTERNAL_H__ */
