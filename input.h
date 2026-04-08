#ifndef INPUT_H
#define INPUT_H

#include "types.h"

/*
 * Reads raw vehicle inputs.
 * In this hackathon version, inputs are simulated via test cases.
 */
void read_inputs(VehicleInput *input, int testCase);

/*
 * Validates raw inputs against defined limits.
 * Invalid values are corrected safely (no crashes).
 */
void validate_inputs(VehicleInput *input);

#endif /* INPUT_H */