//	Copyright (c) 2020 LooUQ Incorporated.
//	Licensed under The MIT License. See LICENSE in the root directory.

#define _DEBUG
#include "lqc_internal.h"

// debugging output options             UNCOMMENT one of the next two lines to direct debug (PRINTF) output
#include <jlinkRtt.h>                   // output debug PRINTF macros to J-Link RTT channel
// #define SERIAL_OPT 1                    // enable serial port comm with devl host (1=force ready test)

extern lqCloudDevice_t g_lqCloud;

#define MIN(x, y) (((x)<(y)) ? (x):(y))


/* Static Local Functions
------------------------------------------------------------------------------------------------ */
static bool sendAlert(lqcEventClass_t evntClass, const char *evntName, const char *evntSummary, const char *message);


/* LooUQ Cloud Alerts
================================================================================================ */

/* As displayed in Azure IoT Explorer telemetry view
{
  "body": {
    "dvStart": {
      "dvcInfo": {
        "dId": "afdba032-138d-4cba-8218-6bf7f42b28e2",
        "codeVer": "LooUQ-Cloud MQTT v1.1",
        "msgVer": "1.0"
      },
      "ntwkInfo": {
        "ntwkType": "<network type>",
        "ntwkDetail": "<network info>"
      }
    }
  },
  "enqueuedTime": "2020-08-01T17:38:18.046Z",
  "properties": {
    "mId": "~0",
    "mV": "1.0",
    "mTyp": "alrt",
    "evC": "cloud",
    "evN": "dvStart",
    "evV": "afdba032-138d-4cba-8218-6bf7f42b28e2"
  }
}
*/


/**
 *	\brief Send alert message to LooUQ Cloud.
 * 
 *  \param [in] evntName - Descriptive name for this type of alert.
 *  \param [in] evntSummary - Brief description of the event raising alert.
 *  \param [in] message - JSON formatted message body: must be JSON Property, JSON Object, or JSON Array
 */
bool lqc_sendAlert(const char *alrtName, const char *alrtSummary, const char *bodyJson)
{
    return sendAlert(lqcEventClass_application, alrtName, alrtSummary, bodyJson);
}



/* LooUQ Cloud Internal Alert Functions: not targetted for end-user application use.
================================================================================================ */


#pragma region LQCloud Internal

/**
 *	\brief Notify LQCloud device started (or recovered from off-line state).
 * 
 *  \param [in] startType - Enum describing the device start conditions.
 */
void LQC_sendDeviceStarted(uint8_t rCause)
{
    char summary[LQC_EVENT_SUMMARY_SZ] = {0};
    char body[LQC_EVENT_BODY_SZ] = {0};
    char restartDescr[18] = {0};

    // summary is a simple C-string, body is a string formatted as a JSON object
    snprintf(summary, LQC_EVENT_SUMMARY_SZ, "DeviceStart:%s (%d)",
        g_lqCloud.deviceId, rCause);
    snprintf(body, LQC_EVENT_BODY_SZ, "{\"dvcInfo\":{\"dId\":\"%s\",\"rCause\":%d,\"codeVer\":\"LooUQ-CloudMQTTv1.1\",\"msgVer\":\"1.0\"},\"ntwkInfo\":{\"ntwkType\":\"%s\",\"ntwkDetail\":\"%s\"}}", \
        g_lqCloud.deviceId, g_lqCloud.diagnostics.resetCause, g_lqCloud.networkType, g_lqCloud.networkName);

    sendAlert(lqcEventClass_lqcloud, "dStart", summary, body);
}


static bool sendAlert(lqcEventClass_t alrtClass, const char *alrtName, const char *alrtSummary, const char *bodyJson)
{
    char mqttName[LQC_EVENT_NAME_SZ] = {0};
    char mqttSummary[LQC_EVENT_SUMMARY_SZ] = {0};
    char mqttTopic[TOPIC_SZ];
    char mqttBody[LQC_EVENT_BODY_SZ]; 
    char eventClass[5];

    strncpy(eventClass, (alrtClass == lqcEventClass_application) ? "appl":"lqc", 5);
    
    if (alrtName[0] == ASCII_cNULL)
        strncpy(mqttName, "alert", 9);
    else
        strncpy(mqttName, alrtName, MIN(strlen(alrtName), LQC_EVENT_NAME_SZ-1));

    if (alrtSummary[0] != ASCII_cNULL)
        snprintf(mqttSummary, LQC_EVENT_SUMMARY_SZ, "\"descr\": \"%s\",", alrtSummary);

    // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=alrt&evC=%s&evN=%s"
    snprintf(mqttTopic, MQTT_TOPIC_SZ, MQTT_MSG_D2CTOPIC_ALERT_TMPLT, g_lqCloud.deviceId, g_lqCloud.msgNm++, eventClass, mqttName);
    snprintf(mqttBody, MQTT_MESSAGE_SZ, "{%s\"alert\": %s}", mqttSummary, bodyJson);
    return LQC_mqttTrySend(mqttTopic, mqttBody, false);
}


#pragma endregion
