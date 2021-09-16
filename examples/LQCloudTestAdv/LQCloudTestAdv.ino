/******************************************************************************
 *  \file LQCloudTestAdv.ino
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
 * Perform a continous test of a LooUQ LQCloud instance. 
 * Demonstrates a typical LTEmC (LTEm1 IoT modem) application for MQTT 
 * applications and a reference design of a LQCloud device application with 
 * telemetry, alerts and actions.
 * 
 * This is a LQCloud full featured functions test. See LQCloudTestBasic
 * for baseline LQCloud client on the Feather M0.
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

// define options for how to assemble this build
#define HOST_FEATHER_UXPLOR             // specify the pin configuration

#include <lq-collections.h>             // using LooUQ collections with mqtt
#include <lq-str.h>
#include <lq-diagnostics.h>
#include <lq-SAMDutil.h>

#include <ltemc.h>
#include <ltemc-tls.h>                  // LQCloud requires MQTT over TLS v1.2
#include <ltemc-mqtt.h>

#include <lqcloud.h>                    // add LQCloud
#include <lq-wrkTime.h>                 // use the wrkTime scheduler


#define DEFAULT_NETWORK_CONTEXT 1


/* This is a basic test\example to run with the LooUQ LQCloud platform. If you do not have a LQCloud subscription you can get a free 
 * trial account with support for up to 5 devices and 12 months of uninterrupted service (SAS tokens need to be renewed at 12 months).
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

// put your LQCloud SASToken here (yep, this one is expired)
#define LQCLOUD_TOKEN "SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2F867198053225169&sig=1iP6gpTw5%2BlsMGpWB%2BCUwY%2FWSLdGxYD0%2FlywxE5Yu5Y%3D&se=1631966544"
// organization key used to validate configuration and binary files and C2D (cloud-to-device) action commands
const char *orgKey = "B89B3C5768B4280E2CE49336FD4EC752DE43BE14E153A6523AD0D71AAC83B4DC";


// test setup
uint16_t loopCnt = 1;
wrkTime_t sendTelemetry_intvl;
wrkTime_t alert_intvl;
resultCode_t fresult;

// LTEm variables
mqttCtrl_t mqttCtrl;                        // MQTT control, data to manage MQTT connection to server
uint8_t receiveBuffer[640];                 // Data buffer where received information is returned (can be local, global, or dynamic... your call)
char mqttTopic[200];                        // application buffer to craft TX MQTT topic
char mqttMessage[200];                      // application buffer to craft TX MQTT publish content (body)


#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define PERIOD_FROM_SECONDS(period)  (period * 1000)
#define PERIOD_FROM_MINUTES(period)  (period * 1000 * 60)

#define ENABLE_NEOPIXEL
#ifdef ENABLE_NEOPIXEL
    #define NEO_PIN 8
    #define NEO_BRIGHTNESS 10                   // brightness: 0-255: Note these things are bright, so <10 is visible
    #include <FastLED.h>
    CRGB pixels[1];
#endif

// Configuration persistence with flash services
#include <SPI.h>                        
#include <Adafruit_SPIFlashBase.h>          // Adafruit SPI Flash extensions
#include <lq-persistStructs.h>              // LooUQ flash struct persistence
#include <lq-provisioning.h>                // Support for over-the-air update 

Adafruit_FlashTransport_SPI flashTransport(EXTERNAL_FLASH_USE_CS, EXTERNAL_FLASH_USE_SPI);
// #if defined(EXTERNAL_FLASH_USE_QSPI)
// Adafruit_FlashTransport_QSPI flashTransport;
// #elif defined(EXTERNAL_FLASH_USE_SPI)
// Adafruit_FlashTransport_SPI flashTransport(EXTERNAL_FLASH_USE_CS, EXTERNAL_FLASH_USE_SPI);
// #else
// #error No QSPI/SPI flash are defined on your board variant.h !
// #endif
Adafruit_SPIFlashBase flash(&flashTransport);
lq_flashDictionary flashDictionary(&flash);
lq_configPkgSrvc cnfgPkgSrvc("deviceservices-dev-pelogical.eastus.cloudapp.azure.com", 10100, &flashDictionary);
lqcDeviceConfig_t lqcDeviceConfigWr;
lqcDeviceConfig_t lqcDeviceConfigRd = {0};


/* LooUQ uses this device app to continuous test LQCloud instances. To support optional scenarios, we use the LTEm-UXplor
 * Feather board with a momentary switch and a LED to simulate a IoT device with a digital sensor and actuator          */
