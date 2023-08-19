/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_UTILS_H
#define NIMBLE_CLIENT_UTILS_H

#include <nimble-steps/types.h>
#include <stdbool.h>
#include <stddef.h>

struct NimbleClient;

bool nimbleClientOptimalStepIdToSend(const struct NimbleClient* self, StepId* outStepId);

#endif
