# 1 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
/******************************************************************************

 *  \file LQCloud-StaticProvision.ino

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

 * Demonstrates a typical LTEmC (LTEm1 IoT modem) application for MQTT 

 * applications and a reference design of a LQCloud device application with 

 * telemetry, alerts and actions.

 * 

 * This is a LQCloud baseline functions test. See LQCloudTestAdv

 * for additional features on the Feather M0 including flash storage of 

 * settings, watchdog timer, and SAMD diagnostics send to cloud.

 *****************************************************************************/
# 36 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
// debugging output options             // LTEm1c will satisfy PRINTF references with empty definition if not already resolved

    asm(".global _printf_float"); // forces build to link in float support for printf

# 41 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2
# 49 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
// define options for how to assemble this build


# 53 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2
# 54 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2
# 55 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2
# 56 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2

# 58 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2
# 59 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2
# 60 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2

# 62 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2
# 63 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2




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
# 82 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
// example settings for testing in dvcSettings.h
// using arduino.json "prebuild" option to copy from common location in LooUQ-LQCloud\examples folder
# 85 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 2

// test setup
uint16_t loopCnt = 0;
wrkTime_t sendTelemetry_intvl;
wrkTime_t alert_intvl;

// LTEm variables
mqttCtrl_t mqttCtrl; // MQTT control, data to manage MQTT connection to server
uint8_t receiveBuffer[640]; // Data buffer where received information is returned (can be local, global, or dynamic... your call)
char mqttTopic[200]; // application buffer to craft TX MQTT topic
char mqttMessage[200]; // application buffer to craft TX MQTT publish content (body)
resultCode_t result;







/* LooUQ uses this device app to continuous test LQCloud instances. To support optional scenarios, we use the LTEm-UXplor

 * Feather board with a momentary switch and a LED to simulate a IoT device with a digital sensor and actuator          */
# 106 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
/* -------------------------------------------------------------------------------------------------------------------- */
const int ledPin = 5;
const int buttonPin = 9; // double duty with battery sense, see getBatteryVoltage()
const int debouncePeriod = 10;
const int onboardLed = 13;

/* CloudTest EXAMPLE action doLedFlash: has a worker function to do the work without blocking

 * and a counter shared by the "start" function and the "do work" function. */
# 114 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
wrkTime_t ledFlash_intvl;
uint8_t flashesRemaining;


/* Define signature of the cloud accessible device actions (commands) 

 * Utilizing pre-process automatic string concatenation */
# 120 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
// #define LQCACTN_PARAMTYPE_INT    "int"
static const char *getStatus_params = { "" };
static const char *setLedState_params = { "ledState" "=" "int" };
static const char *doFlashLed_params = { "flashCount" "=" "int" "&" "cycleMillis" "=" "int" };


/* setup() 

 * --------------------------------------------------------------------------------------------- */
