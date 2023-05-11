/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <monotonic-time/monotonic_time.h>
#include <nimble-client/client.h>
#include <nimble-client/outgoing.h>
#include <nimble-client/receive_transport.h>
#include <nimble-steps-serialize/out_serialize.h>

/// Resets the nimble client so it can be reused for the same transport
/// @param self
void nimbleClientReset(NimbleClient* self)
{
    nbsStepsReset(&self->outSteps);
    nbsPendingStepsReset(&self->authoritativePendingStepsFromServer, 0);

    size_t combinedStepOctetCount = nbsStepsOutSerializeCalculateCombinedSize(
        self->maximumNumberOfParticipants, self->maximumSingleParticipantStepOctetCount);
    nbsStepsInit(&self->authoritativeStepsFromServer, self->memory, combinedStepOctetCount, self->log);

    self->receivedStepIdByServerOnlyForDebug = NIMBLE_STEP_MAX;

    self->localParticipantCount = 0;
    for (size_t i = 0; i < NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT; ++i) {
        self->localParticipantLookup[i].participantId = 0;
        self->localParticipantLookup[i].localUserDeviceIndex = 0;
    }

    statsIntInit(&self->waitingStepsFromServer, 30);
    statsIntInit(&self->outgoingStepsInQueue, 60);
    statsIntInit(&self->stepCountInIncomingBufferOnServerStat, 30);
    statsIntInit(&self->tickDuration, 20);
    statsIntInit(&self->latencyMsStat, 20);
    self->lastUpdateMonotonicMsIsSet = false;

    MonotonicTimeMs now = monotonicTimeMsNow();

    statsIntInit(&self->authoritativeBufferDeltaStat, 30);
    statsIntPerSecondInit(&self->packetsPerSecondOut, now, 1000);
    statsIntPerSecondInit(&self->packetsPerSecondIn, now, 1000);
    statsIntPerSecondInit(&self->simulationStepsPerSecond, now, 1000);
    statsIntPerSecondInit(&self->sentStepsDatagramCountPerSecond, now, 1000);

    self->useStats = true;
    self->state = NimbleClientStateIdle;

    if (self->joinedGameState.gameState != 0) {
        nimbleClientGameStateDestroy(&self->joinedGameState);
    }
    self->joinedGameState.gameState = 0;
    self->downloadStateClientRequestId = 33;
    orderedDatagramInLogicInit(&self->orderedDatagramIn);
    orderedDatagramOutLogicInit(&self->orderedDatagramOut);
    lagometerInit(&self->lagometer);
}


/// Re-initializes the nimble client to be using another transport (connection)
/// @param self
/// @param transport
void nimbleClientReInit(NimbleClient* self, UdpTransportInOut* transport)
{
    self->transport = *transport;
    nimbleClientReset(self);
}


/// Initializes a nimble client
/// @param self
/// @param memory
/// @param blobAllocator
/// @param transport
/// @param maximumSingleParticipantStepOctetCount
/// @param maximumNumberOfParticipants
/// @param applicationVersion
/// @param log
/// @return
int nimbleClientInit(NimbleClient* self, struct ImprintAllocator* memory,
                     struct ImprintAllocatorWithFree* blobAllocator, UdpTransportInOut* transport,
                     size_t maximumSingleParticipantStepOctetCount, size_t maximumNumberOfParticipants,
                     NimbleSerializeVersion applicationVersion, Clog log)
{
    self->log = log;
    self->applicationVersion = applicationVersion;

    const size_t maximumSingleStepCountAllowed = 24;
    if (maximumSingleParticipantStepOctetCount > maximumSingleStepCountAllowed) {
        CLOG_C_ERROR(&self->log, "nimbleClientInit. Single step octet count is not allowed %zu of %zu",
                     maximumSingleParticipantStepOctetCount, maximumSingleStepCountAllowed)
        return -1;
    }

    const size_t maximumNumberOfParticipantsAllowed = 64;
    if (maximumNumberOfParticipants > maximumNumberOfParticipantsAllowed) {
        CLOG_C_ERROR(&self->log, "nimbleClientInit. maximum number of participant count is too high: %zu of %zu",
                     maximumNumberOfParticipants, maximumNumberOfParticipantsAllowed)
        return -1;
    }

    self->memory = memory;
    self->expectedTickDurationMs = 16;
    self->blobStreamAllocator = blobAllocator;
    self->joinedGameState.gameState = 0;
    self->maximumSingleParticipantStepOctetCount = maximumSingleParticipantStepOctetCount;
    self->maximumNumberOfParticipants = maximumNumberOfParticipants;

    self->state = NimbleClientStateIdle;
    self->transport = *transport;

    size_t combinedStepOctetCount = nbsStepsOutSerializeCalculateCombinedSize(maximumNumberOfParticipants,
                                                                              maximumSingleParticipantStepOctetCount);

    nbsStepsInit(&self->outSteps, memory, combinedStepOctetCount, log);
    nbsPendingStepsInit(&self->authoritativePendingStepsFromServer, 0, blobAllocator, log);
    nbsStepsInit(&self->authoritativeStepsFromServer, self->memory, combinedStepOctetCount, log);

    nimbleClientReInit(self, transport);

    return 0;
}

