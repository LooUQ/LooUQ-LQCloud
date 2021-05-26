/******************************************************************************
 *  \file lqDiagnostics.h
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
 * Diagnostics (local and remote) support for LQCloud projects
 *****************************************************************************/

#ifndef __LQDIAGNOSTICS_H__
#define __LQDIAGNOSTICS_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum lqDiagResetCause_tag
{
    lqDiagResetCause_powerOn = 1,
    lqDiagResetCause_pwrCore = 2,
    lqDiagResetCause_pwrPeriph = 4,
    lqDiagResetCause_nvm = 8,
    lqDiagResetCause_external = 16,
    lqDiagResetCause_watchdog = 32,
    lqDiagResetCause_system = 64,
    lqDiagResetCause_backup = 128
} lqDiagResetCause_t;


typedef struct lqDiagInfo_tag
{
    // application level diagnostic info
    lqDiagResetCause_t resetCause;
    uint8_t notifCd;
    char notifMsg[20];
    uint8_t applProtoState;                 // indications: TCP/UDP/SSL connected, MQTT state, etc.
    uint8_t ntwkProtoState;                 // indications: LTE PDP, etc.
    uint8_t physProtoState;                 // indications: rssi, etc.
    // system level diagnostic info
    /*tbd*/
} lqDiagInfo_t;


/* Inline diagnostic functions
 ----------------------------------------------------------------------------------------------- */
#define IS_DEBUGGER_ATTACHED()  (*(volatile uint32_t *)0xE000EDF0) & (1 << 0)


// NOTE: If you are using CMSIS, the registers can also be
// accessed through CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk
// #define HALT_IF_DEBUGGING()                                     \
//     do {                                                        \
//         if ((*(volatile uint32_t *)0xE000EDF0) & (1 << 0)) {    \
//             __asm("bkpt 1");                                    \
//         }                                                       \
//     } while (0);


// #define HARDFAULT_HANDLING_ASM(_x)                              \
//   __asm volatile(                                               \
//       "tst lr, #4 \n"                                           \
//       "ite eq \n"                                               \
//       "mrseq r0, msp \n"                                        \
//       "mrsne r0, psp \n"                                        \
//       "b my_fault_handler_c \n"                                 \
//     )

// __attribute__((packed))
// typedef struct ContextStateFrame_tag  
// {
// uint32_t r0;
//   uint32_t r1;
//   uint32_t r2;
//   uint32_t r3;
//   uint32_t r12;
//   uint32_t lr;
//   uint32_t return_address;
//   uint32_t xpsr;
// } sContextStateFrame_t;


// // Disable optimizations for this function so "frame" argument
// // does not get optimized away
// __attribute__((optimize("O0")))
// void my_fault_handler_c(sContextStateFrame_t *frame) {
//   // If and only if a debugger is attached, execute a breakpoint
//   // instruction so we can take a look at what triggered the fault
//   HALT_IF_DEBUGGING();

//   // Logic for dealing with the exception. Typically:
//   //  - log the fault which occurred for postmortem analysis
//   //  - If the fault is recoverable,
//   //    - clear errors and return back to Thread Mode
//   //  - else
//   //    - reboot system
// }



#ifdef __cplusplus
extern "C"
{
#endif


bool checkSendDiagnostics(lqDiagResetCause_t rcause, lqDiagInfo_t *diagInfo);


#ifdef __cplusplus
}
#endif // !__cplusplus



#endif  /* !__LQDIAGNOSTICS_H__ */