/* -------------------------------------------------------------------------------------------------------------------- */
const int ledPin = 5;
const int buttonPin = 9;                    // double duty with battery sense, see getBatteryVoltage()
const int debouncePeriod = 10;

/* CloudTest EXAMPLE action doLedFlash: has a worker function to do the work without blocking
 * and a counter shared by the "start" function and the "do work" function. */
wrkTime_t ledFlash_intvl;
uint8_t flashesRemaining;


#define LQCACTION_PARAM_INT() "int"

/* Define signature of the cloud accessible device actions (commands) 
 * Utilizing pre-process automatic string concatenation */
// #define LQCACTN_PARAMTYPE_INT    "int"
static const char *getStatus_params = "";
static const char *setLedState_params =   "ledState" "=" LQCACTION_PARAM_INT();
static const char *doFlashLed_params = { "flashCount" "=" LQCACTION_PARAM_INT() "&" "cycleMillis" "=" LQCACTION_PARAM_INT() };


/* setup() 
 * --------------------------------------------------------------------------------------------- */
void setup() {
    #ifdef USE_SERIAL
        Serial.begin(115200);
        #if (USE_SERIAL)
        while (!Serial) {}
        #else
        delay(1000);
        #endif
    #endif

    PRINTF(dbgColor__red, "LooUQ Cloud - CloudTest\r");
    lqDiag_registerNotifCallback(eventNotifCB);               // the LQ ASSERT\Diagnostics subsystem can report a fault back to app for local display (aka RED light)
    lqDiag_setResetCause(lqSAMD_getResetCause());
    PRINTF(dbgColor__none, "RCause=%d\r", lqSAMD_getResetCause());

    #ifdef ENABLE_NEOPIXEL
    FastLED.addLeds<NEOPIXEL, 8>(pixels, 1);
    FastLED.setBrightness(10);
    pixels[0] = CRGB::Blue; 
    FastLED.show();
    #ifdef NEOPIXEL_TEST
        pixels[0] = CRGB::Green; 
        FastLED.show();
        pixels[0] = CRGB::Magenta; 
        FastLED.show();
        pixels[0] = CRGB::Red; 
        FastLED.show();
    #endif
    #endif

    if (!flash.begin())                                     // Initialize flash library and check flash chip ID.
    {
        PRINTF(dbgColor__error, "Error, failed to initialize flash chip!");
        while(1) yield();
    }
    PRINTF(dbgColor__blue,"Flash chip JEDEC ID: %x\r", flash.getJEDECID());

    /* Do a round-trip through flashDictionary for testing purposes */
    // flashDictionary.eraseAll();                                                                     // erase FLASH
    // lqcDeviceConfigWr = lqc_decomposeTokenSas(LQCLOUD_TOKEN);                                       // write config to FLASH
    // fresult = flashDictionary.write(201, &lqcDeviceConfigWr, sizeof(lqcDeviceConfig_t), true);      // lqcDeviceConfigWr = lqc_decomposeTokenSas(LQCLOUD_TOKEN);

    fresult = flashDictionary.readByKey(201, &lqcDeviceConfigRd, sizeof(lqcDeviceConfig_t));        // read config from FLASH
    char lqcToken[180];
    lqc_composeTokenSas(lqcToken, sizeof(lqcToken), lqcDeviceConfigRd.hostUri, lqcDeviceConfigRd.deviceId, lqcDeviceConfigRd.tokenSigExpiry);

    // device application setup and initialization
    pinMode(buttonPin, INPUT);                              // feather uses pin=9 for battery voltage divider, floats at ~2v
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);                             // LED off

    ltem_create(ltem_pinConfig, eventNotifCB);              // create modem\transport reference and wire-up device event callbacks
    ltem_setYieldCallback(resetWatchdog);
    ltem_start();
    PRINTF(dbgColor__none, "BGx %s\r", lqDiag_setHwVersion(mdminfo_ltem().fwver));

    /* Add LQCloud to project and register local service callbacks.
     * ----------------------------------------------------------------------------------------- */
    lqc_create(orgKey, "LQC-DEV", eventNotifCB, networkStart, networkStop, NULL, getBatteryStatus, getFreeMemory);
    /* Callbacks (param 3-8)
     * - cloudNotificationsHandler: cloud informs app of events
     * - networkStart: calls back to application to perform the steps required to start the network transport
     * - networkStop: calls back to application to perform the steps required to STOP the network transport
     * - getUxpPwrStatus: bool indicating external power source online
     * - getBatteryStatus: enum of battery power reserve
     * - getFreeMemory: how much memory between stack and heap. Even without malloc, does the stack have room
    */

    /* Define settings for the MQTT (over TLS) connection to LQCloud, TLS v1.2 and MQTT v3.11 required 
     * Even though LQCloud will manage receives, the receive buffer has to be defined and provided based on
     * your application's needs. You can start larger, then use mqtt_getLastBufferReqd(mqttCtrl_t *mqttCtrl)
     * to see if you can reduce buffer size */
    tls_configure(dataContext_0, tlsVersion_tls12, tlsCipher_any, tlsCertExpiration_default, tlsSecurityLevel_default);
    mqtt_initControl(&mqttCtrl, dataContext_0, mqtt__useTls, mqttVersion_311, receiveBuffer, sizeof(receiveBuffer), NULL);

    lqc_start(&mqttCtrl, lqcToken);
    PRINTF(dbgColor__info, "Connected to LQ Cloud, Device started.\r");

    lqc_diagnosticsCheck(lqDiag_getDiagnosticsInfo());                  // invoke diagnostics reporter to analyze device start conditions and optionally send diagnostics to cloud/host

    sendTelemetry_intvl = wrkTime_create(PERIOD_FROM_SECONDS(30));      // this application sends telemetry continuously every 30 secs
    alert_intvl = wrkTime_create(PERIOD_FROM_MINUTES(60));              // and a periodic alert every 60 min

    lqc_regApplAction("get-stat", &getStatus, getStatus_params);        // register application cloud-to-device commands (callbacks)
    lqc_regApplAction("set-led", &setLedState, setLedState_params);
    lqc_regApplAction("do-flashLed", &doFlashLed, doFlashLed_params);

    /* now enable watchdog as app transitions to loop()
     * setup has already registered yield callback with LTEm1c to handle longer running network actions */

    uint16_t wdtDuration = lqSAMD_wdEnable(0);                          // default = 16 seconds
    PRINTF(dbgColor__blue, "WDT Duration=%d\r\r", wdtDuration);

    // status: init completed.
    #ifdef ENABLE_NEOPIXEL
    pixels[0] = CRGB::Green; 
    FastLED.show();
    #endif
}


