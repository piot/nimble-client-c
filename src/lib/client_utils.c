/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <nimble-client/client.h>
#include <nimble-client/utils.h>

/// Calculates the optimal number of prediction ticks that should be in the
/// outgoing prediction step buffer
/// @param self nimble client
/// @param outDiff the number of prediction ticks ahead of the last received authoritative step.
/// @return false if an estimation could not be made, true if successful.
static bool calculateOptimalSendPredictionTickCount(const NimbleClient* self, size_t* outDiff)
{
    bool hasLatencyStat = self->latencyMsStat.avgIsSet;

    // Latency is so critical to estimate the prediction tick count, so we can
    // not make an estimation without valid latency information.
    if (!hasLatencyStat) {
        if (self->loggingTickCount % 4 == 0) {
            CLOG_C_VERBOSE(&self->log, "no latency average yet. can not calculate optimal prediction tick count")
        }
        return false;
    }

    int latency = self->latencyMsStat.avg;
    if (latency < 0) {
        latency = 0;
    }

    size_t unsignedLatency = (size_t) latency;

    // We need our predicted step to reach the server at the exact (or a bit ahead of)
    // the moment the server compiles a combined authoritative step.
    // The server
    // is currently at or about the client received stepId + latency/2. And we need to
    // add `latencyRoundTrip/2` so the predicted stepId isn't too old
    // when it finally reaches the server.
    //
    // TL;DR: the optimal count is round trip latency in ticks rounded up.
    size_t latencyInTicksRoundedUp = (unsignedLatency / self->expectedTickDurationMs) + 1U;
    size_t optimalSendPredictionTickCount = latencyInTicksRoundedUp;

    // Buffer delta is certainly useful, but it is possible to make an estimation
    // without any buffer delta from the server.
    // We need to offset the latency calculation with the delta buffer information that is received from the server.
    bool hasBufferDeltaAverage = self->authoritativeBufferDeltaStat.avgIsSet;
    const size_t maximumBufferDeltaInfluenceInTicks = 5;

    int bufferDeltaAverage = 0;

    if (hasBufferDeltaAverage) {
        bufferDeltaAverage = self->authoritativeBufferDeltaStat.avg;
    } else {
        if (self->loggingTickCount % 4 == 0) {
            CLOG_C_VERBOSE(&self->log, "no buffer delta average determined yet.")
        }
    }

    const int minimumBufferDelta = 2;
    size_t bufferDeltaAddTickCount = 0;
    if (bufferDeltaAverage < minimumBufferDelta) {
        bufferDeltaAddTickCount = (size_t) (minimumBufferDelta - bufferDeltaAverage);
        if (bufferDeltaAddTickCount > maximumBufferDeltaInfluenceInTicks) {
            bufferDeltaAddTickCount = maximumBufferDeltaInfluenceInTicks;
        }
    }

    optimalSendPredictionTickCount += bufferDeltaAddTickCount;

    if (self->loggingTickCount % 60 == 0) {
        CLOG_C_VERBOSE(&self->log, "latency: %zu ms (count:%zu). bufferDelta:%d (count:%zu). totalTickCount:%zu",
                     unsignedLatency, latencyInTicksRoundedUp, bufferDeltaAverage, bufferDeltaAddTickCount,
                     optimalSendPredictionTickCount)
    }

    *outDiff = optimalSendPredictionTickCount;

    return true;
}

/// Calculates the optimal stepId (tickId) that should be sent to the server right now
/// @param self nimbleClient
/// @param[out] outStepId the stepId that should be sent
/// @retval true if an optimal StepId could be calculated
/// @retval false an optimal StepId could not be determined
bool nimbleClientOptimalStepIdToSend(const NimbleClient* self, StepId* outStepId, size_t* outDiff)
{
    bool worked = calculateOptimalSendPredictionTickCount(self, outDiff);
    if (!worked) {
        *outStepId = NIMBLE_STEP_MAX;
        return false;
    }

    StepId lastReceivedAuthoritativeStepId = self->authoritativeStepsFromServer.expectedWriteId;
    *outStepId = lastReceivedAuthoritativeStepId + (StepId) *outDiff;

    return true;
}
