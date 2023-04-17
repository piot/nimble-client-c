/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <nimble-client/game_step_response.h>
#include <nimble-client/client.h>
#include <flood/in_stream.h>
#include <nimble-steps-serialize/pending_in_serialize.h>

/// Handle game step response (`NimbleSerializeCmdGameStepResponse`) from server.
/// Stream contains authoritative Steps from the server.
/// @param self
/// @param inStream
/// @return
int nimbleClientOnGameStepResponse(NimbleClient* self, FldInStream* inStream)
{
    uint8_t stepCountInIncomingBufferOnServer;
    fldInStreamReadUInt8(inStream, &stepCountInIncomingBufferOnServer);

    if (self->useStats) {
        statsIntAdd(&self->stepCountInIncomingBufferOnServerStat, stepCountInIncomingBufferOnServer);
    }

    int8_t deltaAgainstServerAuthoritativeBuffer;
    fldInStreamReadInt8(inStream, &deltaAgainstServerAuthoritativeBuffer);

    if (self->useStats) {
        statsIntAdd(&self->authoritativeBufferDeltaStat, deltaAgainstServerAuthoritativeBuffer);
    }

    uint32_t receivedStepIdFromRemote;
    fldInStreamReadUInt32(inStream, &receivedStepIdFromRemote);
    if (receivedStepIdFromRemote != self->receivedStepIdByServerOnlyForDebug) {
#if 1
        // CLOG_DEBUG("stepId received by server: %08X", receivedStepIdFromRemote);
#endif
        self->receivedStepIdByServerOnlyForDebug = receivedStepIdFromRemote;
    }

    CLOG_VERBOSE("nimbleClient: gameStep: received from server %08X", receivedStepIdFromRemote)

    int stepCount = nbsPendingStepsInSerialize(inStream, &self->authoritativePendingStepsFromServer);
    if (stepCount < 0) {
        CLOG_SOFT_ERROR("GameStepResponse: nbsPendingStepsInSerialize() failed %d", stepCount)
        return stepCount;
    }

    int copyResult = nbsPendingStepsCopy(&self->authoritativeStepsFromServer,
                                         &self->authoritativePendingStepsFromServer);
    if (copyResult < 0) {
        CLOG_ERROR("nbsPendingStepsCopy failed: %d", copyResult)
    }

    statsIntAdd(&self->waitingStepsFromServer, (int)self->authoritativeStepsFromServer.stepsCount);

    if (stepCount > 0) {
        //        CLOG_DEBUG("last added into")
        // nbsStepsDiscardIncluding(&self->outSteps, lastAddedId);
        statsIntPerSecondAdd(&self->simulationStepsPerSecond, stepCount);
    }

#if 1
    nbsStepsDebugOutput(&self->authoritativeStepsFromServer, "authoritative steps from server", 0);
    CLOG_DEBUG("authoritative received steps count: %d (buffer size: %zu)", stepCount,
               self->authoritativeStepsFromServer.stepsCount);
#endif

    return stepCount;
}
