/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_INCOMING_API_H
#define NIMBLE_CLIENT_INCOMING_API_H

struct NimbleClient;

#include <nimble-steps/steps.h>

int nimbleClientReadStep(struct NimbleClient* self, uint8_t* target, size_t maxTarget, StepId* outStepId);

#endif
