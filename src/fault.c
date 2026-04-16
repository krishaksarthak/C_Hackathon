// File   : fault.c
// Purpose: Fault flag management, counter tracking, persistence promotion and severity classification.

#include "fault.h"

void set_fault(FaultStatus *faults, FaultBit bit)
{
    if (bit >= FAULT_BIT_COUNT) { return; }

    faults->active_flags     |= (1U << (uint32_t)bit);
    faults->counters[bit]++;
    faults->consecutive[bit]++;
}

void clear_fault(FaultStatus *faults, FaultBit bit)
{
    if (bit >= FAULT_BIT_COUNT) { return; }

    faults->active_flags     &= ~(1U << (uint32_t)bit);
    faults->persistent_flags &= ~(1U << (uint32_t)bit);
}

void increment_fault_counter(FaultStatus *faults, FaultBit bit)
{
    if (bit >= FAULT_BIT_COUNT) { return; }
    faults->counters[bit]++;
}

void update_fault_status(FaultStatus *faults)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)FAULT_BIT_COUNT; i++)
    {
        if (faults->consecutive[i] >= PERSISTENT_FAULT_LIMIT)
        {
            faults->persistent_flags |= (1U << i);
        }
    }
}


//  The faultSeverity returns the severity of a given fault bit.

//  CRITICAL faults:
//   OVERTEMP, INVALID_GEAR, INVALID_MODE
//   Response: force MODE_FAULT this cycle -> STATE_SAFE + ECU lock.

//  MAJOR faults:
//   OVERSPEED, HIGH_TEMP
//   Response: stay in current mode -> DEGRADED (1st occurrence), STATE_SAFE + ECU lock after MAJOR_FAULT_SAFE_THRESHOLD cycles.

FaultSeverity get_fault_severity(FaultBit bit)
{
    FaultSeverity severity;

    switch (bit)
    {
        case FAULT_BIT_OVERTEMP:
        case FAULT_BIT_INVALID_GEAR:
        case FAULT_BIT_INVALID_MODE:
            severity = FAULT_SEVERITY_CRITICAL;
            break;

        case FAULT_BIT_OVERSPEED:
        case FAULT_BIT_HIGH_TEMP:
        default:
            severity = FAULT_SEVERITY_MAJOR;
            break;
    }

    return severity;
}

const char *fault_to_string(FaultBit bit)
{
    const char *name;

    switch (bit)
    {
        case FAULT_BIT_OVERSPEED: name = "OVERSPEED"; break;
        case FAULT_BIT_OVERTEMP: name = "CRITICAL OVERHEAT"; break;
        case FAULT_BIT_INVALID_GEAR: name = "INVALID_GEAR"; break;
        case FAULT_BIT_INVALID_MODE: name = "INVALID_MODE"; break;
        case FAULT_BIT_HIGH_TEMP: name = "HIGH TEMPERATURE"; break;
        default: name = "UNKNOWN_FAULT"; break;
    }

    return name;
}

int is_fault_active(const FaultStatus *faults, FaultBit bit)
{
    int result = 0;

    if (bit < FAULT_BIT_COUNT)
    {
        result = ((faults->active_flags & (1U << (uint32_t)bit)) != 0U) ? 1 : 0;
    }

    return result;
}

int is_fault_persistent(const FaultStatus *faults, FaultBit bit)
{
    int result = 0;

    if (bit < FAULT_BIT_COUNT)
    {
        result = ((faults->persistent_flags & (1U << (uint32_t)bit)) != 0U) ? 1 : 0;
    }

    return result;
}

int count_active_faults(const FaultStatus *faults)
{
    int count = 0;
    uint32_t i;

    for (i = 0U; i < (uint32_t)FAULT_BIT_COUNT; i++)
    {
        if ((faults->active_flags & (1U << i)) != 0U)
        {
            count++;
        }
    }

    return count;
}

int count_persistent_faults(const FaultStatus *faults)
{
    int count = 0;
    uint32_t i;

    for (i = 0U; i < (uint32_t)FAULT_BIT_COUNT; i++)
    {
        if ((faults->persistent_flags & (1U << i)) != 0U)
        {
            count++;
        }
    }

    return count;
}