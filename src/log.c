#include <stdio.h>
#include "log.h"
#include "mode.h"
#include "state.h"
#include "fault.h"

extern FILE *log_file;

/* ── Priority table ─────────────────────────────────────────────────────────
 *
 * When multiple faults fire in the same cycle they are reported in this
 * descending priority order.  The highest-priority active fault is also
 * marked as [PRIMARY FAULT].
 *
 * Priority  Fault                      Classification
 * --------  -------------------------  --------------
 *   1       Critical Overheat          CRITICAL
 *   2       Invalid Mode               MAJOR
 *   2       Invalid Gear               MAJOR
 *   3       Overspeed                  MAJOR
 *   4       High Temperature           WARNING
 * ────────────────────────────────────────────────────────────────────────── */

static const FaultBit fault_priority_order[FAULT_BIT_COUNT] = {
    FAULT_BIT_OVERTEMP,     /* P1: Critical Overheat — CRITICAL */
    FAULT_BIT_INVALID_MODE, /* P2: Invalid Mode      — MAJOR    */
    FAULT_BIT_INVALID_GEAR, /* P2: Invalid Gear      — MAJOR    */
    FAULT_BIT_OVERSPEED,    /* P3: Overspeed         — MAJOR    */
    FAULT_BIT_HIGH_TEMP,    /* P4: High Temperature  — WARNING  */
};

/*
 * Returns a human-readable classification label for a fault bit.
 * Each bit now maps to exactly one severity — no runtime temperature
 * check needed since FAULT_BIT_OVERTEMP and FAULT_BIT_HIGH_TEMP are
 * separate bits.
 */
static const char *fault_classification(FaultBit bit)
{
    switch (bit)
    {
    case FAULT_BIT_OVERTEMP:
        return "CRITICAL";
    case FAULT_BIT_OVERSPEED:
        return "MAJOR";
    case FAULT_BIT_INVALID_GEAR:
        return "MAJOR";
    case FAULT_BIT_INVALID_MODE:
        return "MAJOR";
    case FAULT_BIT_HIGH_TEMP:
        return "WARNING";
    default:
        return "UNKNOWN";
    }
}

/*
 * log_faults_prioritised()
 *
 * Logs every active fault in descending priority order.
 * The first active fault found (highest priority) is tagged [PRIMARY FAULT].
 * If only one fault is active no [PRIMARY FAULT] tag is emitted (redundant).
 */
void log_faults_prioritised(const FaultStatus *faults, const VehicleInput *input)
{
    int i;
    int active_count = count_active_faults(faults);
    int primary_idx = -1;

    (void)input;

    if (active_count == 0)
        return;

    if (log_file)
    fprintf(log_file, "-----\n[FAULT REPORT] %d fault(s) active this cycle "
                      "(listed highest -> lowest priority)\n",
            active_count);

    // Find the highest-priority active fault index
    for (i = 0; i < FAULT_BIT_COUNT; i++)
    {
        if (is_fault_active(faults, fault_priority_order[i]))
        {
            primary_idx = i;
            break;
        }
    }

    /*
     * Print all active faults in priority order.
     * The highest-priority fault is tagged [PRIMARY FAULT] (shown once).
     * Remaining faults are tagged [FAULT].
     * When only one fault is active, it is shown as [FAULT] (no primary tag needed).
     */
    for (i = 0; i < FAULT_BIT_COUNT; i++)
    {
        FaultBit bit = fault_priority_order[i];
        if (!is_fault_active(faults, bit))
            continue;

        const char *name = fault_to_string(bit);
        const char *cls = fault_classification(bit);
        int is_pri = (active_count > 1 && i == primary_idx);

        if (log_file)
            fprintf(log_file, "%s %-20s [%s]\n",
                    is_pri ? "[PRIMARY FAULT]" : "[FAULT]        ",
                    name, cls);
        printf("%s %s [%s]\n",
               is_pri ? "[PRIMARY FAULT]" : "[FAULT]",
               name, cls);
    }
}