/// Destroys a nimble client and frees the allocated memory
/// @param self
void nimbleClientDestroy(NimbleClient* self)
{
    if (self->joinedGameState.gameState != 0) {
        nimbleClientGameStateDestroy(&self->joinedGameState);
    }
}

/// Disconnects the client
/// @note not implemented yet
/// @param self
void nimbleClientDisconnect(NimbleClient* self)
{
}

static void showStats(NimbleClient* self)
{
    self->statsCounter++;
    int shouldPrint = (self->statsCounter % 3000) == 0;
    if (!shouldPrint) {
        return;
    }

    statsIntDebug(&self->waitingStepsFromServer, "waiting steps from server", "steps");
    statsIntDebug(&self->stepCountInIncomingBufferOnServerStat, "incoming buffer count on server", "steps");
    statsIntDebug(&self->authoritativeBufferDeltaStat, "delta from last authoritative step on server", "steps");
    statsIntDebug(&self->outgoingStepsInQueue, "outgoing steps to send to server", "steps");

    statsIntPerSecondDebugOutput(&self->packetsPerSecondOut, "PPS Out", "packets/s");
    statsIntPerSecondDebugOutput(&self->packetsPerSecondIn, "PPS In", "packets/s");
    statsIntPerSecondDebugOutput(&self->simulationStepsPerSecond, "SIM In", "steps/s");
    statsIntPerSecondDebugOutput(&self->sentStepsDatagramCountPerSecond, "SSD out", "steps/s");
}

static void calcStats(NimbleClient* self, MonotonicTimeMs now)
{
    statsIntPerSecondUpdate(&self->packetsPerSecondOut, now);
    statsIntPerSecondUpdate(&self->packetsPerSecondIn, now);
    statsIntPerSecondUpdate(&self->simulationStepsPerSecond, now);
    statsIntPerSecondUpdate(&self->sentStepsDatagramCountPerSecond, now);
}

static int sendPackets(NimbleClient* self)
{
    UdpTransportOut transportOut;
    transportOut.self = self->transport.self;
    transportOut.send = self->transport.send;

    int errorCode = nimbleClientOutgoing(self, &transportOut);
    if (errorCode < 0) {
        return errorCode;
    }

    return 0;
}

/// Looks up the participant id for the specified local user device index
/// @param self
/// @param localUserDeviceIndex
/// @param participantId
/// @return 1 on success or negative on error
int nimbleClientFindParticipantId(const NimbleClient* self, uint8_t localUserDeviceIndex, uint8_t* participantId)
{
    for (size_t i = 0; i < self->localParticipantCount; ++i) {
        if (self->localParticipantLookup[i].localUserDeviceIndex == localUserDeviceIndex) {
            *participantId = self->localParticipantLookup[i].participantId;
            return 1;
        }
    }

    *participantId = 0xff;

    return -1;
}

/// Updates the nimble client
/// @param self
/// @param now
/// @return
int nimbleClientUpdate(NimbleClient* self, MonotonicTimeMs now)
{
    int errorCode;

    if (self->lastUpdateMonotonicMsIsSet) {
        int encounteredTickDuration = now - self->lastUpdateMonotonicMs;
        if (encounteredTickDuration + 10 < self->expectedTickDurationMs) {
            CLOG_C_INFO(&self->log, "updating too often, time in ms since last update: %d", encounteredTickDuration)
            return 0;
        }
        statsIntAdd(&self->tickDuration, encounteredTickDuration);
        if (self->tickDuration.avgIsSet) {
            if (abs((int)self->expectedTickDurationMs - self->tickDuration.avg ) > 10) {
                CLOG_C_INFO(&self->log, "not holding tick rate: expected: %zu vs %d", self->expectedTickDurationMs, self->tickDuration.avg)
            }
        }
    } else {
        self->lastUpdateMonotonicMsIsSet = true;
    }
    self->lastUpdateMonotonicMs = now;

    errorCode = nimbleClientReceiveAllInUdpBuffer(self);
    if (errorCode < 0) {
        return errorCode;
    }

    if (self->waitTime > 0) {
        self->waitTime--;
        return 0;
    }

    // CLOG_INFO("nimble client update ===========");

    calcStats(self, now);
    showStats(self);
    sendPackets(self);

    // CLOG_INFO("------ nimble client");

    return errorCode;
}
