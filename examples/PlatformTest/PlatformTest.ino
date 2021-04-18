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


// using Adafruit's CPP package for watchdog, TODO C99 version
#include <Adafruit_SleepyDog.h>         // watchdog support

#include <Adafruit_NeoPixel.h>


#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define VERIZON_DATA_CONTEXT 1

#define SUMMARY_SZ 80
#define BODY_SZ 400

#define ENABLE_NEOPIXEL
#define NEO_PIN 8
#define NEO_BRIGHTNESS 5                // brightness: 0-255: Note these things are bright, so <10 is visible

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
// and a counter shared by the "start" function and the "do work" function.
uint8_t flashesRemaining;


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

    PRINTF(dbgColor_error, "LooUQ Cloud - CloudTest\r");
    lqcResetCause_t resetCause = (lqcResetCause_t)Watchdog.resetCause();
    PRINTFC(dbgColor_magenta,"RCause=%d \r", resetCause);

    // device application setup and initialization
    pinMode(buttonPin, INPUT);                          // feather uses pin=9 for battery voltage divider, floats at ~2v
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);                         // off
    asm(".global _printf_float");                       // forces build to link in float support for printf

    #ifdef ENABLE_NEOPIXEL
    neo.begin();                                        // using feather NEO as a "status" indicator
    neo.setBrightness(NEO_BRIGHTNESS);                  // dim globally to use uint8 (0..255) values for RGB
    neo.setPixelColor(0, 0, 0, 255);                    // blue at start
    neo.show();
    #endif

    ltem1_create(ltem1_pinConfig, appNotifRecvr);
    mqtt_create();

    // since the LQCloud deviceId is the LTEm1 modem's IMEI, the application needs to start the network.
    networkStart();

    /* Add LQCloud to project and register local service callbacks.
     * ----------------------------------------------------------------------------------------- */
    lqc_create(resetCause, appNotifRecvr, networkStart, networkStop, NULL, getBatteryStatus, getFreeMemory);
    /* Callbacks
     * - cloudNotificationsHandler: cloud informs app of events
     * - networkStart:
     * - networkStop:
     * - getUxpPwrStatus: bool indicating external power source online
     * - getBatteryStatus: enum of battery power reserve
     * - getFreeMemory: how much memory between stack and heap. Even without malloc, does the stack have room
    */

    //lqc_start(LQCLOUD_URL, DEVICEID, LQCLOUD_SASTOKEN, "");
    lqc_start(LQCLOUD_URL, mdminfo_ltem1().imei, LQCLOUD_SASTOKEN, "");

    PRINTF(dbgColor_info, "Connected to LQ Cloud, Device started.\r");

    sendTelemetry_intvl = wrkTime_create(PERIOD_FROM_SECONDS(30));      // this application sends telemetry continuously every 30 secs
    alert_intvl = wrkTime_create(PERIOD_FROM_MINUTES(60));              // and a periodic alert every 60 min

    registerAppLqcActions();
    /* now enable watchdog as app transitions to loop()
     * setup has already registered yield callback with LTEm1c to handle longer running network actions
     */
    uint16_t wdtDuration = Watchdog.enable();                           // default = 16 seconds
    PRINTFC(dbgColor_warn, "WDT Duration=%d\r\r", wdtDuration);

    // status: init completed.
    #ifdef ENABLE_NEOPIXEL
    neo.setPixelColor(0, 0, 255, 0);       // green
    neo.show();
    #endif
}


/* loop() ------------------------------------------------------------------------------------------------------------- */
bool lastSend = true;

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

        bool thisSend = lqc_sendTelemetry("LQ CloudTest", summary, body);
        if (!thisSend && !lastSend)
             appNotifRecvr(lqcNotifType_hardFault, "Successive send errors");
        lastSend = thisSend;

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
    //applWork_doFlashLed();

    //#define WD_FAULT_TEST 3
    // IMPORTANT! pet the dog to keep dog happy and app alive
    // ---------------------------------------------------------
    #ifdef WD_FAULT_TEST
    if (loopCnt < WD_FAULT_TEST)
        Watchdog.reset();
    #else
    Watchdog.reset();
    #endif
}



