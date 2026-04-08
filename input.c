#include <stdio.h>
#include "input.h"

/*
 * read_inputs
 * -----------
 * Simulates vehicle inputs for each mandatory test case.
 * Each call represents one ECU cycle.
 */
void read_inputs(VehicleInput *input, int testCase)
{
    if (input == 0)
    {
        return; /* Defensive check */
    }

    switch (testCase)
    {
        /* Test Case 1: Normal Start (OFF -> ACC) */
        case 1:
            input->speed = 40;
            input->temperature = 80;
            input->gear = 1;
            input->requestedMode = MODE_ACC;
            break;

        /* Test Case 2: Overspeed */
        case 2:
            input->speed = 130;
            input->temperature = 85;
            input->gear = 4;
            input->requestedMode = MODE_IGNITION_ON;
            break;

        /* Test Case 3: High Temperature */
        case 3:
            input->speed = 60;
            input->temperature = 100;
            input->gear = 2;
            input->requestedMode = MODE_IGNITION_ON;
            break;

        /* Test Case 4: Critical Overheat */
        case 4:
            input->speed = 70;
            input->temperature = 115;
            input->gear = 3;
            input->requestedMode = MODE_IGNITION_ON;
            break;

        /* Test Case 5: Invalid Gear */
        case 5:
            input->speed = 50;
            input->temperature = 90;
            input->gear = 9; /* Invalid */
            input->requestedMode = MODE_IGNITION_ON;
            break;

        /* Test Case 6: Illegal Mode Transition (OFF -> IGNITION_ON) */
        case 6:
            input->speed = 0;
            input->temperature = 30;
            input->gear = 0;
            input->requestedMode = MODE_IGNITION_ON;
            break;

        /* Test Case 7: Multiple Faults in One Cycle */
        case 7:
            input->speed = 140;       /* Overspeed */
            input->temperature = 120; /* Critical Overheat */
            input->gear = 9;          /* Invalid Gear */
            input->requestedMode = MODE_IGNITION_ON;
            break;

        /* Test Case 8: Persistent Fault (repeat critical fault) */
        case 8:
            input->speed = 145;
            input->temperature = 118;
            input->gear = 3;
            input->requestedMode = MODE_IGNITION_ON;
            break;

        /* Test Case 9: Recovery Scenario */
        case 9:
            input->speed = 60;
            input->temperature = 80;
            input->gear = 2;
            input->requestedMode = MODE_ACC;
            break;

        default:
            /* No change */
            break;
    }
}

/*
 * validate_inputs
 * ---------------
 * Validates input ranges as per problem statement:
 *  - Speed: 0 to 200
 *  - Temperature: -40 to 150
 *  - Gear: 0 to 5
 *
 * Invalid values are corrected to safe defaults.
 */
void validate_inputs(VehicleInput *input)
{
    if (input == 0)
    {
        return;
    }

    /* Speed validation */
    if (input->speed < 0 || input->speed > 200)
    {
        printf("Warning: Invalid speed input detected\n");
        input->speed = 0;
    }

    /* Temperature validation */
    if (input->temperature < -40 || input->temperature > 150)
    {
        printf("Warning: Invalid temperature input detected\n");
        input->temperature = 25;
    }

    /* Gear validation */
    if (input->gear < 0 || input->gear > 5)
    {
        /* Do not correct here – control logic must detect this fault */
        printf("Warning: Invalid gear input detected\n");
    }
}