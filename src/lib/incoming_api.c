/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <nimble-client/client.h>

int nimbleClientReadStep(NimbleClient* self, uint8_t* target, size_t maxTarget, StepId* outStepId)
{
    return nbsStepsRead(&self->authoritativeStepsFromServer, outStepId, target, maxTarget);
}

