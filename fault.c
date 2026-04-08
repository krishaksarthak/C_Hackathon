#include "fault.h"

/*
 * set_fault
 * ---------
 * Sets the specified fault flag and increments
 * the corresponding fault counter.
 */
void set_fault(FaultStatus *faults, unsigned int fault)
{
    if (faults == 0)
    {
        return; /* Defensive programming */
    }

    /* Set fault flag */
    faults->flags |= fault;

    /* Update counters */
    switch (fault)
    {
        case FAULT_OVERSPEED:
            faults->overspeedCount++;
            break;

        case FAULT_OVERTEMP:
            faults->overtempCount++;
            break;

        case FAULT_INVALID_GEAR:
            faults->gearCount++;
            break;

        case FAULT_INVALID_MODE:
            faults->modeCount++;
            break;

        default:
            /* Unknown fault – do nothing */
            break;
    }
}

/*
 * clear_fault
 * -----------
 * Clears the specified fault flag.
 * Counters are NOT reset to preserve history.
 */
void clear_fault(FaultStatus *faults, unsigned int fault)
{
    if (faults == 0)
    {
        return;
    }

    faults->flags &= ~fault;
}

/*
 * is_fault_active
 * ---------------
 * Returns 1 if the fault flag is set, otherwise 0.
 */
int is_fault_active(const FaultStatus *faults, unsigned int fault)
{
    if (faults == 0)
    {
        return 0;
    }

    return ((faults->flags & fault) != 0U);
}

/*
 * clear_all_faults
 * ----------------
 * Clears all active fault flags.
 * Counters remain intact for diagnostics.
 */
void clear_all_faults(FaultStatus *faults)
{
    if (faults == 0)
    {
        return;
    }

    faults->flags = 0U;
}
