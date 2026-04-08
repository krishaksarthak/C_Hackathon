#include <stdio.h>
#include "log.h"
#include "mode.h"
#include "state.h"
#include "fault.h"

/*
 * This allows all functions in this file to write to logs/logs.txt
 */
extern FILE *log_file;

void log_cycle_summary(const VehicleInput *input, const VehicleStatus *status, const FaultStatus *faults) {
    if (!log_file) return; // Safety check

    int i;

    fprintf(log_file, "ECU CYCLE #%-5u SUMMARY\n", status->cycle_count);

    fprintf(log_file, "INPUTS\n");
    fprintf(log_file, "Speed : %4d km/h [%s]\n", input->speed, input->speed_valid ? "OK " : "ERR");
    fprintf(log_file, "Temperature : %4d deg C [%s] \n", input->temperature, input->temp_valid ? "OK " : "ERR");
    fprintf(log_file, "Gear : %4d [%s] \n", input->gear, input->gear_valid ? "OK " : "ERR");
    fprintf(log_file, "Mode Req. : %-12s [%s] \n", mode_to_string(input->requested_mode), input->mode_valid ? "OK " : "ERR");

    fprintf(log_file, "STATUS\n");
    fprintf(log_file, "Mode : %-12s\n", mode_to_string(status->current_mode));
    fprintf(log_file, "Sys State : %-10s\n", state_to_string(status->system_state));

    fprintf(log_file, "FAULTS (active=0x%02X  persistent=0x%02X)\n", faults->active_flags, faults->persistent_flags);

    for (i = 0; i < FAULT_BIT_COUNT; i++) {
        FaultBit bit = (FaultBit)i;
        fprintf(log_file, "%-16s cnt=%-4u active=%-3s persist=%-3s\n", 
                fault_to_string(bit), 
                faults->counters[i], 
                is_fault_active(faults, bit) ? "YES" : "no ", 
                is_fault_persistent(faults, bit) ? "YES" : "no ");
    }
    
    fprintf(log_file, "--------------------------------------------------\n");
    fflush(log_file);
}

void log_input_warning(const char *field, int value) {
    if (log_file) fprintf(log_file, "[WARN] INPUT : Invalid %s value = %d, preserving last valid\n", field, value);
}

void log_mode_transition(VehicleMode from, VehicleMode to, int is_illegal) {
    if (!log_file) return;
    if (is_illegal) {
        fprintf(log_file, "[FAULT] MODE : ILLEGAL transition %s -> %s  => forcing FAULT mode\n", mode_to_string(from), mode_to_string(to));
    } else {
        fprintf(log_file, "[INFO] MODE   : Transition %s -> %s\n", mode_to_string(from), mode_to_string(to));
    }
}

void log_state_transition(SystemState from, SystemState to, const char *reason) {
    if (log_file) fprintf(log_file, "[STATE] SYSTEM : %s -> %s | Reason: %s\n", state_to_string(from), state_to_string(to), reason);
}

void log_fault_event(FaultBit bit, const char *detail) {
    if (log_file) fprintf(log_file, "[FAULT] %-16s : %s\n", fault_to_string(bit), detail);
}

void log_warning(const char *message) {
    if (log_file) fprintf(log_file, "[WARN] %s\n", message);
}

void log_info(const char *message) {
    if (log_file) fprintf(log_file, "[INFO] %s\n", message);
}