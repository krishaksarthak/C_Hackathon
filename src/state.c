
// File   : state.c
// Purpose: System state evaluation — NORMAL / DEGRADED / SAFE.

//   STATE_NORMAL: No active faults and no ECU lock pending.
//   STATE_DEGRADED:
//     One or more MAJOR faults active this cycle (1st occurrence).
//     Vehicle is operable but operator is warned.

#include "state.h"
#include "fault.h"
#include "log.h"

// Scans every MAJOR fault and returns 1

static int any_major_fault_at_threshold(const FaultStatus *faults)
{
    uint32_t i;
    int found = 0;

    for (i = 0U; (i < (uint32_t)FAULT_BIT_COUNT) && (found == 0); i++)
    {
        FaultBit bit = (FaultBit)i;

        if (get_fault_severity(bit) == FAULT_SEVERITY_MAJOR)
        {
            if (faults->consecutive[i] >= MAJOR_FAULT_SAFE_THRESHOLD)
            {
                found = 1;
            }
        }
    }

    return found;
}

void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults)
{
    SystemState new_state;
    const char *reason;
    int active;

    if (status->system_state == STATE_SAFE)
    {
        return;
    }

    active = count_active_faults(faults);
    new_state = status->system_state;
    reason = "no change";

    if (status->current_mode == MODE_FAULT)
    {
        // Critical Fault
        new_state = STATE_SAFE;
        reason = "critical fault — MODE_FAULT active";
        status->reset_required = 1U;
    }
    else if (any_major_fault_at_threshold(faults) != 0)
    {
        // Major Fault
        new_state = STATE_SAFE;
        reason = "major fault reached safe threshold — ECU locked";
        status->reset_required = 1U;
    }
    else if (active > 0)
    {
        // First single major fault
        new_state = STATE_DEGRADED;
        reason = "active fault(s) present — degraded operation";
    }
    else
    {
        // All clear
        new_state = STATE_NORMAL;
        reason = "all faults cleared";
    }

    if (new_state != status->system_state)
    {
        log_state_transition(status->system_state, new_state, reason);
        status->previous_state = status->system_state;
        status->system_state = new_state;
    }
}

const char *state_to_string(SystemState state)
{
    const char *name;

    switch (state)
    {
        case STATE_NORMAL: name = "NORMAL";   break;
        case STATE_DEGRADED: name = "DEGRADED"; break;
        case STATE_SAFE: name = "SAFE";     break;
        default: name = "UNKNOWN";  break;
    }

    return name;
}