/******************************************************************************
 *  \file LQCloud-Static.ino
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2020-2022 LooUQ Incorporated.
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
 * Perform a continous test of a LooUQ LQCloud instance. 
 *
 * This is a LQCloud baseline functions test. See LQCloudTestAdv
 * for additional features on the Feather M0 including flash storage of 
 * settings, watchdog timer, and SAMD diagnostics send to cloud.
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

// specify the host board pin configuration
#define HOST_FEATHER_UXPLOR_L           // original legacy Feather UXplor
// #define HOST_FEATHER_UXPLOR          // Feather UXplor

// #define WATCHDOG                     // MCU watchdog enable
// #define WD_FAULT_TEST 3              // simulate watchdog fault after # of loop() iterations


#include <lq-collections.h>             // using LooUQ collections with mqtt
#include <lq-str.h>
#include <lq-diagnostics.h>
#include <lq-SAMDutil.h>

#include <ltemc.h>
#include <ltemc-tls.h>                  // LQCloud requires MQTT over TLS v1.2
#include <ltemc-mqtt.h>

#include <lqcloud.h>                    // add LQCloud
#include <lq-wrkTime.h>                 // use the wrkTime schedule helper


// network
#define PDP_DATA_CONTEXT 1
#define PDP_APN_NAME "iot.aer.net"

// validationKey is used to validate configuration, binary files and C2D (cloud-to-device) action commands. 
#define VALIDATION_KEY "1234567890abcdef"

#define TESTTARGET_PRODUCTION 
#ifdef TESTTARGET_PRODUCTION
    const char *target = "Production";
    const char *sasToken = "HostName=iothub-a-prod-loouq.azure-devices.net;DeviceId=869084063117729;SharedAccessSignature=SharedAccessSignature sr=iothub-a-prod-loouq.azure-devices.net%2Fdevices%2F869084063117729&sig=PbngiKMizBOijcHCn9jiGICv66ftKbPFuwiJnLHMGpw%3D&se=1672178109"; 
#else
    const char *target = "Development";
    const char *sasToken = ""; 
#endif

/* This is a basic test\example to run with the LooUQ LQCloud platform. If you do not have a LQCloud subscription you can get a free 
 * account with support for up to 5 devices and 12 months of uninterrupted service. Your account doesn't lapse but the access tokens
 * for your devices with need to be renewed every year (trial account SAS tokens have a 13 month lifetime).
 * 
 * LQCloud needs 3 provisioning elements: 
 *   -- The hub address, if you are using LQ Cloud for testing the MQTT_IOTHUB address below is valid. Otherwise supply your MQTT access point (no protocol prefix)
 *   -- A deviceId and 
 *   -- A SAS token (a time-limited password). For deviceId we are using the modem's IMEI
 * 
 * The Device ID is up to 40 characters long and using the modem's IMEI the device ID is typically 15 characters. An example of the SAS token is shown on the next line.  
 * 
 * "SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2Fe8fdd7df-2ca2-4b64-95de-031c6b199299&sig=XbjrqvX4kQXOTefJIaw86jRhfkv1UMJwK%2FFDiWmfqFU%3D&se=1759244275"
 *
 * You will obtain a SAS Token like above from the LQCloud-Manage web application. Under devices, you can get a SAS token from the list or the device details page.
*/


// test setup
uint16_t loopCnt = 0;
wrkTime_t telemetry_intvl;
wrkTime_t alert_intvl;

// LTEm variables
mqttCtrl_t mqttCtrl;                        // MQTT control, data to manage MQTT connection to server
uint8_t receiveBuffer[640];                 // Data buffer where received information is returned (can be local, global, or dynamic... your call)
char mqttTopic[200];                        // application buffer to craft TX MQTT topic
char mqttMessage[200];                      // application buffer to craft TX MQTT publish content (body)
resultCode_t result;