# 128 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
void setup() {
# 138 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
    do { rtt_printf(dbgColor__red, ("LooUQ Cloud - Basic CloudTest\r")); } while(0);
    lqDiag_registerEventCallback(eventNotifCB); // the LQ ASSERT\Diagnostics subsystem can report a fault back to app for local display (aka RED light)
    lqDiag_setResetCause(lqSAMD_getResetCause());
    do { rtt_printf(dbgColor__none, ("RCause=%d\r"), lqSAMD_getResetCause()); } while(0);


    // device application setup and initialization
    pinMode(buttonPin, (0x0)); // feather uses pin=9 for battery voltage divider, floats at ~2v
    pinMode(ledPin, (0x1));
    pinMode(onboardLed, (0x1));
    digitalWrite(ledPin, (0x1)); // LEDs off
    digitalWrite(onboardLed, (0x1));

    ltem_create(ltem_pinConfig, resetWatchdog, eventNotifCB); // create modem\transport reference
    ltem_start((resetAction_t)resetAlways);

    /* Add LQCloud to project and register local service callbacks.

     * ----------------------------------------------------------------------------------------- */
# 156 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
    lqc_create(lqcConnect_mqttContinuous, "867198053224766", "1234567890ABCDEF", eventNotifCB, resetWatchdog);
    /* Callbacks

     * - cloudNotificationsHandler: cloud informs app of events

     * - networkStart: calls back to application to perform the steps required to start the network transport

     * - networkStop: calls back to application to perform the steps required to STOP the network transport

     * - getUxpPwrStatus: bool indicating external power source online

     * - getBatteryStatus: enum of battery power reserve

     * - getFreeMemory: how much memory between stack and heap. Even without malloc, does the stack have room

    */
# 166 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
    /* Define settings for the MQTT (over TLS) connection to LQCloud, TLS v1.2 and MQTT v3.11 required 

     * Even though LQCloud will manage receives, the receive buffer has to be defined and provided based on

     * your application's needs. You can start larger, then use mqtt_getLastBufferReqd(mqttCtrl_t *mqttCtrl)

     * to see if you can reduce buffer size */
# 170 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
    tls_configure(socket_0, tlsVersion_tls12, tlsCipher_any, tlsCertExpiration_default, tlsSecurityLevel_default);
    mqtt_initControl(&mqttCtrl, socket_0, receiveBuffer, sizeof(receiveBuffer), 
# 171 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino" 3 4
                                                                               __null
# 171 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
                                                                                   );

    lqcDeviceConfig_t lqcCnfg;
    const char* lqcLabel = "testDevice";
    const char* lqcToken = "tbd";

    lqc_decomposeSASToken(lqcToken, &lqcCnfg);

    memcpy(lqcCnfg.deviceLabel, lqcLabel, sizeof(lqcLabel));


    lqc_start(&mqttCtrl, lqcCnfg);
    do { rtt_printf(dbgColor__info, ("Connected to LQ Cloud, Device started.\r")); } while(0);

    //lqDiag_report(lqc_getDeviceLabel(), lqc_sendDiagAlert);            // invoke diagnostics reporter to analyze device start conditions and optionally send diagnostics to cloud/host

    sendTelemetry_intvl = wrkTime_create((30 * 1000)); // this application sends telemetry continuously every 30 secs
    alert_intvl = wrkTime_create((60 * 1000 * 60)); // and a periodic alert every 60 min

    lqc_registerApplicationAction("get-stat", &getStatus, getStatus_params); // register application cloud-to-device commands (callbacks)
    lqc_registerApplicationAction("set-led", &setLedState, setLedState_params);
    lqc_registerApplicationAction("do-flashLed", &doFlashLed, doFlashLed_params);
    // macro way
    // REGISTER_ACTION("get-stat", getStatus, getStatus_params);

    /* now enable watchdog as app transitions to loop()

     * setup has already registered yield callback with LTEm1c to handle longer running network actions

     */
# 199 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
    // uint16_t wdtDuration = lqSAMD_wdEnable(0);                          // default = 16 seconds
    // PRINTF(dbgColor__blue, "WDT Duration=%d\r\r", wdtDuration);
}


/* loop() ------------------------------------------------------------------------------------------------------------- */
bool lastSendSuccessful = true;

void loop()
{
    if (wrkTime_doNow(&alert_intvl))
    {
        char alrtSummary[40];
        snprintf(alrtSummary, 40, "Periodic Alert, Loop=%d", loopCnt);
        do { rtt_printf(dbgColor__magenta, ("Sending periodic alert.\r")); } while(0);
        lqc_sendAlert("cloudTest-periodic-alert", "PeriodicAlert", alrtSummary);
    }
    if (wrkTime_doNow(&sendTelemetry_intvl))
    {
        do { rtt_printf(dbgColor__dMagenta, ("\rLoop=%i>>\r"), loopCnt); } while(0);

        char summary[80] = {0};
        char body[400] = {0};
        snprintf(summary, sizeof(summary), "Test Telemetry, Loop=%d", loopCnt);
        snprintf(body, sizeof(body), "{\"loopCnt\": %d}" , loopCnt);

        bool thisSend = lqc_sendTelemetry("LQ CloudTest", summary, body);
        if (!thisSend && !lastSendSuccessful)
             eventNotifCB(appEvent_fault_hardFault, "Successive send errors");
        lastSendSuccessful = thisSend;

        loopCnt++;
    }

    if (digitalRead(buttonPin) == (0x0))
    {
        delay(debouncePeriod);
        int bttn = digitalRead(buttonPin);
        if (digitalRead(buttonPin) == (0x0))
        {
            do { rtt_printf(dbgColor__info, ("\rUser ReqstAlert Button Pressed\r")); } while(0);
            lqc_sendAlert("cloudTest-user-alert", "UserAlert", "\"User signaled\"");
        }
    }

    // Do LQ Cloud and application background tasks
    lqc_doWork();
    applWork_doFlashLed();


    // //#define WD_FAULT_TEST 3
    // // IMPORTANT! pet the dog to keep dog happy and app alive
    // // ---------------------------------------------------------
    // #ifdef WD_FAULT_TEST
    // if (loopCnt < WD_FAULT_TEST)
    //     Watchdog.reset();
    // #else
    // lqSAMD_wdReset();
    // #endif
}

