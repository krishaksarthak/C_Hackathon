/*
 * File        : main.c
 * Author      : C10 Team
 * Date        : 09/04/2026
 * Description : Vehicle ECU Simulator — main scheduler loop.
 *
 * ── Execution order (fixed, every cycle) ────────────────────────────────
 *   1. read_inputs()           — raw sensor read + quit/reset sentinel
 *   2. validate_inputs()       — range checks; substitutes fallback values
 *   3. update_mode()           — legal transition check; forces FAULT mode
 *                                on illegal transition (CRITICAL)
 *   4. run_control_checks()    — overspeed / temperature / gear; critical
 *                                faults force FAULT mode here as well
 *   5. update_fault_status()   — promotes consecutive faults to persistent
 *   6. evaluate_system_state() — NORMAL / DEGRADED / SAFE determination
 *   7. log_faults_prioritised()— priority-ordered fault report to console
 *   8. log_cycle_summary()     — full structured summary to log file
 *
 * Why this order?
 *   Inputs must be read and validated before they can drive any decision.
 *   Mode must be updated before control checks so that an illegal mode
 *   transition (which forces FAULT) is visible to the state evaluator in
 *   the same cycle.  Fault status promotion and state evaluation run last
 *   so they see the complete fault picture for this cycle.  Changing the
 *   order can cause a one-cycle delay in detecting or escalating faults,
 *   which is unacceptable in a safety-critical ECU.
 *
 * ── Reset / recovery behaviour ──────────────────────────────────────────
 *   When the system reaches STATE_SAFE (critical fault or major fault
 *   threshold), reset_required is set and the normal cycle is gated.
 *   The operator must enter -1 to call init_system(), which zeroes all
 *   state and restarts from Cycle #1.  Entering anything else while in
 *   SAFE keeps the gate active.
 *
 *   The -1 sentinel also works during normal operation (not in SAFE):
 *   it triggers the same re-initialisation, acting as a soft power cycle.
 *
 * ── Log file ────────────────────────────────────────────────────────────
 *   One file per run, named with the session start timestamp.
 *   Format: logs/YYYY-MM-DD_HH-MM-SS.txt
 *   A new run always creates a new file — nothing is overwritten.
 *
 * MISRA C:2012 compliance notes
 *   • Infinite loop expressed as for(;;) (Rule 14.4 advisory).
 *   • All scanf return values checked (Rule 17.7).
 *   • memset return cast to void (Rule 17.7).
 *   • No dynamic allocation (Rule 21.3).
 *   • All variables initialised at declaration (Rule 9.1).
 *   • Static helper functions declared before use (Rule 8.4).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "types.h"
#include "input.h"
#include "mode.h"
#include "control.h"
#include "fault.h"
#include "state.h"
#include "log.h"

/* Global log file handle — declared extern in log.c */
FILE *log_file = NULL;

/* ── Static helpers (file scope only) ───────────────────────────────────── */

/*
 * reset_input_defaults()
 *   Restores the VehicleInput struct to a known-safe state after a reset.
 *   Keeps the ECU from operating on stale sensor values from the previous run.
 */
static void reset_input_defaults(VehicleInput *input)
{
    input->speed          = 0;
    input->temperature    = 20;
    input->gear           = 0;
    input->requested_mode = MODE_OFF;
    input->speed_valid    = 1U;
    input->temp_valid     = 1U;
    input->gear_valid     = 1U;
    input->mode_valid     = 1U;
}

/*
 * init_system()
 *   Full ECU re-initialisation: zeroes all runtime state and faults,
 *   then sets safe defaults.  Called at startup and on every -1 reset.
 */
static void init_system(VehicleStatus *status, FaultStatus *faults)
{
    (void)memset(status, 0, sizeof(VehicleStatus));
    (void)memset(faults, 0, sizeof(FaultStatus));

    status->current_mode   = MODE_OFF;
    status->previous_mode  = MODE_OFF;
    status->system_state   = STATE_NORMAL;
    status->previous_state = STATE_NORMAL;
    status->cycle_count    = 0U;
    status->reset_required = 0U;

    status->last_valid_speed = 0;
    status->last_valid_temp  = 20;
    status->last_valid_gear  = 0;

    log_info("System initialised — Mode: OFF, State: NORMAL");
}

/*
 * open_log_file()
 *   Creates the timestamped log file in the logs/ directory.
 *   Sets the global log_file handle.  Returns 0 on success, 1 on failure.
 */
static int open_log_file(char *filename, size_t size)
{
    time_t     now = time(NULL);
    struct tm *t   = localtime(&now);
    int        rc  = 0;

    (void)strftime(filename, size, "logs/%Y-%m-%d_%H-%M-%S.txt", t);
    log_file = fopen(filename, "w");

    if (log_file == NULL)
    {
        printf("ERROR: Could not open %s. "
               "Ensure the 'logs' folder exists.\n", filename);
        rc = 1;
    }

    return rc;
}