/* loop() ------------------------------------------------------------------------------------------------------------- */
bool lastSendSuccessful = true;

void loop() 
{
    if (wrkTime_doNow(&alert_intvl))
    {
        char alrtSummary[40];
        snprintf(alrtSummary, 40, "Periodic Alert, Loop=%d", loopCnt);
        PRINTF(dbgColor__magenta, "Sending periodic alert.\r");
        lqc_sendAlert("cloudTest-periodic-alert", "PeriodicAlert", alrtSummary);
    }
    if (wrkTime_doNow(&sendTelemetry_intvl))
    {
        PRINTF(dbgColor__dMagenta, "\rLoop=%i>>\r", loopCnt);

        char summary[80] = {0};
        char body[400] = {0};
        snprintf(summary, sizeof(summary), "Test Telemetry, Loop=%d", loopCnt);
        snprintf(body, sizeof(body), "{\"loopCnt\": %d}" , loopCnt);
        bool thisSend = lqc_sendTelemetry("LQ CloudTest", summary, body);

        if (!thisSend && !lastSendSuccessful)
             eventNotifCB(lqNotifType_hardFault, 0, 0, "Successive send errors");
        lastSendSuccessful = thisSend;
        loopCnt++;
    }

    if (digitalRead(buttonPin) == LOW)
    {
        delay(debouncePeriod);
        int bttn = digitalRead(buttonPin);
        if (digitalRead(buttonPin) == LOW)
        {
            PRINTF(dbgColor__info, "\rUser ReqstAlert Button Pressed\r");
            lqc_sendAlert("cloudTest-user-alert", "UserAlert", "\"User signaled\"");
        }
    }

    // Do LTEmC, LQCloud and application background tasks
    ltem_doWork();
    lqc_doWork();
    applWork_doFlashLed();
    resetWatchdog();
}


