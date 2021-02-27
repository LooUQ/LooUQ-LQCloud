# 1 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
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
# 15 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
# 16 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino" 2
# 17 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino" 2
// debugging output options             UNCOMMENT one of the next two lines to direct debug (PRINTF) output
# 19 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino" 2
// #define SERIAL_OPT 1                    // enable serial port comm with devl host (1=force ready test)

# 22 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino" 2

# 24 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino" 2
# 36 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
/* -------------------------------------------------------------------------------------------------------------------- */

const int ledPin = 5;
const int buttonPin = 9; // double duty with battery sense, see getBatteryVoltage()
const int debouncePeriod = 10;

// application variables
char telemetry[400];
bool lqcConnected = false;
uint16_t loopCnt = 1;
wrkTime_t sendTelemetry_intvl;
wrkTime_t alert_intvl;

// example action doLedFlash: has a worker function to do the work without blocking
wrkTime_t ledFlash_intvl;

// NEOPixel display

Adafruit_NeoPixel neo(1, 8, ((1<<6) | (1<<4) | (0<<2) | (2)) /*|< Transmit as G,R,B*/ + 0x0000 /*|< 800 KHz data transmission*/);




/* setup() 

 * --------------------------------------------------------------------------------------------- */
void setup() {
# 72 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
    rtt_printf(dbgColor_white, ("LooUQ Cloud - CloudTest\r"));;

    // device application setup and initialization
    pinMode(buttonPin, (0x0)); // feather uses pin=9 for battery voltage divider, floats at ~2v
    pinMode(ledPin, (0x1));
    digitalWrite(ledPin, (0x1)); // off
    asm(".global _printf_float"); // forces build to link in float support for printf


    neo.begin(); // using feather NEO as a "status" indicator
    neo.setBrightness(5 /* brightness: 0-255: Note these things are bright, so <10 is visible*/); // dim globally to use uint8 (0..255) values for RGB
    neo.setPixelColor(0, 0, 0, 255); // blue at start
    neo.show();


    ltem1_create(ltem1_pinConfig, ltem1Start_powerOff, ltem1Functionality_services);
    // since the LQCloud deviceId is the LTEm1 modem's IMEI, the application needs to start the network.
    networkStart();

    /* Add LQCloud to project and register local service callbacks.

     * ----------------------------------------------------------------------------------------- */
# 93 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
    lqc_create(notificationHandler, networkStart, networkStop, 
# 93 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino" 3 4
                                                              __null
# 93 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
                                                                  , getBatteryStatus, getFreeMemory);
    /* Callbacks

     * - cloudNotificationsHandler: cloud informs app of events

     * - networkStart:

     * - networkStop:

     * - getUxpPwrStatus: bool indicating external power source online

     * - getBatteryStatus: enum of battery power reserve

     * - getFreeMemory: how much memory between stack and heap. Even without malloc, does the stack have room

    */
# 102 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
    lqc_start("iothub-dev-pelogical.azure-devices.net", mdminfo_ltem1().imei, "SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2F864508030074113&sig=E3xuQEgprendqdqfD%2BuAQQsHM%2B6v%2B6LUv5bYfeSkonM%3D&se=1614672710", 
# 102 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino" 3 4
                                                                  __null
# 102 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
                                                                      );

    rtt_printf(dbgColor_info, ("Connected to LQ Cloud, Device started.\r"));;

    neo.setPixelColor(0, 0, 255, 0); // green once started
    neo.show();


    // sendTelemetry_intvl = wrkTime_create(PERIOD_FROM_SECONDS(30));      // this application sends telemetry continuously every 30 secs
    // alert_intvl = wrkTime_create(PERIOD_FROM_MINUTES(60));              // and a periodic alert every 60 min
    sendTelemetry_intvl = wrkTime_create((30 * 1000)); // this application sends telemetry continuously every 30 secs

    registerActions();

    // status: init completed.

    neo.setPixelColor(0, 0, 255, 0); // green
    neo.show();

}


/* loop() ------------------------------------------------------------------------------------------------------------- */

void loop()
{
    if (wrkTime_doNow(&alert_intvl))
    {
        char alrtSummary[40];
        snprintf(alrtSummary, 40, "Periodic Alert, Loop=%d", loopCnt);
        rtt_printf(dbgColor_magenta, ("Sending periodic alert.\r"));;
        lqc_sendAlert("cloudTest-periodic-alert", "PeriodicAlert", alrtSummary);
    }
    if (wrkTime_doNow(&sendTelemetry_intvl))
    {
        rtt_printf(dbgColor_dMagenta, ("\rLoop=%i>>\r"), loopCnt);;

        char summary[80] = {0};
        char body[400] = {0};
        snprintf(summary, 80, "Test Telemetry, Loop=%d", loopCnt);
        snprintf(body, 400, "{\"loopCnt\": %d}" , loopCnt);
        lqc_sendTelemetry("LQ CloudTest", summary, body);

        loopCnt++;
    }

    // if (digitalRead(buttonPin) == LOW)
    // {
    //     delay(debouncePeriod);
    //     int bttn = digitalRead(buttonPin);
    //     if (digitalRead(buttonPin) == LOW)
    //     {
    //         PRINTFC(dbgColor_info, "\rUser ReqstAlert Button Pressed\r");
    //         lqc_sendAlert("cloudTest-user-alert", "UserAlert", "\"User signaled\"");
    //     }
    // }

    // Do LQ Cloud and application background tasks
    lqc_doWork();
    //applWork_doFlashLed();
}



