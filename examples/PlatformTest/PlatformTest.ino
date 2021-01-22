/******************************************************************************
 *  \file LQCloud-PlatformTest.ino
 *  \author Greg Terrell
 *
 *  \copyright Copyright (c) 2020 LooUQ Incorporated. 
 *             Copyright (c) 2020 CloudMelt, LLC.
 *
 ******************************************************************************
 * 
 *****************************************************************************/

#define HOST_FEATHER_UXPLOR             // specify the pin configuration

#define _DEBUG                          // commment out to disable PRINTF expansion to statements // PER SOURCE TRANSLATION UNIT 
#include <ltem1c.h>
#include <lqCloud.h>
// debugging output options             UNCOMMENT one of the next two lines to direct debug (PRINTF) output
#include <jlinkRtt.h>                   // output debug PRINTF macros to J-Link RTT channel
// #define SERIAL_OPT 1                    // enable serial port comm with devl host (1=force ready test)

#include "dvcSettings.h"

#include <Adafruit_NeoPixel.h>


#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define VERIZON_DATA_CONTEXT 1

#define SUMMARY_SZ 80
#define BODY_SZ 400

#define ENABLE_NEOPIXEL
#define NEO_PIN 8
#define NEO_BRIGHTNESS 5                                    // brightness: 0-255: Note these things are bright, so 5 is visible

/* -------------------------------------------------------------------------------------------------------------------- */

const int ledPin = 5;
const int buttonPin = 9;                // double duty with battery sense, see getBatteryVoltage()
const int debouncePeriod = 10;

// application variables
char telemetry[BODY_SZ];
bool lqcConnected = false;
uint16_t loopCnt = 1;
wrkTime_t sendTelemetry_intvl;
wrkTime_t alert_intvl;

// example action doLedFlash: has a worker function to do the work without blocking
wrkTime_t ledFlash_intvl;

// NEOPixel display
#ifdef ENABLE_NEOPIXEL
Adafruit_NeoPixel neo(1, NEO_PIN, NEO_GRB + NEO_KHZ800);
#endif

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

    PRINTF(dbgColor_white, "LooUQ Cloud - CloudTest\r");

    // device application setup and initialization
    pinMode(buttonPin, INPUT);                          // feather uses pin=9 for battery voltage divider, floats at ~2v
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);                         // off
    asm(".global _printf_float");                       // forces build to link in float support for printf

    #ifdef ENABLE_NEOPIXEL
    neo.begin();                                        // using feather NEO as a "status" indicator
    neo.setBrightness(NEO_BRIGHTNESS);                  // dim globally to use uint8 (0..255) values for RGB
    #endif

    lqc_create(appNotificationHandler, NULL, getBatteryVoltage, getFreeMemory);    // add LQ Cloud to project and register local services

    /* Callbacks
     * - cloudNotificationsHandler:
     * - getUxpPwrStatus: 
     * - getBatteryVoltage:
     * - getFreeMemory: 
    */

    ltem1_create(ltem1_pinConfig, ltem1Start_powerOn, ltem1Functionality_services);
    mqtt_create();
    
    PRINTF(dbgColor_none, "Waiting on network...\r");
    // local OLED display WAITING...
    
    networkOperator_t networkOp;

    while (true)                                        // wait for cell network PDP context available
    {
        #ifdef ENABLE_NEOPIXEL
        neo.setPixelColor(0, 0, 0, 255);                // blue during initialization
        neo.show();
        #endif
        networkOp = ntwk_awaitOperator(30000);

        if (strlen(networkOp.operName) > 0)
        {
            uint8_t apnCount = ntwk_getActivePdpContexts();     // populates network APN table in LTEm1c, SUCCESS == network communications established.
            if (apnCount == 0)
            {
                /* Network dependent: not required on most networks
                 * If apnCount > 0, assume "data" context is available. Can test with ntwk_getContext(context_ID) != NULL.
                 * 
                 * If not required, ntwk_activateContext() stills "warms up" the connection 
                */
                ntwk_activatePdpContext(VERIZON_DATA_CONTEXT);  // Verizon IP data on ContextId = 1
            }
            pdpContext_t *pdpCntxt = ntwk_getPdpContext(1);
            PRINTFC(dbgColor_white, "PDP Context=1, IPaddr=%s\r", pdpCntxt->ipAddress);
            break;
        }

        #ifdef ENABLE_NEOPIXEL
        neo.setPixelColor(0, 255, 80, 0);
        neo.show();
        #endif
        millisTime_t startRetryAt = millis();
        while (!wrkTime_elapsed(startRetryAt, PERIOD_FROM_SECONDS(90))) { }     // local (blocking) wait for cell ntwk retry
    }
    PRINTF(dbgColor_white, "Network type is %s on %s\r", networkOp.ntwkMode, networkOp.operName);

    lqc_start(LQCLOUD_URL, mdminfo_ltem1().imei, LQCLOUD_SASTOKEN, NULL);
    PRINTF(dbgColor_info, "Connected to LQ Cloud, Device started.\r");

    sendTelemetry_intvl = wrkTime_createPeriodic(PERIOD_FROM_SECONDS(30));      // this application sends telemetry continuously every 30 secs
    alert_intvl = wrkTime_createPeriodic(PERIOD_FROM_MINUTES(60));              // and a periodic alert every 60 min
    registerActions();

    // status: init completed.
    #ifdef ENABLE_NEOPIXEL
    neo.setPixelColor(0, 0, 255, 0);       // green
    neo.show();
    #endif
}