void resetWatchdog()
{
    // IMPORTANT! pet the dog to keep dog happy and app alive
    // #define WD_FAULT_TEST 3
    // ---------------------------------------------------------
    #ifndef WD_FAULT_TEST
    lqSAMD_wdReset();
    #else
    if (loopCnt < WD_FAULT_TEST)
        lqSAMD_wdReset();
    #endif
}


/* LQCloud Network Action Callbacks
========================================================================================================================= 
  Standard callbacks for LQCloud to start and stop network services. Here the LTEm1 (modem) is started and stopped. 
  If LQCloud repeated send retries it will utilize a stop/start sequence to reset the transport. These functions should
  be constructed to block until their function is complete before returning to LQCloud.

  Note: networkStart() returns bool indicating if the network started. Function should return true only if complete, i.e.
  when the network transport state is ready for communications. If returning false, LQC will retry as required.
========================================================================================================================= */


// Reference definition
// lqcNtwkStart_func >> typedef bool (*lqcNtwkStart_func)()

bool networkStart(bool reset)
{
    if (reset)
        ltem_reset();

    if (ltem_getReadyState() != qbg_readyState_appReady)
        ltem_start();                                               // start modem processing

    networkOperator_t networkOp = ntwk_awaitOperator(30000);

    if (strlen(networkOp.operName) > 0)
    {
        uint8_t connAttempts = 0;
        PRINTF(dbgColor__white, "\rNetwork type is %s on %s", networkOp.ntwkMode, networkOp.operName);

        do 
        {
            uint8_t apnCount = ntwk_getActivePdpCntxtCnt();         // populates network APN table in LTEm1c, SUCCESS == network communications established.
            if (apnCount == 0)
            {
                /* Activating PDP context is network carrier dependent: NOT REQUIRED on most carrier networks
                *  If apnCount > 0, assume "data" context is available. Can test with ntwk_getContext(context_ID) != NULL.
                *  If not required, ntwk_activateContext() stills "warms up" the connection 
                */
                if (ntwk_activatePdpContext(DEFAULT_NETWORK_CONTEXT))  // Verizon IP data on ContextId = 1
                    apnCount++;
            }
            if (apnCount > 0)
            {
                pdpCntxt_t *pdpCntxt = ntwk_getPdpCntxt(DEFAULT_NETWORK_CONTEXT);
                PRINTF(dbgColor__white, "\rPDP Context=1, IPaddr=%s\r", pdpCntxt->ipAddress);
                return true;
            }
            PRINTF(dbgColor__white, ".");
            connAttempts++;
            pDelay(5000);
        }
        while (connAttempts < 3);
    }
    return false;
}


// typedef for function pointer: lqcNtwkStop_func >> typedef void (*lqcNtwkStop_func)()
void networkStop()
{
}


/* Application "other" Callbacks
========================================================================================================================= */

