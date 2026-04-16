// File        : main.c
// Author      : C10 Team
// Date        : 09/04/2026
// Description : Vehicle ECU Simulator - main scheduler loop.

// Execution order
// 1. read_inputs()           — raw sensor read + quit/reset sentinel
// 2. validate_inputs()       — range checks; substitutes fallback values
// 3. update_mode()           — legal transition check; forces FAULT mode on illegal transition (CRITICAL)
// 4. run_control_checks()    — overspeed / temperature / gear; critical faults force FAULT mode here as well
// 5. update_fault_status()   — promotes consecutive faults to persistent
// 6. evaluate_system_state() — NORMAL / DEGRADED / SAFE determination
// 7. log_faults_prioritised()— priority-ordered fault report to console
// 8. log_cycle_summary()     — full structured summary to log file

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

// Global File handler
FILE *log_file = NULL;

// Resets the input values
static void reset_input_defaults(VehicleInput *input)
{
    input->speed = 0;
    input->temperature = 20;
    input->gear = 0;
    input->requested_mode = MODE_OFF;
    input->speed_valid = 1U;
    input->temp_valid = 1U;
    input->gear_valid = 1U;
    input->mode_valid = 1U;
}

// This initializes the system
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

// Creates log file with
static int open_log_file(char *filename, size_t size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int rc = 0;

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

// Main function
int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
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

    for (;;)
    {
        // This is safe state execution condition
        if (status.reset_required != 0U)
        {
            printf("\n");
            printf("--- [!! SAFE STATE]  ECU LOCKED - MANUAL RESET NEEDED ---------\n");
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
                    break;
                }
                else
                {
                    printf("[!] Input rejected. System remains in SAFE state.\n");
                }
            }
            continue;
        }

        // This is a normal cycle execution condition
        status.cycle_count++;
        printf("--- Cycle #%u ---\n", status.cycle_count);

        read_inputs(&input);

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
            break;
        }

        print_cycle_header(status.cycle_count, &input);

        validate_inputs(&input, &status);

        update_mode(&status, &input, &faults);

        run_control_checks(&input, &status, &faults);

        update_fault_status(&faults);

        evaluate_system_state(&status, &faults);

        log_faults_prioritised(&faults, &input);

        log_cycle_summary(&input, &status, &faults);

        printf("-> Cycle #%u done. State=%-10s Mode=%s\n",
               status.cycle_count,
               state_to_string(status.system_state),
               mode_to_string(status.current_mode));

        if (status.reset_required != 0U)
        {
            printf("\n[!! SAFE STATE] ECU locked this cycle.\n"
                   "    Enter -1 at the next prompt to reset.\n");
        }

        printf("\n");
    }

    if (log_file != NULL)
    {
        (void)fclose(log_file);
        log_file = NULL;
    }

    return 0;
}