/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <monotonic-time/lower_bits.h>
#include <nimble-client/client.h>
#include <nimble-client/pong.h>
#include <nimble-client/game_step_response.h>
#include <nimble-steps-serialize/pending_in_serialize.h>

/// Handle game step response (`NimbleSerializeCmdGameStepResponse`) from server.
/// Stream contains authoritative Steps from the server.
/// @param self nimble protocol client
/// @param inStream stream to read from
/// @return negative on error
ssize_t nimbleClientOnGameStepResponse(NimbleClient* self, FldInStream* inStream)
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

    LagometerPacket packet = {LagometerPacketStatusReceived, self->latencyMs, inStream->size};
    lagometerAddPacket(&self->lagometer, packet);

    uint32_t serverReceivedPredictedStepId;
    fldInStreamReadUInt32(inStream, &serverReceivedPredictedStepId);
    if (serverReceivedPredictedStepId != self->receivedStepIdByServerOnlyForDebug) {
        self->receivedStepIdByServerOnlyForDebug = serverReceivedPredictedStepId;
    }

    CLOG_C_VERBOSE(&self->log, "server has received predicted step %08X, discarding out steps before that", serverReceivedPredictedStepId)

    nbsStepsDiscardUpTo(&self->outSteps, serverReceivedPredictedStepId + 1);

    ssize_t stepCount = nbsPendingStepsInSerialize(inStream, &self->authoritativePendingStepsFromServer);
    if (stepCount < 0) {
        CLOG_C_SOFT_ERROR(&self->log, "GameStepResponse: nbsPendingStepsInSerialize() failed %zd", stepCount)
        return stepCount;
    }

    int copyResult = nbsPendingStepsCopy(&self->authoritativeStepsFromServer,
                                         &self->authoritativePendingStepsFromServer);
    if (copyResult < 0) {
        CLOG_C_ERROR(&self->log, "nbsPendingStepsCopy failed: %d", copyResult)
        // return copyResult;
    }

    statsIntAdd(&self->waitingStepsFromServer, (int) self->authoritativeStepsFromServer.stepsCount);

    //if (stepCount > 0) {
        nimbleClientConnectionQualityReceivedAuthoritativeSteps(&self->quality, (size_t) stepCount);
        statsIntPerSecondAdd(&self->simulationStepsPerSecond, (int) stepCount);
    //}

#if 1
    nbsStepsDebugOutput(&self->authoritativeStepsFromServer, "authoritative steps from server after in serialize", 0);
    //CLOG_C_VERBOSE(&self->log, "authoritative received steps count: %zd (buffer size: %zu)", stepCount,
      //             self->authoritativeStepsFromServer.stepsCount)
#endif

    return stepCount;
}