#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define PERIOD_FROM_SECONDS(period)  (period * 1000)
#define PERIOD_FROM_MINUTES(period)  (period * 1000 * 60)
#define STRCMP(x,y) (strcmp(x, y) == 0)
#define STREMPTY(s) (s != NULL && s[0] != '\0')


lqcDeviceConfig_t lqcDeviceConfig;          // device configuration (local)
providerInfo_t *provider;

#define ENABLE_NEOPIXEL
#ifdef ENABLE_NEOPIXEL
    #define NEO_PIN 8
    #define NEO_BRIGHTNESS 10                   // brightness: 0-255: Note these things are bright, so <10 is visible
    #include <FastLED.h>
    CRGB pixels[1];
    #define SHOW_PIXEL(color) pixels[0] = (color); FastLED.show();
#endif


/* LooUQ uses this device app to continuous test LQCloud instances. 
 * ------------------------------------------------------------------------------------------------
 * To support both device originating event (device-to-cloud) and remote device actions 
 * (cloud-to-device) this example uses a LooUQ LTEm-UXplor Feather board with a momentary switch
 * and a LED. These allow for simulation of a device event and a remote action.
 =============================================================================================== */
const int ledPin = 5;
const int buttonPin = 9;                    // double duty with battery sense, see getBatteryVoltage()
const int debouncePeriod = 10;
const int onboardLed = 13;

/* CloudTest EXAMPLE action doLedFlash: has a worker function to do the work without blocking
 * and a counter shared by the "start" function and the "do work" function. */
wrkTime_t ledFlash_intvl;
uint8_t flashesRemaining;


/* setup() 
 * --------------------------------------------------------------------------------------------- */
void setup() 
{
    #ifdef USE_SERIAL
        Serial.begin(115200);
        #if (USE_SERIAL)
        while (!Serial) {}
        #else
        delay(1000);
        #endif
    #endif

    uint8_t resetCause = lqSAMD_getResetCause();
    PRINTF(dbgColor__red, "\rLQCloud-Test %s\r", target);

    lqDiag_setNotifyCallback(applEvntNotifyCB);                 // the LQ ASSERT\Diagnostics subsystem can report a fault back to app for local display (aka RED light)
    lqDiag_setResetCause(resetCause);
    PRINTF(dbgColor__none,"RCause=%d \r", resetCause);

    /* Set for initial display states */
    #ifdef ENABLE_NEOPIXEL
        FastLED.addLeds<NEOPIXEL, 8>(pixels, 1);
        FastLED.setBrightness(40);
        SHOW_PIXEL(CRGB::Magenta);
    #endif

    /* Create connectivity interface (LTEm modem)
     */
    ltem_create(ltem_pinConfig, yieldCB, applEvntNotifyCB);
    ltem_setDefaultNetwork(PDP_DATA_CONTEXT, PDP_PROTOCOL_IPV4, PDP_APN_NAME);
    mqtt_initControl(&mqttCtrl, dataCntxt_0, receiveBuffer, sizeof(receiveBuffer), mqttRecv);
    lqc_create(lqcDeviceType_ctrllr, &lqcDeviceConfig, cloudTrySend, applEvntNotifyCB, applInfoRequestCB, yieldCB, VALIDATION_KEY);
    lqc_enableDiagnostics(lqDiag_getDiagnosticsBlock());

    /*
     * Create and Initialize Complete.
     * Warm up network and start CloudMelt local processing
     * ------------------------------------------------------------------------------------------*/
    
    if (startNetwork(true))                                         // warmup network with display status
        PRINTF(dbgColor__info, "Network started.\r");
    else
        PRINTF(dbgColor__warn, "Network not available!\r");

    lqc_start(resetCause);                                          // start LQCloud (network may/may not be running)

    // resultCode_t gnssResult = gnss_on();
    // if (gnssResult != resultCode__success && gnssResult != 504)
    //     PRINTF(dbgColor__info, "GNSS ON: errResult=%d \r", gnssResult);

    /* Define signature of the cloud accessible device actions (commands) 
     * Utilizing pre-process automatic string concatenation */
    static const char *getStatus_params = { "" };
    static const char *setLedState_params = { "ledstate" "=" "int" };
    static const char *setLedFlash_params = { "flashcnt" "=" "int" "&" "cyclems" "=" "int" };

    lqc_registerApplicationAction("get-status", &LQCACTION_getStatus, getStatus_params);
    lqc_registerApplicationAction("set-ledstate", &LQCACTION_setLedState ,setLedState_params);
    lqc_registerApplicationAction("set-ledflash", &LQCACTION_setLedFlash ,setLedState_params);

    telemetry_intvl = wrkTime_create(PERIOD_FROM_SECONDS(60));
    alert_intvl = wrkTime_create(PERIOD_FROM_MINUTES(5));

    #ifdef WATCHDOG
    /* setup has already registered yield callback with LTEmC to handle longer running network actions 
     * now enable watchdog as app transitions to loop() */
    uint16_t wdtDuration = lqSAMD_wdEnable(0);                                                  // default = 16 seconds
    PRINTF(dbgColor__blue, "WDT Duration=%d\r\r", wdtDuration);
    #endif
}