/* loop() ------------------------------------------------------------------------------------------------------------- */

void loop() 
{
    if (wrkTime_doNow(&alert_intvl))
    {
        char alrtSummary[40];
        snprintf(alrtSummary, 40, "Periodic Alert, Loop=%d", loopCnt);
        PRINTF(dbgColor_magenta, "Sending periodic alert.\r");
        lqc_sendAlert("cloudTest-periodic-alert", "PeriodicAlert", alrtSummary);
    }
    if (wrkTime_doNow(&sendTelemetry_intvl))
    {
        PRINTF(dbgColor_dMagenta, "\rLoop=%i>>\r", loopCnt);

        char summary[SUMMARY_SZ] = {0};
        char body[BODY_SZ] = {0};
        snprintf(summary, SUMMARY_SZ, "Test Telemetry, Loop=%d", loopCnt);
        snprintf(body, BODY_SZ, "{\"loopCnt\": %d}" , loopCnt);
        lqc_sendTelemetry("LQ CloudTest", summary, body);

        loopCnt++;
    }

    if (digitalRead(buttonPin) == LOW)
    {
        delay(debouncePeriod);
        int bttn = digitalRead(buttonPin);
        if (digitalRead(buttonPin) == LOW)
        {
            PRINTFC(dbgColor_info, "\rUser ReqstAlert Button Pressed\r");
            lqc_sendAlert("cloudTest-user-alert", "UserAlert", "\"User signaled\"");
        }
    }

    // Do LQ Cloud and application background tasks
    lqc_doWork();
    applWork_doFlashLed();
}



/* Application Actions
========================================================================================================================= */


static const char *getStatus_params = { "" };
static void getStatus(keyValueDict_t params)
{
    PRINTF(dbgColor_cyan, "CM-RMT_getStatus: ");

    const char *state = "off";
    double loadI = 9.9;

    char body[BODY_SZ] = {0};
    const char *bodyTmplt = "{\"getStatus\":{\"dId\":\"%s\",\"ledState\":\"%s\"}}";
    snprintf(body, BODY_SZ, bodyTmplt , lqc_getDeviceId(), !digitalRead(ledPin));

    lqc_sendActionResponse(RESULT_CODE_SUCCESS, body);
}


