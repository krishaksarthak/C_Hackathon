/*
 * File: main.c
 * Author: C10 Team
 * Date: 09/04/2026
 * Description: Main Entry point
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

/*
* Global File pointer
*/
FILE *log_file = NULL;

static void init_system(VehicleStatus *status, FaultStatus *faults) {
    // this fills every byte of both structs with zero. 
    memset(status, 0, sizeof(VehicleStatus));
    memset(faults, 0, sizeof(FaultStatus));

    status->current_mode     = MODE_OFF;
    status->previous_mode    = MODE_OFF;
    status->system_state     = STATE_NORMAL;
    status->previous_state   = STATE_NORMAL;
    status->cycle_count      = 0;

    // If the very first input is invalid the system needs a safe fallback, These are the safest possible starting assumptions for a parked vehicle.
    status->last_valid_speed = 0;
    status->last_valid_temp  = 20;
    status->last_valid_gear  = 0;

    log_info("System initialised - Mode: OFF, State: NORMAL");
}

int main(void) {
    VehicleInput  input  = {0};
    VehicleStatus status = {0};
    FaultStatus   faults = {0};

    log_file = fopen("logs/logs.txt", "w");

    // Error handling for file
    if (log_file == NULL) {
        printf("ERROR: Could not open logs/logs.txt. Please ensure the 'logs' folder exists in the project root.\n");
        return 1;
    }

    init_system(&status, &faults);

    // Before the loop starts, input is given safe default values
    input.speed           = 0;
    input.temperature     = 20;
    input.gear            = 0;
    input.requested_mode  = MODE_OFF;
    input.speed_valid     = 1;
    input.temp_valid      = 1;
    input.gear_valid      = 1;
    input.mode_valid      = 1;

    printf("\nVehicle ECU Simulator\n");
    printf("Logging active: writing to logs/logs.txt\n");
    printf("Enter inputs each cycle: <speed> <temp> <gear> <mode>\n");
    printf("Modes: 0=OFF  1=ACC  2=IGNITION_ON  3=FAULT\n");
    printf("Valid speed: 0-200 | temp: -40 to 150 | gear: 0-5\n");
    printf("Enter -1 -1 -1 -1 to quit\n\n");

    // Main Scheduler Loop
    while (1) {
        status.cycle_count++;

        /*
        * Printing Cycle Count
        */
        printf("Enter inputs for Cycle #%u: ", status.cycle_count);
        read_inputs(&input);

        /*
        * Quit sentinel - if all inputs are -1, we exit the loop and end the program. This allows for graceful shutdown and ensures the log file is properly closed.
        */
        if (input.speed == -1 && input.temperature == -1 && input.gear  == -1 && (int)input.requested_mode == -1) {
            log_info("Quit sentinel received - shutting down.");
            break; // Exiting the while loop
        }

        /*
        * The Fixed Scheduler Order
        */
        validate_inputs(&input, &status);
        update_mode(&status, &input, &faults);
        run_control_checks(&input, &status, &faults);
        update_fault_status(&faults);
        evaluate_system_state(&status, &faults);
        log_cycle_summary(&input, &status, &faults);

        printf("✅ Cycle #%u complete. Log entry added to logs/logs.txt\n", status.cycle_count);
    }

    /*
    * After exiting the loop, we close the log file to ensure all data is flushed to disk and resources are cleaned up properly and also checking for the NULL check because if fopen failed earlier, log_file is still NULL and calling fclose(NULL) would crash.
    */
    if (log_file != NULL) {
        fclose(log_file);
    }

    return 0;
}