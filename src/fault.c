#include "fault.h"
#include <stddef.h>

void set_fault(FaultStatus *faults, FaultBit bit){
    if (bit >= FAULT_BIT_COUNT) return;

    faults->active_flags |= (1U << bit);
    faults->counters[bit]++;
    faults->consecutive[bit]++;
}

void clear_fault(FaultStatus *faults, FaultBit bit){
    if (bit >= FAULT_BIT_COUNT) return;

    faults->active_flags      &= ~(1U << bit);
    faults->consecutive[bit]   = 0;
    faults->persistent_flags  &= ~(1U << bit);
}

void increment_fault_counter(FaultStatus *faults, FaultBit bit){
    if (bit >= FAULT_BIT_COUNT) return;
    faults->counters[bit]++;
}

void update_fault_status(FaultStatus *faults){
    int i;
    for (i = 0; i < FAULT_BIT_COUNT; i++)
    {
        if (faults->consecutive[i] >= PERSISTENT_FAULT_LIMIT)
        {
            faults->persistent_flags |= (1U << i);
        }
    }
}

const char *fault_to_string(FaultBit bit){
    switch (bit)
    {
        case FAULT_BIT_OVERSPEED:    return "OVERSPEED";
        case FAULT_BIT_OVERTEMP:     return "OVERTEMPERATURE";
        case FAULT_BIT_INVALID_GEAR: return "INVALID_GEAR";
        case FAULT_BIT_INVALID_MODE: return "INVALID_MODE";
        default:                     return "UNKNOWN_FAULT";
    }
}

int is_fault_active(const FaultStatus *faults, FaultBit bit){
    if (bit >= FAULT_BIT_COUNT) return 0;
    return (faults->active_flags & (1U << bit)) ? 1 : 0;
}

int is_fault_persistent(const FaultStatus *faults, FaultBit bit){
    if (bit >= FAULT_BIT_COUNT) return 0;
    return (faults->persistent_flags & (1U << bit)) ? 1 : 0;
}

int count_active_faults(const FaultStatus *faults){
    int count = 0, i;
    for (i = 0; i < FAULT_BIT_COUNT; i++)
    {
        if (faults->active_flags & (1U << i)) count++;
    }
    return count;
}

int count_persistent_faults(const FaultStatus *faults){
    int count = 0, i;
    for (i = 0; i < FAULT_BIT_COUNT; i++)
    {
        if (faults->persistent_flags & (1U << i)) count++;
    }
    return count;
}