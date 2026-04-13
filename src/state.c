#include "state.h"
#include "fault.h"
#include "log.h"

void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults)
{
    int active     = count_active_faults(faults);
    int persistent = count_persistent_faults(faults);

    /*
     * Only OVERTEMP and OVERSPEED are treated as critical for the purpose of
     * escalating to STATE_SAFE.  (Overspeed is a *major* fault — two
     * persistent major/critical faults together still escalate to SAFE.)
     */
    int critical_persistent = 0;
    if (is_fault_persistent(faults, FAULT_BIT_OVERTEMP))  critical_persistent++;
    if (is_fault_persistent(faults, FAULT_BIT_OVERSPEED)) critical_persistent++;

    SystemState new_state = status->system_state;

    if (critical_persistent >= 2)
    {
        new_state = STATE_SAFE;
    }
    else if (active > 0 || persistent > 0)
    {
        if (status->system_state != STATE_SAFE)
            new_state = STATE_DEGRADED;
    }
    else
    {
        if (status->system_state != STATE_SAFE)
            new_state = STATE_NORMAL;
    }

    if (new_state != status->system_state)
    {
        const char *reason = "unknown";

        if (new_state == STATE_SAFE)
            reason = "2+ persistent critical faults detected";
        else if (new_state == STATE_DEGRADED && active > 0)
            reason = "active fault(s) present";
        else if (new_state == STATE_DEGRADED && persistent > 0)
            reason = "persistent fault(s) present";
        else if (new_state == STATE_NORMAL)
            reason = "all faults cleared";

        log_state_transition(status->system_state, new_state, reason);
        status->previous_state = status->system_state;
        status->system_state   = new_state;
    }
}

const char *state_to_string(SystemState state)
{
    switch (state)
    {
        case STATE_NORMAL:   return "NORMAL";
        case STATE_DEGRADED: return "DEGRADED";
        case STATE_SAFE:     return "SAFE";
        default:             return "UNKNOWN";
    }
}