void print_cycle_header(uint32_t cycle, const VehicleInput *input)
{
    printf("\n[CYCLE: %u]\n", cycle);
    printf("[INPUT] Mode=%-12s Speed=%-4d Temp=%-4d Gear=%d\n",
           mode_to_string(input->requested_mode),
           input->speed,
           input->temperature,
           input->gear);

    if (log_file)
    {
        fprintf(log_file, "\n[CYCLE: %u]\n", cycle);
        fprintf(log_file,
                "[INPUT] Mode=%-12s Speed=%-4d Temp=%-4d Gear=%d\n",
                mode_to_string(input->requested_mode),
                input->speed,
                input->temperature,
                input->gear);
    }
}


void log_cycle_summary(const VehicleInput *input, const VehicleStatus *status,
                       const FaultStatus *faults)
{
    if (!log_file)
        return;

    int i;

    fprintf(log_file, "❇️  ECU CYCLE #%-5u SUMMARY\n", status->cycle_count);
    fprintf(log_file, "-----\n");
    fprintf(log_file, "INPUTS\n");
    fprintf(log_file, "Speed       : %4d km/h [%s]\n",
            input->speed, input->speed_valid ? "OK " : "ERR");
    fprintf(log_file, "Temperature : %4d deg C [%s]\n",
            input->temperature, input->temp_valid ? "OK " : "ERR");
    fprintf(log_file, "Gear        : %4d       [%s]\n",
            input->gear, input->gear_valid ? "OK " : "ERR");
    fprintf(log_file, "Mode Req.   : %-12s [%s]\n",
            mode_to_string(input->requested_mode),
            input->mode_valid ? "OK " : "ERR");
    fprintf(log_file, "-----\n");

    fprintf(log_file, "STATUS\n");
    fprintf(log_file, "Mode        : %-12s\n", mode_to_string(status->current_mode));
    fprintf(log_file, "Sys State   : %-10s\n", state_to_string(status->system_state));
    fprintf(log_file, "-----\n");

    fprintf(log_file,
            "FAULTS (active=0x%02X persistent=0x%02X)\n",
            faults->active_flags, faults->persistent_flags);

    for (i = 0; i < FAULT_BIT_COUNT; i++)
    {
        FaultBit bit = (FaultBit)i;

        /* cnt is total lifetime occurrences of that fault since program start */
        fprintf(log_file,
                "%-20s cnt=%-4u active=%-3s persist=%-3s\n",
                fault_to_string(bit),
                faults->counters[i],
                is_fault_active(faults, bit) ? "YES" : "no ",
                is_fault_persistent(faults, bit) ? "YES" : "no ");
    }

    fprintf(log_file,
            "--------------------------------------------------\n");
    fflush(log_file);
}


void log_input_warning(const char *field, int value)
{
    if (log_file)
        fprintf(log_file,
                "[WARN] INPUT  : Invalid %s value = %d, preserving last valid\n",
                field, value);
}

void log_mode_transition(VehicleMode from, VehicleMode to, int is_illegal)
{
    if (!log_file)
        return;
    if (is_illegal)
    {
        fprintf(log_file,
                "[FAULT] MODE  : ILLEGAL transition %s -> %s => forcing FAULT mode\n",
                mode_to_string(from), mode_to_string(to));
        printf("[MODE] %s -> FAULT (illegal transition)\n",
               mode_to_string(from));
    }
    else
    {
        fprintf(log_file,
                "[INFO]  MODE  : Transition %s -> %s\n",
                mode_to_string(from), mode_to_string(to));
        printf("[MODE] %s -> %s\n",
               mode_to_string(from), mode_to_string(to));
    }
}

void log_state_transition(SystemState from, SystemState to, const char *reason)
{
    if (log_file)
        fprintf(log_file,
                "[STATE] SYSTEM : %s -> %s | Reason: %s\n",
                state_to_string(from), state_to_string(to), reason);

    printf("[STATE CHANGE] %s -> %s\n",
           state_to_string(from), state_to_string(to));
}

void log_fault_event(FaultBit bit, const char *detail)
{
    if (log_file)
        fprintf(log_file,
                "[FAULT] %-20s : %s\n",
                fault_to_string(bit), detail);
}

void log_warning(const char *message)
{
    if (log_file)
        fprintf(log_file, "[WARN]  %s\n", message);
}

void log_info(const char *message)
{
    if (log_file)
        fprintf(log_file, "[INFO]  %s\n", message);
}