#ifndef FAULT_H
#define FAULT_H

#include "types.h"

void set_fault(FaultStatus *faults, FaultBit bit);
void clear_fault(FaultStatus *faults, FaultBit bit);
void increment_fault_counter(FaultStatus *faults, FaultBit bit);
void update_fault_status(FaultStatus *faults);

// Helper and status check functions
const char *fault_to_string(FaultBit bit);
int is_fault_active(const FaultStatus *faults, FaultBit bit);
int is_fault_persistent(const FaultStatus *faults, FaultBit bit);
int count_active_faults(const FaultStatus *faults);
int count_persistent_faults(const FaultStatus *faults);

#endif