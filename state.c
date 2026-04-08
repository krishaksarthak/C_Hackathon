#include "state.h"

/*
 * System State Rules:
 *
 * NORMAL:
 *  - No active faults
 *
 * DEGRADED:
 *  - One active fault
 *  - Or repeated non-critical faults
 *
 * SAFE:
 *  - Two or more persistent critical faults
 *  - Or repeated critical fault across cycles
 *
 * NOTE:
 *  - Once SAFE, the system does NOT recover automatically
 *    unless reset logic is explicitly implemented.
 */

void evaluate_system_state(VehicleStatus *status,
                           const FaultStatus *faults)
{
    unsigned int activeFaults = 0U;

    if (status == 0 || faults == 0)
    {
        return; /* Defensive programming */
    }

    /* Count number of active fault flags */
    if (faults->flags & FAULT_OVERSPEED)
        activeFaults++;

    if (faults->flags & FAULT_OVERTEMP)
        activeFaults++;

    if (faults->flags & FAULT_INVALID_GEAR)
        activeFaults++;

    if (faults->flags & FAULT_INVALID_MODE)
        activeFaults++;

    /*
     * SAFE state lock:
     * Once the system enters SAFE, it remains there.
     */
    if (status->systemState == STATE_SAFE)
    {
        return;
    }

    /*
     * Escalation logic
     */
    if (activeFaults == 0U)
    {
        status->systemState = STATE_NORMAL;
    }
    else if (activeFaults == 1U)
    {
        status->systemState = STATE_DEGRADED;
    }
    else
    {
        status->systemState = STATE_SAFE;
    }
}