/* ── Main ───────────────────────────────────────────────────────────────── */
int main(void)
{
    VehicleInput  input       = {0};
    VehicleStatus status      = {0};
    FaultStatus   faults      = {0};
    char          log_fn[80]  = {0};
    int           sentinel    = 0;

    if (open_log_file(log_fn, sizeof(log_fn)) != 0)
    {
        return 1;
    }

    init_system(&status, &faults);
    reset_input_defaults(&input);

    printf("  Vehicle ECU Simulator\n");
    printf("####################################################\n");
    printf("Log file : %s\n", log_fn);
    printf("Enter each field when prompted.\n");
    printf("Type -1 at any Mode prompt to reset / power-cycle the ECU.\n");
    printf("Type -2 to QUIT the simulator completely.\n");
    printf("####################################################\n\n");

    for (;;)   /* ECU runs indefinitely — for(;;) preferred over while(1) */
    {
        /*  SAFE STATE GATE                                               *
         *  When reset_required is set, the normal cycle is suspended.   *
         *  Only -1 is accepted; anything else keeps the system locked.  */
        if (status.reset_required != 0U)
        {
            printf("\n");
            printf("--- [!! SAFE STATE]  ECU LOCKED — MANUAL RESET NEEDED ---------\n");
            printf("--- Enter -1 to re-initialise the ECU, or -2 to QUIT. ---------\n");
            printf("  > ");
            (void)fflush(stdout);

            if (scanf("%d", &sentinel) == 1)
            {
                if (sentinel == -1)
                {
                    printf("\n[RESET] Operator reset received. Re-initialising ECU...\n");
                    log_info("Operator reset — re-initialising system from SAFE state.");
                    init_system(&status, &faults);
                    reset_input_defaults(&input);
                    printf("[RESET] ECU re-initialised. Restarting from Cycle #1.\n");
                    printf("####################################################\n\n");
                }
                else if (sentinel == -2)
                {
                    printf("\n[QUIT] Operator quit received. Exiting simulator...\n");
                    log_info("Quit sentinel (-2) received in SAFE state. Exiting.");
                    break; /* Breaks out of the for(;;) loop */
                }
                else
                {
                    printf("[!] Input rejected. System remains in SAFE state.\n");
                }
            }
            continue;
        }

        /* ══════════════════════════════════════════════════════════════ *
         *  NORMAL CYCLE EXECUTION                                        *
         * ══════════════════════════════════════════════════════════════ */
        status.cycle_count++;
        printf("--- Cycle #%u ---\n", status.cycle_count);

        /* Step 1: Read raw inputs */
        read_inputs(&input);

        /* Soft-reset sentinel (-1) or Quit sentinel (-2) */
        if ((int)input.requested_mode == -1)
        {
            printf("\n[RESET] User-initiated reset. Re-initialising ECU...\n");
            log_info("User-initiated reset (-1 sentinel) — re-initialising.");
            init_system(&status, &faults);
            reset_input_defaults(&input);
            printf("[RESET] ECU re-initialised. Restarting from Cycle #1.\n");
            printf("####################################################\n\n");
            continue;
        }
        else if ((int)input.requested_mode == -2)
        {
            printf("\n[QUIT] User-initiated quit. Exiting simulator...\n");
            log_info("Quit sentinel (-2) received. Exiting.");
            break; /* Breaks out of the for(;;) loop */
        }

        print_cycle_header(status.cycle_count, &input);

        /* Step 2: Validate inputs (substitutes fallback values where needed) */
        validate_inputs(&input, &status);

        /* Step 3: Update mode (illegal transition forces MODE_FAULT + CRITICAL) */
        update_mode(&status, &input, &faults);

        /* Step 4: Control logic (critical faults also force MODE_FAULT here) */
        run_control_checks(&input, &status, &faults);

        /* Step 5: Promote persistent faults */
        update_fault_status(&faults);

        /* Step 6: Determine system state (sets reset_required if STATE_SAFE) */
        evaluate_system_state(&status, &faults);

        /* Step 7: Fault report to console (priority-ordered) */
        log_faults_prioritised(&faults, &input);

        /* Step 8: Full cycle summary to log file */
        log_cycle_summary(&input, &status, &faults);

        printf("-> Cycle #%u done. State=%-10s Mode=%s\n",
               status.cycle_count,
               state_to_string(status.system_state),
               mode_to_string(status.current_mode));

        /* Notify operator immediately if this cycle triggered a lock */
        if (status.reset_required != 0U)
        {
            printf("\n[!! SAFE STATE] ECU locked this cycle.\n"
                   "    Enter -1 at the next prompt to reset.\n");
        }

        printf("\n");
    }

    /*
     * Unreachable in normal ECU operation — the loop above never exits.
     * Present for MISRA Rule 15.5 compliance (function has a reachable return).
     */
    if (log_file != NULL)
    {
        (void)fclose(log_file);
        log_file = NULL;
    }

    return 0;
}