/* loop() --------------------------------------------------------------------------------------- */
void loop() 
{
    #define SUMMARY_SZ 80
    #define BODY_SZ 100

    char summary[SUMMARY_SZ] = {0};
    char body[BODY_SZ] = {0};
    lqcSendResult_t sendRslt;
    loopCnt++;
    SHOW_PIXEL(CRGB::Blue);


    if (wrkTime_doNow(&telemetry_intvl))
    {
        snprintf(summary, SUMMARY_SZ, "LQCloud-Test telemetry loop=%d", loopCnt);
        strcpy(body, "LQCloud-Test telemetry");

        lqcSendResult_t sendRslt = lqc_sendTelemetry("LQC-Test-telemetry", summary, body);
        if (sendRslt == lqcSendResult_sent)
        {
            PRINTF(dbgColor__info, "telemetry sent\r");
            SHOW_PIXEL(CRGB::Green);
        }
        else
        {
            PRINTF(dbgColor__warn, "telemetry send failed result=%d\r", sendRslt);
            SHOW_PIXEL(CRGB::Yellow);
        }
    }

    if (wrkTime_doNow(&alert_intvl))
    {
        snprintf(summary, SUMMARY_SZ, "LQCloud-Test alert loop=%d", loopCnt);
        strcpy(body, "LQCloud-Test periodic alert");

        sendRslt = lqc_sendAlert("LQC-PeriodicAlert", summary, body);
        if (sendRslt == lqcSendResult_sent)
            PRINTF(dbgColor__info, "periodic alert sent\r");
        else
            PRINTF(dbgColor__warn, "periodic alert send failed result=%d\r", sendRslt);
    }


    /* Device Events
    */ 
    pinMode(buttonPin, INPUT);
    if (digitalRead(buttonPin) == LOW)
    {
        PRINTF(dbgColor__cyan, "Button=%d\r", digitalRead(buttonPin));
        delay(debouncePeriod);
        int bttn = digitalRead(buttonPin);
        if (digitalRead(buttonPin) == LOW)
        {
            PRINTF(dbgColor__info, "\rUser ReqstAlert Button Pressed\r");
            lqc_sendAlert("cloudTest-user-alert", "UserAlert", "\"User signaled\"");
        }
    }

    // Do LQ Cloud and application background tasks
    lqc_doWork();
    applWork_doFlashLed();
    pDelay(PERIOD_FROM_SECONDS(5));

    #ifdef WATCHDOG
        // IMPORTANT! pet the dog to keep dog happy and app alive
        // ---------------------------------------------------------
        #ifdef WD_FAULT_TEST
        if (loopCnt < WD_FAULT_TEST)
            Watchdog.reset();
        #else
        lqSAMD_wdReset();
        #endif
    #endif
}


