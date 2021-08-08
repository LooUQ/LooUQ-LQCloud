/******************************************************************************
 *  \file lqc-alerts.c
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
 * LooUQ LQCloud Client Alert Services
 *****************************************************************************/

#define _DEBUG 2                        // set to non-zero value for PRINTF debugging output, 
// debugging output options             // LTEm1c will satisfy PRINTF references with empty definition if not already resolved
#if defined(_DEBUG)
    asm(".global _printf_float");       // forces build to link in float support for printf
    #if _DEBUG == 2
    #include <jlinkRtt.h>               // output debug PRINTF macros to J-Link RTT channel
    #define PRINTF(c_,f_,__VA_ARGS__...) do { rtt_printf(c_, (f_), ## __VA_ARGS__); } while(0)
    #else
    #define SERIAL_DBG _DEBUG           // enable serial port output using devl host platform serial, _DEBUG 0=start immediately, 1=wait for port
    #endif
#else
#define PRINTF(c_, f_, ...) ;
#endif

#include "lqc-internal.h"
#include "lqc-azure.h"

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
    return LQC_sendAlert(lqcEventClass_application, alrtName, alrtSummary, bodyJson);
}


void lqc_sendDiagAlert(const char *deviceName, const char *diagBody)
{
    lqc_sendAlert("lqDiagnostics", deviceName, diagBody);
}


/* LooUQ Cloud Internal Alert Functions: not targetted for end-user application use.
================================================================================================ */

#pragma region LQCloud Internal

/**
 *	\brief Notify LQCloud device started (or recovered from off-line state).
 * 
 *  \param [in] startType - Enum describing the device start conditions.
 */
void LQC_sendDeviceStarted(uint8_t rcause)
{
    char summary[lqc__event_summarySz] = {0};
    char body[lqc__event_bodySz] = {0};
    char restartDescr[18] = {0};

    // summary is a simple C-string, body is a string formatted as a JSON object
    snprintf(summary, sizeof(summary), "DeviceStart:%s (%d)",g_lqCloud.deviceId, rcause);
    snprintf(body, 
             sizeof(body), 
             "{\"dvcInfo\":{\"dId\":\"%s\",\"rCause\":%d,\"codeVer\":\"LooUQ-CloudMQTTv1.1\",\"msgVer\":\"1.0\"},\"ntwkInfo\":{\"ntwkType\":\"%s\",\"ntwkDetail\":\"%s\"}}",
             g_lqCloud.deviceId, 
             rcause, 
             g_lqCloud.networkType, 
             g_lqCloud.networkName);

    LQC_sendAlert(lqcEventClass_lqcloud, "dStart", summary, body);
}


bool LQC_sendAlert(lqcEventClass_t alrtClass, const char *alrtName, const char *alrtSummary, const char *bodyJson)
{
    char msgEvntName[lqc__event_nameSz] = {0};
    char msgEvntSummary[lqc__event_summarySz] = {0};
    char msgTopic[LQMQ_TOPIC_PUB_MAXSZ];
    char msgBody[LQMQ_MSG_MAXSZ]; 
    char eventClass[5];

    strncpy(eventClass, (alrtClass == lqcEventClass_application) ? "appl":"lqc", 5);
    
    if (alrtName[0] == '\0')
        strncpy(msgEvntName, "alert", 9);
    else
        strncpy(msgEvntName, alrtName, MIN(strlen(alrtName), sizeof(msgEvntName)-1));

    if (alrtSummary[0] != '\0')
        snprintf(msgEvntSummary, sizeof(msgEvntSummary), "\"descr\": \"%s\",", alrtSummary);

    // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=alrt&evC=%s&evN=%s"
    snprintf(msgTopic, LQMQ_TOPIC_PUB_MAXSZ, IotHubTemplate_D2C_topicAlert, g_lqCloud.deviceId, g_lqCloud.msgNm++, eventClass, msgEvntName);
    snprintf(msgBody, LQMQ_MSG_MAXSZ, "{%s\"alert\": %s}", msgEvntSummary, bodyJson);
    return LQC_mqttTrySend(msgTopic, msgBody, false);
}


#pragma endregion