/* LQCloud Network Action Callbacks

========================================================================================================================= 

  Standard callbacks for LQCloud to start and stop network services. Here the LTEm1 (modem) is started and stopped 

  (power on/off). If LQCloud repeated send retries it will utilize a stop/start sequence to reset the transport.

  These functions should be constructed to block until their function is complete before returning to LQCloud.



  Note: networkStart() returns bool indicating if the network started. Function "complete" is when the function can

  determine the network state. If returning false, LQC will retry as required.

========================================================================================================================= */
# 176 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
// lqcNtwkStart_func >> typedef bool (*lqcNtwkStart_func)()
bool networkStart()
{
    if (ltem1_getReadyState() != qbg_readyState_appReady)
        ltem1_start(); // powers on modem (standard power profile)

    rtt_printf(dbgColor_none, ("Waiting on network...\r"));;
    networkOperator_t networkOp;

    networkOp = ntwk_awaitOperator(30000);

    if (strlen(networkOp.operName) > 0)
    {
        uint8_t apnCount = ntwk_getActivePdpCntxtCnt(); // populates network APN table in LTEm1c, SUCCESS == network communications established.
        if (apnCount == 0)
        {
            /* Activating PDP context is network carrier dependent: not required on most carrier networks

             *   If apnCount > 0, assume "data" context is available. Can test with ntwk_getContext(context_ID) != NULL.

             *   If not required, ntwk_activateContext() stills "warms up" the connection 

            */
# 196 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
            ntwk_activatePdpContext(1); // Verizon IP data on ContextId = 1
        }
        pdpCntxt_t *pdpCntxt = ntwk_getPdpCntxt(1);
        rtt_printf(dbgColor_white, ("PDP Context=1, IPaddr=%s\r"), pdpCntxt->ipAddress);;
    }
    if (strlen(networkOp.operName) > 0)
    {
        rtt_printf(dbgColor_white, ("Network type is %s on %s\r"), networkOp.ntwkMode, networkOp.operName);;
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
void notificationHandler(notificationType_t notifType, const char *notifMsg)
{
    switch (notifType)
    {
        case notificationType_info:
            rtt_printf(dbgColor_info, ("LQCloud Info: %s\r"), notifMsg);;
            return;

        case notificationType_connectPending:
            rtt_printf(dbgColor_info, ("LQCloud Attempting Connect\r"));;

                neo.setPixelColor(0, 255, 80, 0); // yellow-ish
                neo.show();

            return;

        case notificationType_connect:
            rtt_printf(dbgColor_info, ("LQCloud Connected\r"));;

                neo.setPixelColor(0, 0, 255, 0); // green
                neo.show();

            return;

        case notificationType_disconnect:
            rtt_printf(dbgColor_warn, ("LQCloud Disconnected\r"));;
            // update local indicators like LEDs, OLED, etc.

                neo.setPixelColor(0, 255, 255, 0); // yellow
                neo.show();

            return;

        case notificationType_hardFault:
            rtt_printf(dbgColor_error, ("LQCloud-HardFault: %s\r"), notifMsg);;
            // update local indicators like LEDs, OLED, etc.

                neo.setPixelColor(0, 255, 0, 0); // red
                neo.show();

            while (true) {}
    }
}
# 272 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
/* Note: A7 (aka pin digital 9) can do double duty with compatible functions. In this example

 * the pin is also a digital input sensing a button press. The voltage divider sensing the 

 * battery voltage (Adafruit Feather) keeps pin 9/A7 at about 2v (aka logic HIGH).

*/
# 277 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
/**

 *	\brief Get feather battery status 

 */
# 280 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
batteryWarning_t getBatteryStatus()
{
    double vbatAnalog = 0.0;
    batteryWarning_t rslt = batteryStatus_good;


        vbatAnalog = analogRead(9 /* aka A7, which is not necessarily defined in variant.h*/);
        // detach pin from analog mux, req'd for shared digital input use
        pinMode(9 /* aka A7, which is not necessarily defined in variant.h*/, (0x0));
        vbatAnalog *= 2 * 3.3 / 1023;

        if (vbatAnalog < 3.8)
            rslt = batteryStatus_yellow;
        else if (vbatAnalog < 3.7)
            rslt = batteryStatus_red;

    return rslt;
}


/* Check free memory SAMD tested (stack-to-heap) 

 * ---------------------------------------------------------------------------------

*/
# 304 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);




int getFreeMemory()
{
    char top;

    return &top - reinterpret_cast<char*>(sbrk(0));





}



/* Application Actions

========================================================================================================================= */

static const char *getStatus_params = { "" };
static void getStatus(keyValueDict_t params)
{
    rtt_printf(dbgColor_cyan, ("CM-RMT_getStatus: "));;

    const char *state = "off";
    double loadI = 9.9;

    char body[400] = {0};
    const char *bodyTmplt = "{\"getStatus\":{\"dId\":\"%s\",\"ledState\":\"%s\"}}";
    snprintf(body, 400, bodyTmplt , lqc_getDeviceId(), !digitalRead(ledPin));

    lqc_sendActionResponse(200, body);
}


static const char *setLedState_params = { "ledState" "=" "int" };

static void setLedState(keyValueDict_t params)
{

    char kValue[15] = {0};

    lqc_getDictValue("ledState", params, kValue, 15);
    rtt_printf(dbgColor_cyan, ("setledState: %s\r"), kValue);;

    if (kValue[0] == '1')
        digitalWrite(ledPin, (0x0)); // ON
    else
        digitalWrite(ledPin, (0x1)); // otherwise OFF

    lqc_sendActionResponse(200, "{}");
    return;
}


    /* Using a workSchedule object to handle needed "flash" delays without blocking delay()

     * WorkSchedule properties enabled and state are for application consumer use. Here we are using

     * state to count up flashes until request serviced. Clearing it back to 0 when complete.

     * When triggered this function uses workSched_reset to start the interval in sync with request.

     * 

     * Note: this function pattern doesn't prevent duplicate triggers. A 2nd trigger while a flash 

     * sequence is running will be mo

    */
# 373 "c:\\Users\\GregTerrell\\Documents\\CodeDev\\Arduino\\libraries\\LooUQ-LQCloud\\examples\\PlatformTest\\PlatformTest.ino"
// /**
//  *	\brief The doFlashLed is the function to kick off the LED flashing, it is invoked by the LC Cloud Actions dispatcher. 
//  * 
//  *  See applWork_doFlashLed() below for the rest of the story. 
//  */
// static const char *doFlashLed_params = { "flashCount" "=" LQCACTN_PARAMTYPE_INT "&" "cycleMillis" "=" LQCACTN_PARAMTYPE_INT };
// static void doFlashLed(keyValueDict_t params)
// {
//     uint8_t flashes;
//     uint16_t cycleMillis;

//     flashes = atol(lqc_getDictValue("flashCount", params));
//     cycleMillis = atoi(lqc_getDictValue("cycleMillis", params));

//     /* If cycleMillis * flashes is too long LQ Cloud will timeout action request, to prevent that we limit total time
//     * here. For long running LQ Cloud actions: start process, return status of action start, and use a separate Action
//     * to test for action process still running or complete. Alternate pattern: use alert webhook callback on process end.
//     */

//     uint16_t cycleDuration = flashes * 2 * cycleMillis;
//     if (cycleDuration > 10000)
//     {
//         PRINTF(dbgColor_cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
//         lqc_sendActionResponse(RESULT_CODE_BADREQUEST, "Flashing event is too long.");
//     }

//     if (ledFlash_intvl.userState == 0)
//     {
//         wrkTime_create(cycleMillis);
//         ledFlash_intvl.userState = flashes;                     // this will count down, 0 = complete
//     }
//     else
//         lqc_sendActionResponse(RESULT_CODE_CONFLICT, "Flash is busy.");

//     PRINTF(dbgColor_cyan, "doLampFlash: flashes=%i, cycleMs=%i\r", flashes, cycleMillis);
//     // started the process now return
// }


// /**
//  *	\brief The applWork_doFlashLed is the pseudo-background "worker" function process the LED flashing until request is complete.
//  * 
//  *  The applWork_doFlashLed is the background worker that continues the process until complete. This function's invoke is in standard loop().
//  *  applWork_* is just a convention to make the background process easier to identify.
//  * 
//  *  NOTE: In the workSched object the .enabled and .state properties are for appl use as needed. Here we are using .state as a count down to completion.
//  */
// void applWork_doFlashLed()
// {
//     // no loop here, do some work and leave

//     if (ledFlash_intvl.userState > 0 && wrkTime_doNow(&ledFlash_intvl))         // put state test first to short-circuit evaluation
//     {
//         if (ledFlash_intvl.userState % 2 == 0)
//             digitalWrite(ledPin, LOW);      // ON
//         else
//             digitalWrite(ledPin, HIGH);

//         ledFlash_intvl.userState--;             // count down

//         if (ledFlash_intvl.userState == 0)
//         {
//             PRINTF(dbgColor_cyan, "doLampFlash Completed\r");
//             lqc_sendActionResponse(RESULT_CODE_SUCCESS, "");   // flash action completed sucessfully
//         }
//     }
// }


void registerActions()
{
    lqc_regApplAction("get-stat", &getStatus, getStatus_params);
    lqc_regApplAction("set-led", &setLedState, setLedState_params);
    // lqc_regApplAction("do-flashLed", &doFlashLed, doFlashLed_params);

    // macro way
    // REGISTER_ACTION("get-stat", getStatus, getStatus_params);
    // REGISTER_ACTION("set-ase", setAseState, setAseState_params);
}