void eventNotifCB(uint8_t notifType, uint8_t notifAssm, uint8_t notifInst, const char *notifMsg)
{
    if (notifType >= lqNotifType__CATASTROPHIC)
    {
        PRINTF(dbgColor__error, "LQCloud-HardFault: %s\r", notifMsg);
        // update local indicators like LEDs, OLED, etc.
        lqDiag_setApplicationMessage(notifType, notifMsg);
        #ifdef ENABLE_NEOPIXEL
            pixels[0] = CRGB::Red; 
            FastLED.show();
        #endif

        // // Application Level - LQ Diagnostics Structure (fault stack will be left empty)
        // lqDiag_setNotification(notifType, notifMsg);
        // lqDiag_setPhysPState(mdminfo_rssi());
        // lqDiag_setNtwkPState((ntwk_getPdpCntxt(1) == NULL) ? 0 : 1);
        // lqDiag_setApplPState(lqc_getConnectState("", false));                   // get MQTT state 

        // now gather more invasive data, data requiring functioning LTEmC driver
        //lqDiagInfo.applProtoState = lqc_getConnectState("", true);            // now force read MQTT state from transport

        //assert_brk();
        while (true) {}                                                         // end of the road
    }

    else if (notifType >= lqNotifType__WARNING)
    {
        PRINTF(dbgColor__warn, "LQCloud-Warning: %s\r", notifMsg);
        // update local indicators like LEDs, OLED, etc.
        lqDiag_setApplicationMessage(notifType, notifMsg);
        #ifdef ENABLE_NEOPIXEL
            pixels[0] = CRGB::Orange; 
            FastLED.show();
        #endif
    }

    else if (notifType == lqNotifType_connect)
    {
        PRINTF(dbgColor__info, "LQCloud Connected\r");
        #ifdef ENABLE_NEOPIXEL
            pixels[0] = CRGB::Green; 
            FastLED.show();
        #endif
        return;
    }

    else if (notifType == lqNotifType_disconnect)
    {
        PRINTF(dbgColor__warn, "LQCloud Disconnected\r");
        #ifdef ENABLE_NEOPIXEL
            pixels[0] = CRGB::Magenta; 
            FastLED.show();
        #endif
        lqDiag_setApplicationMessage(notifType, notifMsg);
        return;
    }

    else                                                            //(notifType == lqNotifType_info)
    {
        PRINTF(dbgColor__info, "LQCloud Info: %s\r", notifMsg);
        return;
    }

}


/* Note: A7 (aka pin digital 9) can do double duty with compatible functions. In this example
 * the pin is also a digital input sensing a button press. The voltage divider sensing the 
 * battery voltage (Adafruit Feather) keeps pin 9/A7 at about 2v (aka logic HIGH).
*/

#define VBAT_ENABLED 
#define VBAT_PIN 9          // aka A7, which is not necessarily defined in variant.h
#define VBAT_DIVIDER 2
#define AD_SCALE 1023
#define AD_VREF 3300        // milliVolts

/**
 *	\brief Get feather battery status 
 *  \return Current battery voltage in millivolts 
 */
uint16_t getBatteryStatus()
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



static void getStatus(keyValueDict_t params)
{
    PRINTF(dbgColor__cyan, "CM-RMT_getStatus: ");

    const char *state = "off";
    double loadI = 9.9;

    char body[400] = {0};
    const char *bodyTmplt = "{\"getStatus\":{\"dId\":\"%s\",\"ledState\":\"%s\"}}";
    snprintf(body, sizeof(body), bodyTmplt , lqc_getDeviceId(), !digitalRead(ledPin));

    lqc_sendActionResponse(resultCode__success, body);
}


static void setLedState(keyValueDict_t params)
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
static void doFlashLed(keyValueDict_t params)
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

bool lqcConfigParser(const char *buf, void *lqcDeviceConfig)
{
    lqJsonPropValue_t jProp;
    lqcDeviceConfig_t *cnfg = (lqcDeviceConfig_t *)lqcDeviceConfig;

    jProp = lq_getJsonPropValue(buf, "dvcTest");
    if (jProp.type)
        strncpy(cnfg->deviceLabel, jProp.value, jProp.len);

    jProp = lq_getJsonPropValue(buf, "sasSig");
    if (jProp.type)
        strncpy(cnfg->tokenSigExpiry, jProp.value, jProp.len);

    return true;
}

