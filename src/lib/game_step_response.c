/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/game_step_response.h>
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
        self->receivedStepIdByServerOnlyForDebug = receivedStepIdFromRemote;
    }

    CLOG_C_VERBOSE(&self->log, "gameStep: received from server %08X", receivedStepIdFromRemote)

    int stepCount = nbsPendingStepsInSerialize(inStream, &self->authoritativePendingStepsFromServer);
    if (stepCount < 0) {
        CLOG_C_SOFT_ERROR(&self->log, "GameStepResponse: nbsPendingStepsInSerialize() failed %d", stepCount)
        return stepCount;
    }

    int copyResult = nbsPendingStepsCopy(&self->authoritativeStepsFromServer,
                                         &self->authoritativePendingStepsFromServer);
    if (copyResult < 0) {
        CLOG_C_ERROR(&self->log, "nbsPendingStepsCopy failed: %d", copyResult)
        return copyResult;
    }

    statsIntAdd(&self->waitingStepsFromServer, (int) self->authoritativeStepsFromServer.stepsCount);

    if (stepCount > 0) {
        statsIntPerSecondAdd(&self->simulationStepsPerSecond, stepCount);
    }

#if 1
    nbsStepsDebugOutput(&self->authoritativeStepsFromServer, "authoritative steps from server", 0);
    CLOG_C_VERBOSE(&self->log, "authoritative received steps count: %d (buffer size: %zu)", stepCount,
                   self->authoritativeStepsFromServer.stepsCount);
#endif

    return stepCount;
}
