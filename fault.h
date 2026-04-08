#ifndef FAULT_H
#define FAULT_H

#include "types.h"

/*
 * Fault bit definitions
 * Each fault occupies one bit position
 */
#define FAULT_OVERSPEED        (1U << 0)
#define FAULT_OVERTEMP         (1U << 1)
#define FAULT_INVALID_GEAR     (1U << 2)
#define FAULT_INVALID_MODE     (1U << 3)

/*
 * Fault handling APIs
 */

/* Set a fault flag and increment its counter */
void set_fault(FaultStatus *faults, unsigned int fault);

/* Clear a fault flag */
void clear_fault(FaultStatus *faults, unsigned int fault);

/* Check if a fault is currently active */
int is_fault_active(const FaultStatus *faults, unsigned int fault);

/* Clear all active fault flags (used for reset/recovery logic) */
void clear_all_faults(FaultStatus *faults);

#endif /* FAULT_H */