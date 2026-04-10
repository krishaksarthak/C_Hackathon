#include <stdio.h>
#include "input.h"
#include "log.h"

// Reads raw values from the user (or from stdin).
void read_inputs(VehicleInput *input){
    int mode_raw = 0;

    printf("\nEnter inputs (speed temp gear mode [0=OFF,1=ACC,2=IGN,3=FAULT] ): ");

    if (scanf("%d %d %d %d", &input->speed, &input->temperature, &input->gear, &mode_raw) != 4){
        log_warning("read_inputs: failed to read inputs, preserving previous values");
        return;
    }

    input->requested_mode = (VehicleMode)mode_raw;
}

// Validating the raw inputs
void validate_inputs(VehicleInput *input, VehicleStatus *status){

    // Speed
    if (input->speed >= MIN_SPEED && input->speed <= MAX_SPEED){
        input->speed_valid          = 1;
        status->last_valid_speed    = input->speed;
    }
    else{
        input->speed_valid  = 0;
        input->speed        = status->last_valid_speed; 
        log_input_warning("Speed", input->speed);
    }

    // Temperature
    if (input->temperature >= MIN_TEMP && input->temperature <= MAX_TEMP){
        input->temp_valid           = 1;
        status->last_valid_temp     = input->temperature;
    }
    else{
        input->temp_valid   = 0;
        input->temperature  = status->last_valid_temp;
        log_input_warning("Temperature", input->temperature);
    }

    // Gear
    if (input->gear >= MIN_GEAR && input->gear <= MAX_GEAR){
        input->gear_valid           = 1;
        status->last_valid_gear     = input->gear;
    }
    else{
        input->gear_valid   = 0;
        // Do NOT restore last good gear here, invalid gear must still be visible to the control layer so it can raise a fault. We only preserve for display purposes after fault is raised.
    }

    // Mode
    if (input->requested_mode >= MODE_OFF && input->requested_mode < MODE_COUNT){
        input->mode_valid = 1;
    }
    else{
        input->mode_valid           = 0;
        input->requested_mode       = status->current_mode;
        log_input_warning("Mode (out of enum range)", (int)input->requested_mode);
    }
}