/******************************************************************************
 *  \file lqc-azure.h
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
 * LooUQ LQCloud Azure Support Services
 *****************************************************************************/

#ifndef __LQC_AZURE_H__
#define __LQC_AZURE_H__

#include "lqc-internal.h"


#pragma region AZURE IoTHub Templates
/* ----------------------------------------------------------------------------------------------- */

/* C2D Receive Topic (subscribe) Template
 * %s = device ID
*/
#define IOTHUB_C2D_RECVTOPIC_TMPLT "devices/%s/messages/devicebound/#"
static const char *IoTHubTemplate_C2D_recvTopic = "devices/%s/messages/devicebound/#";


/* UserID Template
 * %s = iothub URL
 * %s = device ID
*/
#define IOTHUB_USERID_TMPLT "%s/%s/?api-version=2018-06-30" 
static const char *IotHubTemplate_sas_userId = "%s/%s/?api-version=2018-06-30";


/* Message Properties Template
 * %s = dId : device ID
 * %d = mId : msg ID
 * %s = evT : event\message type (tdat, alrt, etc.)
 * %s = cId : correlation ID (message ID from action request)
 * %s = evC : event class ()
 * %s = evN : event name
*/
#define IOTHUB_MSG_D2CTOPIC_TELEMETRY_TMPLT "devices/%s/messages/events/mId=~%d&mV=1.0&evT=tdat&evC=%s&evN=%s"
#define IOTHUB_MSG_D2CTOPIC_ALERT_TMPLT "devices/%s/messages/events/mId=~%d&mV=1.0&evT=alrt&evC=%s&evN=%s"
#define IOTHUB_MSG_D2CTOPIC_ACTIONRESP_TMPLT "devices/%s/messages/events/mId=~%d&mV=1.0&evT=aRsp&aCId=%s&evC=%s&evN=%s&aRslt=%d"

static const char *IotHubTemplate_D2C_topicTelemetry = "devices/%s/messages/events/mId=~%d&mV=1.0&evT=tdat&evC=%s&evN=%s";
static const char *IotHubTemplate_D2C_topicAlert = "devices/%s/messages/events/mId=~%d&mV=1.0&evT=alrt&evC=%s&evN=%s";
static const char *IotHubTemplate_D2C_topicActionResponse = "devices/%s/messages/events/mId=~%d&mV=1.0&evT=aRsp&aCId=%s&evC=%s&evN=%s&aRslt=%d";

#pragma endregion




// #ifdef __cplusplus
// extern "C"
// {
// #endif

// void lqc_composeTokenSas(char *sasToken, uint8_t sasSz, const char* hostUri, const char *deviceId, const char* sigExpiry);
// lqcDeviceConfig_t lqc_decomposeTokenSas(const char* sasToken);

// #ifdef __cplusplus
// }
// #endif // !__cplusplus

#endif  /* !__LQC_AZURE_H__ */
