/*
 * File:        main.c
 * Author:      C10 Team
 * Date:        09/04/2026
 * Description: Main entry point of Vehicle ECU Simulator
 *
 * Log behaviour:
 *   One log file per program run, named with the session start timestamp.
 *   Format: logs/YYYY-MM-DD_HH-MM-SS.txt
 *   All cycles in a run are written to the same file.
 *   A new run always creates a new file, nothing is ever overwritten.
 *
 * Quit: enter -1 at the Mode prompt, no further prompts appear.
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

FILE *log_file = NULL;

static void init_system(VehicleStatus *status, FaultStatus *faults)
{
    memset(status, 0, sizeof(VehicleStatus));
    memset(faults, 0, sizeof(FaultStatus));

    status->current_mode   = MODE_OFF;
    status->previous_mode  = MODE_OFF;
    status->system_state   = STATE_NORMAL;
    status->previous_state = STATE_NORMAL;
    status->cycle_count    = 0;

    status->last_valid_speed = 0;
    status->last_valid_temp  = 20;
    status->last_valid_gear  = 0;

    log_info("System initialised — Mode: OFF, State: NORMAL");
}


int main(void)
{
    VehicleInput  input  = {0};
    VehicleStatus status = {0};
    FaultStatus   faults = {0};
    char          log_filename[80];

    {
        time_t     now = time(NULL);
        struct tm *t   = localtime(&now);
        strftime(log_filename, sizeof(log_filename),
                 "logs/%Y-%m-%d_%H-%M-%S.txt", t);

        log_file = fopen(log_filename, "w");
        if (log_file == NULL)
        {
            printf("ERROR: Could not open %s. "
                   "Ensure the 'logs' folder exists.\n", log_filename);
            return 1;
        }
    }

    init_system(&status, &faults);

    input.speed          = 0;
    input.temperature    = 20;
    input.gear           = 0;
    input.requested_mode = MODE_OFF;
    input.speed_valid    = 1;
    input.temp_valid     = 1;
    input.gear_valid     = 1;
    input.mode_valid     = 1;

    printf("  Vehicle ECU Simulator\n");
    printf("####################################################\n");
    printf("Log file : %s\n", log_filename);
    printf("Enter each field when prompted.\n");
    printf("Type -1 at the Mode prompt to quit.\n");
    printf("####################################################\n");


    while (1)
    {
        status.cycle_count++;

        printf("--- Cycle #%u ---\n", status.cycle_count);
        read_inputs(&input);

        if ((int)input.requested_mode == -1)
        {
            log_info("Quit sentinel received — shutting down.");
            printf("Exiting Program.\n");
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

        printf("-> Cycle #%u done. State=%-10s Mode=%s\n\n",
               status.cycle_count,
               state_to_string(status.system_state),
               mode_to_string(status.current_mode));
    }

    if (log_file != NULL)
    {
        fclose(log_file);
        log_file = NULL;
    }

    return 0;
}