void resetWatchdog()
{
    // IMPORTANT! pet the dog to keep dog happy and app alive
    // #define WD_FAULT_TEST 3
    // ---------------------------------------------------------

    lqSAMD_wdReset();




}


/* LQCloud Network Action Callbacks

========================================================================================================================= 

  Standard callbacks for LQCloud to start and stop network services. Here the LTEm1 (modem) is started and stopped. 

  If LQCloud repeated send retries it will utilize a stop/start sequence to reset the transport. These functions should

  be constructed to block until their function is complete before returning to LQCloud.



  Note: networkStart() returns bool indicating if the network started. Function should return true only if complete, i.e.

  when the network transport state is ready for communications. If returning false, LQC will retry as required.

========================================================================================================================= */
# 285 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
// Reference definition
// lqcNtwkStart_func >> typedef bool (*lqcNtwkStart_func)()

bool networkStart(bool reset)
{
    if (ltem_getDeviceState() != deviceState_appReady)
        ltem_start((resetAction_t)resetAlways); // start modem processing

    providerInfo_t *ntwkProvider = ntwk_awaitProvider(30000);

    if (strlen(ntwkProvider->name) > 0)
    {
        uint8_t apnCount = ntwk_getActiveNetworkCount(); // populates network APN table in LTEm1c, SUCCESS == network communications established.
        if (apnCount == 0)
        {
            /* Activating PDP context is network carrier dependent: not required on most carrier networks

             *   If apnCount > 0, assume "data" context is available. Can test with ntwk_getContext(context_ID) != NULL.

             *   If not required, ntwk_activateContext() stills "warms up" the connection 

            */
# 304 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
            ntwk_activateNetwork(1, pdpProtocolType_IPV4, ""); // Verizon IP data on ContextId = 1
        }
        networkInfo_t *pdpNtwk = ntwk_getNetworkInfo(1);
        do { rtt_printf(dbgColor__white, ("PDP Context=1, IPaddr=%s\r"), pdpNtwk->ipAddress); } while(0);
        do { rtt_printf(dbgColor__white, ("Network type is %s on %s\r"), ntwkProvider->iotMode, ntwkProvider->name); } while(0);
        return true;
    }
    return false;
}


// typedef for function pointer: lqcNtwkStop_func >> typedef void (*lqcNtwkStop_func)()
void networkStop()
{
    ltem_reset((resetAction_t)resetAlways);
}


/* Application "other" Callbacks

========================================================================================================================= */
void eventNotifCB(uint8_t notifType, const char *notifMsg)
{
    if (notifType > appEvent__FAULTS)
    {
        // update local indicators like LEDs, OLED, etc.
        do { rtt_printf(dbgColor__error, ("LQCloud-HardFault: %s\r"), notifMsg); } while(0);

        // // Application Level - LQ Diagnostics Structure (fault stack will be left empty)
        // lqDiag_setNotification(notifType, notifMsg);
        // lqDiag_setPhysPState(mdminfo_rssi());
        // lqDiag_setNtwkPState((ntwk_getPdpCntxt(1) == NULL) ? 0 : 1);
        // lqDiag_setApplPState(lqc_getConnectState("", false));                   // get MQTT state 
        // now gather more invasive data, data requiring functioning LTEmC driver
        //lqDiagInfo.applProtoState = lqc_getConnectState("", true);            // now force read MQTT state from transport

        assert_brk();
        while (true) {} // should never get here
    }

    if (notifType > appEvent__WARNINGS)
    {
        do { rtt_printf(dbgColor__warn, ("LQCloud Warning\r")); } while(0);
    }
    else if (notifType > appEvent_ntwk_connected)
    {
        do { rtt_printf(dbgColor__info, ("LQCloud Connected\r")); } while(0);
    }
    else if (notifType > appEvent_ntwk_disconnected)
    {
        do { rtt_printf(dbgColor__warn, ("LQCloud Disconnected\r")); } while(0);
    }
    else
    {
        do { rtt_printf(dbgColor__info, ("LQCloud Info: %s\r"), notifMsg); } while(0);
    }
    return;
}
# 371 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
/* Note: A7 (aka pin digital 9) can do double duty with compatible functions. In this example

 * the pin is also a digital input sensing a button press. The voltage divider sensing the 

 * battery voltage (Adafruit Feather) keeps pin 9/A7 at about 2v (aka logic HIGH).

*/
# 376 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
/**

 *	\brief Get feather battery status 

 *  \return Current battery voltage in millivolts 

 */