#pragma region network
/*  Network Start() (or restart) and trySend()
 *  Start LTEm device to communicate with LQ provisioning service and get config packages
 * ------------------------------------------------------------------------------------------ */
bool startNetwork(bool displayStatusOLED)
{
    bool networkFault = false;

    /* Static lqcDeviceConfig for this test
     ------------------------------------------------------------------------------------------- */
    if (!STREMPTY(lqcDeviceConfig.deviceId))
    {
        #ifdef TESTTARGET_PRODUCTION
            strcpy(lqcDeviceConfig.deviceLabel, "lqcPROD");
            //strcpy(lqcDeviceConfig.hostUrl, "iothub-a-prod-loouq.azure-devices.net");
            //strcpy(lqcDeviceConfig.signature, "TBD");
        #else
            strcpy(lqcDeviceConfig.deviceLabel, "lqcDEV");
            //strcpy(lqcDeviceConfig.hostUrl, "iothub-dev-pelogicl.azure-devices.net");
            //strcpy(lqcDeviceConfig.signature, "TBD");
        #endif

        lqc_setDeviceConfigFromSASToken(sasToken, &lqcDeviceConfig);

        //strcpy(lqcDeviceConfig.magicFlag, PROVISION_MAGIC);
        //strcpy(lqcDeviceConfig.packageId, PROVISION_LQCCONFIG);
        //strcpy(lqcDeviceConfig.deviceId, mdminfo_ltem()->imei);
        //lqcDeviceConfig.hostPort = 8883;

    }

    /* Start modem and await network provider for short period of time
     */
    ltem_start(resetAction_swReset);                                                // start modem with soft-reset
    PRINTF(dbgColor__none, "BGx %s\r", mdminfo_ltem()->fwver);
    tls_configure(dataCntxt_0, tlsVersion_tls12, tlsCipher_any, tlsCertExpiration_default, tlsSecurityLevel_default);

    provider = ntwk_awaitProvider(PERIOD_FROM_SECONDS(15));
    if (strlen(provider->name) == 0)
    {
        networkFault = true;
    }
    else
    {
        PRINTF(dbgColor__info, "Connected to %s using %s, %d networks available.\r", provider->name, provider->iotMode, provider->networkCnt);
    }
    uint8_t tries = 0;
    while (mdmInfo_signalPercent() == 0 && tries < 20)                      // Quectel indication is CSQ takes a couple seconds to stablize and report
    {
        tries++;
        pDelay(500);                                                                   
    }
    PRINTF(dbgColor__cyan, "   Signal %%: %d  (%d)\r", mdmInfo_signalPercent(), tries);
    PRINTF(dbgColor__cyan, "Signal RSSI: %d\r", mdminfo_signalRSSI());


    /*  Local FLASH storage: read LQCloud device config, CM config, CM baselines
     * ------------------------------------------------------------------------------------------ */


    networkFault = !STRCMP(lqcDeviceConfig.packageId, "LQCC");                                      // Check LQCloud settings for expected pkg ID
    while (!networkFault)
    {
        char userId[101];
        char sasToken[201];
        lqc_composeIothUserId(userId, sizeof(userId), lqcDeviceConfig.hostUrl, lqcDeviceConfig.deviceId);
        lqc_composeIothSasToken(sasToken, sizeof(sasToken), lqcDeviceConfig.hostUrl, lqcDeviceConfig.deviceId, lqcDeviceConfig.signature);
        mqtt_setConnection(&mqttCtrl, lqcDeviceConfig.hostUrl, 8883, true, mqttVersion_311, lqcDeviceConfig.deviceId,  userId, sasToken);

        resultCode_t rslt = mqtt_open(&mqttCtrl);
        if (rslt != resultCode__success)
        {
            networkFault = true;
            PRINTF(dbgColor__warn, "Open fail status=%d\r", rslt);
            break;
        }
        PRINTF(dbgColor__dCyan, "MQTT Opened\r");

        rslt = mqtt_connect(&mqttCtrl, true);
        if (rslt != resultCode__success)
        {
            networkFault = true;
            PRINTF(dbgColor__warn, "Connect fail status=%d\r", rslt);
            break;
        }
        PRINTF(dbgColor__dCyan, "MQTT Connected\r");

        char recvTopic[80];
        snprintf(recvTopic, sizeof(recvTopic), "devices/%s/messages/devicebound/#", mqttCtrl.clientId);
        // at+qmtsub=0,1,"devices/864508030074113/messages/devicebound/#",1
        rslt = mqtt_subscribe(&mqttCtrl, recvTopic, mqttQos_1);
        if (rslt != resultCode__success)
        {
            networkFault = true;
            PRINTF(dbgColor__warn, "Subscribe fail status=%d\r", rslt);
            break;
        }
        PRINTF(dbgColor__dCyan, "MQTT Subscribed\r");
        break;
    }

    if (networkFault)
    {
        memset(&lqcDeviceConfig, 0, sizeof(lqcDeviceConfig_t));
    }   
    return !networkFault;
}


