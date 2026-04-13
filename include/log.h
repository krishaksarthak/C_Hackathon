#ifndef LOG_H
#define LOG_H

#include "types.h"

void log_cycle_summary(const VehicleInput *input, const VehicleStatus *status, const FaultStatus *faults);
void log_input_warning(const char *field, int value);
void log_mode_transition(VehicleMode from, VehicleMode to, int is_illegal);
void log_state_transition(SystemState from, SystemState to, const char *reason);
void log_fault_event(FaultBit bit, const char *detail);
void log_warning(const char *message);
void log_info(const char *message);

// logs all active faults in priority order, marking the primary fault
void log_faults_prioritised(const FaultStatus *faults,
                             const VehicleInput *input);

// Structured cycle header printed to stdout
void print_cycle_header(uint32_t cycle, const VehicleInput *input);

#endif