# 380 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
uint16_t getBatteryStatus()
{
    uint32_t vbatAnalog = 0;


        vbatAnalog = analogRead(9 /* aka A7, which is not necessarily defined in variant.h*/);
        // detach pin from analog mux, req'd for shared digital input use
        pinMode(9 /* aka A7, which is not necessarily defined in variant.h*/, (0x0));
        // vbatAnalog *= (VBAT_DIVIDER * AD_VREF / AD_SCALE);
        vbatAnalog = vbatAnalog * 2 * 3300 /* milliVolts*/ / 1023;


    return vbatAnalog;
}


/* Check free memory SAMD tested (stack-to-heap) 

 * ---------------------------------------------------------------------------------

*/
# 400 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);




uint32_t getFreeMemory()
{
    char top;

    return &top - reinterpret_cast<char*>(sbrk(0));





}



/* Application Actions

========================================================================================================================= */


static void getStatus(keyValueDict_t params)
{
    do { rtt_printf(dbgColor__cyan, ("CM-RMT_getStatus: ")); } while(0);

    const char *state = "off";
    double loadI = 9.9;

    char body[400] = {0};
    const char *bodyTmplt = "{\"getStatus\":{\"dId\":\"%s\",\"ledState\":\"%s\"}}";
    snprintf(body, sizeof(body), bodyTmplt , lqc_getDeviceId(), !digitalRead(ledPin));

    lqc_sendActionResponse(resultCode__success, body);
}


static void setLedState(keyValueDict_t params)
{

    char kValue[15] = {0};

    lq_getQryStrDictionaryValue("ledState", params, kValue, 15);
    do { rtt_printf(dbgColor__cyan, ("setledState: %s\r"), kValue); } while(0);

    if (kValue[0] == '1')
        digitalWrite(ledPin, (0x0)); // ON
    else
        digitalWrite(ledPin, (0x1)); // otherwise OFF

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
# 468 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
/**

 *	\brief The doFlashLed is the function to kick off the LED flashing, it is invoked by the LC Cloud Actions dispatcher. 

 * 

 *  See applWork_doFlashLed() below for the rest of the story. 

 */
# 473 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
static void doFlashLed(keyValueDict_t params)
{
    uint8_t flashes;
    uint16_t cycleMillis;


    char kvalue[12];

    lq_getQryStrDictionaryValue("flashCount", params, kvalue, 12);
    flashes = atoi(kvalue);

    if (flashes = 0 || flashes > 10)
        lqc_sendActionResponse(resultCode__badRequest, "Requested flash count must be between 1 and 10.");

    lq_getQryStrDictionaryValue("cycleMillis", params, kvalue, 12);
    cycleMillis = atoi(kvalue);

    /* If cycleMillis * flashes is too long LQ Cloud will timeout action request, to prevent that we limit total time

    * here. For long running LQ Cloud actions: start process, return status of action start, and use a separate Action

    * to test for action process still running or complete. Alternate pattern: use alert webhook callback on process end.

    */
# 495 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
    uint16_t cycleDuration = flashes * 2 * cycleMillis;
    if (cycleDuration > 5000)
    {
        do { rtt_printf(dbgColor__cyan, ("doLampFlash: flashes=%i, cycleMs=%i\r"), flashes, cycleMillis); } while(0);
        lqc_sendActionResponse(resultCode__badRequest, "Flashing event is too long, 5 seconds max.");
    }

    if (flashesRemaining != 0)
        lqc_sendActionResponse(resultCode__conflict, "Flash process is already busy flashing.");
    else
    {
        ledFlash_intvl = wrkTime_create(cycleMillis);
        flashesRemaining = flashes; // this will count down, 0 = complete
    }

    do { rtt_printf(dbgColor__cyan, ("doLampFlash: flashes=%i, cycleMs=%i\r"), flashes, cycleMillis); } while(0);
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
# 524 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\LQCloud-StaticProvision\\LQCloud-StaticProvision.ino"
void applWork_doFlashLed()
{
    // no loop here, do some work and leave

    if (flashesRemaining > 0 && wrkTime_doNow(&ledFlash_intvl)) // put state test first to short-circuit evaluation
    {
        if (flashesRemaining % 2 == 0)
            digitalWrite(ledPin, (0x0)); // ON
        else
            digitalWrite(ledPin, (0x1));

        flashesRemaining--; // count down

        if (flashesRemaining == 0)
        {
            do { rtt_printf(dbgColor__cyan, ("doLampFlash Completed\r")); } while(0);
            lqc_sendActionResponse(resultCode__success, ""); // flash action completed sucessfully
        }
    }
}