/* LQCloud Network Action Callbacks
========================================================================================================================= 
  Standard callbacks for LQCloud to start and stop network services. Here the LTEm1 (modem) is started and stopped 
  (power on/off). If LQCloud repeated send retries it will utilize a stop/start sequence to reset the transport.
  These functions should be constructed to block until their function is complete before returning to LQCloud.

  Note: networkStart() returns bool indicating if the network started. Function "complete" is when the function can
  determine the network state. If returning false, LQC will retry as required.
========================================================================================================================= */

// lqcNtwkStart_func >> typedef bool (*lqcNtwkStart_func)()
bool networkStart()
{
    if (ltem1_getReadyState() != qbg_readyState_appReady)
        ltem1_start(ltem1Protocols_mqtt);                   // validates protocols, powers on modem (standard power profile) and starts processing

    PRINTFC(dbgColor_none, "Waiting on network...\r");
    networkOperator_t networkOp;

    networkOp = ntwk_awaitOperator(30000);

    if (strlen(networkOp.operName) > 0)
    {
        uint8_t apnCount = ntwk_getActivePdpCntxtCnt();     // populates network APN table in LTEm1c, SUCCESS == network communications established.
        if (apnCount == 0)
        {
            /* Activating PDP context is network carrier dependent: not required on most carrier networks
             *   If apnCount > 0, assume "data" context is available. Can test with ntwk_getContext(context_ID) != NULL.
             *   If not required, ntwk_activateContext() stills "warms up" the connection 
            */
            ntwk_activatePdpContext(VERIZON_DATA_CONTEXT);  // Verizon IP data on ContextId = 1
        }
        pdpCntxt_t *pdpCntxt = ntwk_getPdpCntxt(1);
        PRINTFC(dbgColor_white, "PDP Context=1, IPaddr=%s\r", pdpCntxt->ipAddress);
    }
    if (strlen(networkOp.operName) > 0)
    {
        PRINTFC(dbgColor_white, "Network type is %s on %s\r", networkOp.ntwkMode, networkOp.operName);
        return true;
    }
    return false;
}


// lqcNtwkStop_func >> typedef void (*lqcNtwkStop_func)()
void networkStop()
{
    ltem1_reset();
}


/* Application "other" Callbacks
========================================================================================================================= */

void appNotifRecvr(uint8_t notifType, const char *notifMsg)
{
    switch (notifType)
    {
        case lqcNotifType_info:
            PRINTFC(dbgColor_info, "LQCloud Info: %s\r", notifMsg);
            return;

        case lqcNotifType_connect:
            PRINTFC(dbgColor_info, "LQCloud Connected\r");
            #ifdef ENABLE_NEOPIXEL
                neo.setPixelColor(0, 0, 255, 0);        // green
                neo.show();
            #endif
            return;

        case lqcNotifType_disconnect:
            PRINTFC(dbgColor_warn, "LQCloud Attempting Connect\r");
            #ifdef ENABLE_NEOPIXEL
                neo.setPixelColor(0, 255, 0, 255);       // magenta
                neo.show();
            #endif
            return;
    }

    if (notifType > lqcNotifType__CATASTROPHIC)
    {
        PRINTFC(dbgColor_error, "LQCloud-HardFault: %s\r", notifMsg);
        // update local indicators like LEDs, OLED, etc.
        #ifdef ENABLE_NEOPIXEL
            neo.setPixelColor(0, 255, 0, 0);        // red
            neo.show();
        #endif
        while (true) {}
    }
}



#define VBAT_ENABLED 
#define VBAT_PIN 9          // aka A7, which is not necessarily defined in variant.h
#define VBAT_DIVIDER 2
#define AD_SCALE 1023
#define AD_VREF 3.3