static const char *setLedState_params = { "ledState" "=" LQCACTN_PARAMTYPE_INT };
static void setLedState(keyValueDict_t params)
{
    if (lqc_getDictValue("ledState", params))
    {
        PRINTF(dbgColor_cyan, "setledState: %c\r", lqc_getDictValue("ledState", params)[0]);

        if (lqc_getDictValue("ledState", params)[0] == '1')
            digitalWrite(ledPin, LOW);  // ON
        else
            digitalWrite(ledPin, HIGH); // otherwise OFF

        lqc_sendActionResponse(RESULT_CODE_SUCCESS, "{}");
        return;
    }
    lqc_sendActionResponse(RESULT_CODE_BADREQUEST, "{}");
}


    /* Using a workSchedule object to handle needed "flash" delays without blocking delay()
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
static const char *doFlashLed_params = { "flashCount" "=" LQCACTN_PARAMTYPE_INT "&" "cycleMillis" "=" LQCACTN_PARAMTYPE_INT };
static void doFlashLed(keyValueDict_t params)
{
    uint8_t flashes;
    uint16_t cycleMillis;

    flashes = atol(lqc_getDictValue("flashCount", params));
    cycleMillis = atoi(lqc_getDictValue("cycleMillis", params));

    /* If cycleMillis * flashes is too long LQ Cloud will timeout action request, to prevent that we limit total time
    * here. For long running LQ Cloud actions: start process, return status of action start, and use a separate Action
    * to test for action process still running or complete. Alternate pattern: use alert webhook callback on process end.
    */

    uint16_t cycleDuration = flashes * 2 * cycleMillis;
    if (cycleDuration > 10000)
    {
        PRINTF(dbgColor_cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
        lqc_sendActionResponse(RESULT_CODE_BADREQUEST, "Flashing event is too long.");
    }

    if (ledFlash_intvl.userState == 0)
    {
        wrkTime_createPeriodic(cycleMillis);
        ledFlash_intvl.userState = flashes;                     // this will count down, 0 = complete
    }
    else
        lqc_sendActionResponse(RESULT_CODE_CONFLICT, "Flash is busy.");

    PRINTF(dbgColor_cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
    // started the process now return
}


/**
 *	\brief The applWork_doFlashLed is the pseudo-background "worker" function process the LED flashing until request is complete.
 * 
 *  The applWork_doFlashLed is the background worker that continues the process until complete. This function's invoke is in standard loop().
 *  applWork_* is just a convention to make the background process easier to identify.
 * 
 *  NOTE: In the workSched object the .enabled and .state properties are for appl use as needed. Here we are using .state as a count down to completion.
 */
void applWork_doFlashLed()
{
    // no loop here, do some work and leave

    if (ledFlash_intvl.userState > 0 && wrkTime_doNow(&ledFlash_intvl))         // put state test first to short-circuit evaluation
    {
        if (ledFlash_intvl.userState % 2 == 0)
            digitalWrite(ledPin, LOW);      // ON
        else
            digitalWrite(ledPin, HIGH);

        ledFlash_intvl.userState--;             // count down

        if (ledFlash_intvl.userState == 0)
        {
            PRINTF(dbgColor_cyan, "doLampFlash Completed\r");
            lqc_sendActionResponse(RESULT_CODE_SUCCESS, "");   // flash action completed sucessfully
        }
    }
}


void registerActions()
{
    lqc_regApplAction("get-stat", &getStatus, getStatus_params);
    lqc_regApplAction("set-led", &setLedState, setLedState_params);
    lqc_regApplAction("do-flashLed", &doFlashLed, doFlashLed_params);

    // macro way
    // REGISTER_ACTION("get-stat", getStatus, getStatus_params);
    // REGISTER_ACTION("set-ase", setAseState, setAseState_params);
}


/* Platform helpers
========================================================================================================================= */

void appNotificationHandler(lqcAppNotification_t notifType, const char *notifMsg)
{
    switch (notifType)
    {
        case lqcAppNotification_connect:
            PRINTFC(dbgColor_info, "LQCloud Connected.\r");
            // update local indicators like LEDs, OLED, etc.
            #ifdef ENABLE_NEOPIXEL
            neo.setPixelColor(0, 0, 255, 0);        // green
            neo.show();
            #endif
            return;

        case lqcAppNotification_disconnect:
            PRINTFC(dbgColor_warn, "LQCloud Disconnected.\r");
            // update local indicators like LEDs, OLED, etc.
            #ifdef ENABLE_NEOPIXEL
            neo.setPixelColor(0, 255, 255, 0);      // yellow
            neo.show();
            #endif
            return;

        //case lqcAppNotification_connectStatus:
        //    return

        case lqcAppNotification_hardFault:
            PRINTFC(dbgColor_error, "LQCloud-HardFault: %s\r", notifMsg);
            // update local indicators like LEDs, OLED, etc.
            #ifdef ENABLE_NEOPIXEL
            neo.setPixelColor(0, 255, 0, 0);        // red
            neo.show();
            #endif
            // optionally HALT or return to LQCloud to periodically attempt reconnect.
            // while (true) { }
            return;
            
        // default: fall through and return
    }
}



#define VBAT_ENABLED 
#define VBAT_PIN 9          // aka A7, which is not necessarily defined in variant.h
#define VBAT_DIVIDER 2
#define AD_SCALE 1023
#define AD_VREF 3.3

/* Note: A7 (aka pin digital 9) can do double duty with compatible functions. In this example
 * the pin is also a digital input sensing a button press. The voltage divider sensing the 
 * battery voltage (Adafruit Feather) keeps pin 9/A7 at about 2v.
*/

/**
 *	\brief Get feather battery voltage 
 */
double getBatteryVoltage()
{
    double vbatAnalog = 0.0;
    #ifdef VBAT_ENABLED
    vbatAnalog = analogRead(VBAT_PIN);
    // detach pin from analog mux, req'd for shared digital input use
    pinMode(VBAT_PIN, INPUT);                       
    vbatAnalog *= VBAT_DIVIDER * AD_VREF / AD_SCALE;
    #endif
    return vbatAnalog;
}


/* Check free memory (stack-to-heap) 
 * - Remove if not needed for production
 * ---------------------------------------------------------------------------------
*/
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C"  char *sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int getFreeMemory() 
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

