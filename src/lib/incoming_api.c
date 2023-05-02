/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <nimble-client/client.h>

/// Reads a single combined authoritative steps that has been received from the nimble server.
/// @param self
/// @param target target buffer that receives the combined authoritative step.
/// @param maxTarget maximum number of octets in target buffer.
/// @param outStepId the tickId for when the input should be applied to before simulation tick().
/// @return negative values on error.
int nimbleClientReadStep(NimbleClient* self, uint8_t* target, size_t maxTarget, StepId* outStepId)
{
    return nbsStepsRead(&self->authoritativeStepsFromServer, outStepId, target, maxTarget);
}
