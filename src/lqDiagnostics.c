
#include "lqDiagnostics.h"
// #include <cmsis_gcc.h>
// #include <core_cm0plus.h>


#define FORCE_DIAG_SEND 0

bool checkSendDiagnostics(lqDiagResetCause_t rcause, lqDiagInfo_t *diagInfo)
{
    diagInfo->resetCause = rcause;
    if (rcause == lqDiagResetCause_watchdog || rcause == lqDiagResetCause_system)
    {
        #if FORCE_DIAG_SEND == 1
        return true;
        #else
        return IS_DEBUGGER_ATTACHED();
        #endif
    }
    memset(diagInfo + 1, 0, sizeof(lqDiagInfo_t) - 1);
    return false;
}