resultCode_t cloudTrySend(const char *topic, const char* message, uint8_t timeoutSec)
{
    resultCode_t pubResult = mqtt_publish(&mqttCtrl, topic, mqttQos_1, message, timeoutSec);
    if (pubResult != resultCode__success)
    {
        PRINTF(dbgColor__warn, "LQC Send Failed, Result=%d (%s)\r", pubResult, atcmd_getValue());

        if (startNetwork(false))
        {
            PRINTF(dbgColor__info, "Successfully Restarted Network.\r");
            // retry once and return whatever happens
            pubResult = mqtt_publish(&mqttCtrl, topic, mqttQos_1, message, timeoutSec);
        }
        else
            PRINTF(dbgColor__warn, "Failed to Restart Network.\r");
    }
    return pubResult;
}


void mqttRecv(dataCntxt_t dataCntxt, uint16_t msgId, const char *topic, char *topicVar, char *message, uint16_t messageSz)
{
    uint16_t newLen = lq_strUriDecode(topicVar, message - topicVar);        // Azure IoTHub URI encodes properties sent as topic suffix 

    PRINTF(dbgColor__info, "\r**MQTT--MSG** @tick=%d BufferSz=%d\r", pMillis(), mqtt_getLastBufferReqd(&mqttCtrl));
    PRINTF(dbgColor__cyan, "   msgId:=%d   topicSz=%d, propsSz=%d, messageSz=%d\r", msgId, strlen(topic), strlen(topicVar), strlen(message));
    PRINTF(dbgColor__cyan, "   topic: %s\r", topic);
    PRINTF(dbgColor__cyan, "   props: %s\r", topicVar);
    PRINTF(dbgColor__cyan, " message: %s\r", message);

    // Azure IoTHub appends properties collection to the topic 
    // That is why Azure requires wildcard topic
    keyValueDict_t mqttProps = lq_createQryStrDictionary(topicVar, strlen(topicVar));
    PRINTF(dbgColor__info, "Props(%d)\r", mqttProps.count);
    for (size_t i = 0; i < mqttProps.count; i++)
    {
        PRINTF(dbgColor__cyan, "%s=%s\r", mqttProps.keys[i], mqttProps.values[i]);
    }
    PRINTF(0, "\r");

    lqc_receiveMsg(message, messageSz, topic);
}


#pragma endregion


