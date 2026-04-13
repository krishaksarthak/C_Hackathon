/*
 * File:        main.c
 * Author:      C10 Team
 * Date:        09/04/2026
 * Description: Main entry point of Vehicle ECU Simulator
 * Expected console output for that input (cycle 1):
 *   [CYCLE: 1]
 *   [INPUT] Mode=IGNITION_ON  Speed=130 Temp=115 Gear=4
 *   [FAULT] OVERTEMPERATURE : CRITICAL OVERHEAT
 *   [FAULT] OVERSPEED       : Speed 130 km/h exceeds limit 120 km/h [MAJOR]
 *   [PRIMARY FAULT] OVERTEMPERATURE [CRITICAL]
 *   [FAULT]         OVERSPEED       [MAJOR]
 *   [STATE CHANGE] NORMAL -> DEGRADED
 *
 * Enter -1 -1 -1 -1 to quit.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

    log_file = fopen("logs/logs.txt", "w");
    if (log_file == NULL){
        printf("ERROR: Could not open logs/logs.txt. "
               "Please ensure the 'logs' folder exists.\n");
        return 1;
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

    printf("Vehicle ECU Simulator\n");
    printf("____________________________________________________\n");
    printf("Log file : logs/logs.txt\n");
    printf("Enter each field when prompted. Type -1 to quit.\n");
    printf("____________________________________________________\n");

    while (1){
        status.cycle_count++;

        printf("--- Cycle #%u ---\n", status.cycle_count);
        read_inputs(&input);

        if (input.speed == -1 && input.temperature == -1 && input.gear  == -1 && (int)input.requested_mode == -1){
            log_info("Quit sentinel received — shutting down.");
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
        fclose(log_file);

    return 0;
}