/* Note: A7 (aka pin digital 9) can do double duty with compatible functions. In this example
 * the pin is also a digital input sensing a button press. The voltage divider sensing the 
 * battery voltage (Adafruit Feather) keeps pin 9/A7 at about 2v (aka logic HIGH).
*/

/**
 *	\brief Get feather battery status 
 */
batteryWarning_t getBatteryStatus()
{
    double vbatAnalog = 0.0;
    batteryWarning_t rslt = batteryStatus_good;

    #ifdef VBAT_ENABLED
        vbatAnalog = analogRead(VBAT_PIN);
        // detach pin from analog mux, req'd for shared digital input use
        pinMode(VBAT_PIN, INPUT);                       
        vbatAnalog *= VBAT_DIVIDER * AD_VREF / AD_SCALE;

        if (vbatAnalog < 3.8)
            rslt = batteryStatus_yellow;
        else if (vbatAnalog < 3.7)
            rslt = batteryStatus_red;
    #endif
    return rslt;
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
    #define KVALUESZ 15
    char kValue[KVALUESZ] = {0};

    lqc_getDictValue("ledState", params, kValue, KVALUESZ);
    PRINTF(dbgColor_cyan, "setledState: %s\r", kValue);

    if (kValue[0] == '1')
        digitalWrite(ledPin, LOW);  // ON
    else
        digitalWrite(ledPin, HIGH); // otherwise OFF

    lqc_sendActionResponse(RESULT_CODE_SUCCESS, "{}");
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
static const char *doFlashLed_params = { "flashCount" "=" LQCACTN_PARAMTYPE_INT "&" "cycleMillis" "=" LQCACTN_PARAMTYPE_INT };
static void doFlashLed(keyValueDict_t params)
{
    uint8_t flashes;
    uint16_t cycleMillis;

    #define DICTIONARY_VALUE_CSTRING_SZ 12
    char kvalue[DICTIONARY_VALUE_CSTRING_SZ];

    lqc_getDictValue("flashCount", params, kvalue, DICTIONARY_VALUE_CSTRING_SZ);
    flashes = atoi(kvalue);

    if (flashes = 0 || flashes > 10)
        lqc_sendActionResponse(RESULT_CODE_BADREQUEST, "Requested flash count must be between 1 and 10.");

    lqc_getDictValue("cycleMillis", params, kvalue, DICTIONARY_VALUE_CSTRING_SZ);
    cycleMillis = atoi(kvalue);

    /* If cycleMillis * flashes is too long LQ Cloud will timeout action request, to prevent that we limit total time
    * here. For long running LQ Cloud actions: start process, return status of action start, and use a separate Action
    * to test for action process still running or complete. Alternate pattern: use alert webhook callback on process end.
    */

    uint16_t cycleDuration = flashes * 2 * cycleMillis;
    if (cycleDuration > 5000)
    {
        PRINTF(dbgColor_cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
        lqc_sendActionResponse(RESULT_CODE_BADREQUEST, "Flashing event is too long, 5 seconds max.");
    }

    if (flashesRemaining != 0)                                  
        lqc_sendActionResponse(RESULT_CODE_CONFLICT, "Flash process is already busy flashing.");
    else
    {
        ledFlash_intvl = wrkTime_create(cycleMillis);
        flashesRemaining = flashes;                     // this will count down, 0 = complete
    }

    PRINTF(dbgColor_cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
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
            PRINTF(dbgColor_cyan, "doLampFlash Completed\r");
            lqc_sendActionResponse(RESULT_CODE_SUCCESS, "");   // flash action completed sucessfully
        }
    }
}


void registerAppLqcActions()
{
    lqc_regApplAction("get-stat", &getStatus, getStatus_params);
    lqc_regApplAction("set-led", &setLedState, setLedState_params);
    lqc_regApplAction("do-flashLed", &doFlashLed, doFlashLed_params);

    // macro way
    // REGISTER_ACTION("get-stat", getStatus, getStatus_params);
    // REGISTER_ACTION("set-ase", setAseState, setAseState_params);
}