/* LQCloud Network Action Callbacks
========================================================================================================================= 
  Standard callbacks for LQCloud to start and stop network services. Here the LTEm1 (modem) is started and stopped. 
  If LQCloud repeated send retries it will utilize a stop/start sequence to reset the transport. These functions should
  be constructed to block until their function is complete before returning to LQCloud.

  Note: networkStart() returns bool indicating if the network started. Function should return true only if complete, i.e.
  when the network transport state is ready for communications. If returning false, LQC will retry as required.
========================================================================================================================= */

/* Application "other" Callbacks
========================================================================================================================= */
#define STRCMP(n, h)  (strcmp(n, h) == 0)

char infoRequestResponse[16] = {0};


void applEvntNotifyCB(const char *eventTag, const char *eventMsg)
{
    if (STRCMP(eventTag, "ASSERT"))
    {
        // local indication/display
        assert_brk();
        while (true) {}                                                         // should never get here
    }
    if (STRCMP(eventTag, "ASSERTW"))
    {
        // local indication/display
    }
}


const char *applInfoRequestCB(const char *infoRqst)
{
    if (STRCMP(infoRqst, "batt"))
    {
        uint16_t vbat = getBatteryVoltage();
        sprintf(infoRequestResponse, "batt=%d", vbat);
    }

    // TODO: power status, memory
    return infoRequestResponse;
}

void yieldCB()
{
    lqSAMD_wdReset();
}




/* Local environment state inquiry
 ----------------------------------------------------------------------------------------------- */

#define VBAT_ENABLED 
#define VBAT_PIN 9          // aka A7, which is not necessarily defined in variant.h
#define VBAT_DIVIDER 2
#define AD_SCALE 1023
#define AD_VREF 3300        // milliVolts

/* Note: A7 (aka pin digital 9) can do double duty with compatible functions. In this example
 * the pin is also a digital input sensing a button press. The voltage divider sensing the 
 * battery voltage (Adafruit Feather) keeps pin 9/A7 at about 2v (aka logic HIGH).
*/

/**
 *	\brief Get feather battery status 
 *  \return Current battery voltage in millivolts 
 */
uint16_t getBatteryVoltage()
{
    uint32_t vbatAnalog = 0;

    #ifdef VBAT_ENABLED
        vbatAnalog = analogRead(VBAT_PIN);
        // detach pin from analog mux, req'd for shared digital input use
        pinMode(VBAT_PIN, INPUT);
        // vbatAnalog *= (VBAT_DIVIDER * AD_VREF / AD_SCALE);
        vbatAnalog = vbatAnalog * VBAT_DIVIDER * AD_VREF / AD_SCALE;

    #endif
    return vbatAnalog;
}


/* Check free memory SAMD tested (stack-to-heap) 
 * ---------------------------------------------------------------------------------
*/
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C"  char *sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

uint32_t getFreeMemory()
{
    char top;
    #ifdef __arm__
    return &top - reinterpret_cast<char*>(sbrk(0));
    #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    return &top - __brkval;
    #else  // __arm__
    return __brkval ? &top - __brkval : &top - __malloc_heap_start;
    #endif  // __arm__
}



/* Application Actions
========================================================================================================================= */

/* See registration at end of setup() */

static void LQCACTION_getStatus(keyValueDict_t params)
{
    PRINTF(dbgColor__cyan, "CM-RMT_getStatus: ");

    const char *state = "off";
    double loadI = 9.9;

    char body[400] = {0};
    const char *bodyTmplt = "{\"getStatus\":{\"dId\":\"%s\",\"ledState\":\"%s\"}}";
    snprintf(body, sizeof(body), bodyTmplt , lqcDeviceConfig.deviceId, !digitalRead(ledPin));

    lqc_sendActionResponse(resultCode__success, body);
}


static void LQCACTION_setLedState(keyValueDict_t params)
{
    #define KVALUESZ 15
    char kValue[KVALUESZ] = {0};

    lq_getQryStrDictionaryValue("ledState", params, kValue, KVALUESZ);
    PRINTF(dbgColor__cyan, "setledState: %s\r", kValue);

    if (kValue[0] == '1')
        digitalWrite(ledPin, LOW);  // ON
    else
        digitalWrite(ledPin, HIGH); // otherwise OFF

    lqc_sendActionResponse(resultCode__success, "{}");
    return;
}


    /* Using a wrkSched (work schedule) object to handle needed "flash" delays without blocking delay()
     * WorkSchedule properties enabled and state are for application consumer use. Here we are using
     * state to count up flashes until request serviced. Clearing it back to 0 when complete.
     * When triggered this function uses workSched_reset to start the interval in sync with request.
     * 
     * Note: this function pattern doesn't prevent duplicate triggers. A 2nd trigger while a flash 
     * sequence is running will be mo
    */


/**
 *	\brief The doFlashLed is the function to kick off the LED flashing, it is invoked by the LC Cloud Actions dispatcher. 
 * 
 *  See applWork_doFlashLed() below for the rest of the story. 
 */
static void LQCACTION_setLedFlash(keyValueDict_t params)
{
    uint8_t flashes;
    uint16_t cycleMillis;

    #define DICTIONARY_VALUE_CSTRING_SZ 12
    char kvalue[DICTIONARY_VALUE_CSTRING_SZ];

    lq_getQryStrDictionaryValue("flashCount", params, kvalue, DICTIONARY_VALUE_CSTRING_SZ);
    flashes = atoi(kvalue);

    if (flashes = 0 || flashes > 10)
        lqc_sendActionResponse(resultCode__badRequest, "Requested flash count must be between 1 and 10.");

    lq_getQryStrDictionaryValue("cycleMillis", params, kvalue, DICTIONARY_VALUE_CSTRING_SZ);
    cycleMillis = atoi(kvalue);

    /* If cycleMillis * flashes is too long LQ Cloud will timeout action request, to prevent that we limit total time
    * here. For long running LQ Cloud actions: start process, return status of action start, and use a separate Action
    * to test for action process still running or complete. Alternate pattern: use alert webhook callback on process end.
    */

    uint16_t cycleDuration = flashes * 2 * cycleMillis;
    if (cycleDuration > 5000)
    {
        PRINTF(dbgColor__cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
        lqc_sendActionResponse(resultCode__badRequest, "Flashing event is too long, 5 seconds max.");
    }

    if (flashesRemaining != 0)                                  
        lqc_sendActionResponse(resultCode__conflict, "Flash process is already busy flashing.");
    else
    {
        ledFlash_intvl = wrkTime_create(cycleMillis);
        flashesRemaining = flashes;                     // this will count down, 0 = complete
    }

    PRINTF(dbgColor__cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
    // the "flash LED" process is now started, return to the main loop and handle the flashing in applWork_doFlashLed() below.
}


/**
 *	\brief The applWork_doFlashLed is the pseudo-background "worker" function process the LED flashing until request is complete.
 * 
 *  The applWork_doFlashLed is the background worker that continues the process until complete. This function's invoke is in standard loop().
 *  applWork_* is just a convention to make the background process easier to identify.
 * 
 *  NOTE: In the wrkSched object you can extend it to contain information for your needs at a potential cost of memory. You could create a 
 *  .flashes or .state property for this case that is a count down to completion.
 */
void applWork_doFlashLed()
{
    // no loop here, do some work and leave

    if (flashesRemaining > 0 && wrkTime_doNow(&ledFlash_intvl))         // put state test first to short-circuit evaluation
    {
        if (flashesRemaining % 2 == 0)
            digitalWrite(ledPin, LOW);      // ON
        else
            digitalWrite(ledPin, HIGH);

        flashesRemaining--;             // count down

        if (flashesRemaining == 0)
        {
            PRINTF(dbgColor__cyan, "doLampFlash Completed\r");
            lqc_sendActionResponse(resultCode__success, "");   // flash action completed sucessfully
        }
    }
}
