/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <monotonic-time/monotonic_time.h>
#include <nimble-client/client.h>
#include <nimble-client/outgoing.h>
#include <nimble-client/receive_transport.h>
#include <nimble-steps-serialize/out_serialize.h>
#include <inttypes.h>

/// Resets the nimble client so it can be reused for the same transport
/// @param self nimble client
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

    statsIntInit(&self->waitingStepsFromServer, 20);
    statsIntInit(&self->outgoingStepsInQueue, 20);
    statsIntInit(&self->stepCountInIncomingBufferOnServerStat, 20);
    statsIntInit(&self->tickDuration, 20);
    statsIntInit(&self->latencyMsStat, 20);
    self->lastUpdateMonotonicMsIsSet = false;

    MonotonicTimeMs now = monotonicTimeMsNow();

    statsIntInit(&self->authoritativeBufferDeltaStat, 10);
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
    nimbleClientConnectionQualityReset(&self->quality);
    orderedDatagramInLogicInit(&self->orderedDatagramIn);
    orderedDatagramOutLogicInit(&self->orderedDatagramOut);
    lagometerInit(&self->lagometer);
}

/// Re-initializes the nimble client to be using another transport (connection)
/// @param self nimble client
/// @param transport transport to use
void nimbleClientReInit(NimbleClient* self, DatagramTransport* transport)
{
    self->transport = *transport;
    nimbleClientReset(self);
}

/// Initializes a nimble client
/// @param self nimble client
/// @param memory tagAllocator
/// @param blobAllocator freeAllocator
/// @param transport the datagram transport to use
/// @param maximumSingleParticipantStepOctetCount application specific step octet size
/// @param maximumNumberOfParticipants maximum number of participants in a game
/// @param applicationVersion application specific version
/// @param log logging target
/// @return negative on error
int nimbleClientInit(NimbleClient* self, struct ImprintAllocator* memory,
                     struct ImprintAllocatorWithFree* blobAllocator, DatagramTransport* transport,
                     size_t maximumSingleParticipantStepOctetCount, size_t maximumNumberOfParticipants,
                     NimbleSerializeVersion applicationVersion, bool wantsDebugStreams, Clog log)
{
    self->log = log;
    self->useDebugStreams = false;
    self->wantsDebugStreams = wantsDebugStreams;
    self->applicationVersion = applicationVersion;
    self->remoteConnectionId = 0;

    if (maximumSingleParticipantStepOctetCount > NimbleStepMaxSingleStepOctetCount) {
        CLOG_C_ERROR(&self->log, "nimbleClientInit. Single step octet count is not allowed %zu of %zu",
                     maximumSingleParticipantStepOctetCount, NimbleStepMaxSingleStepOctetCount)
        // return -1;
    }

    const size_t maximumNumberOfParticipantsAllowed = 64;
    if (maximumNumberOfParticipants > maximumNumberOfParticipantsAllowed) {
        CLOG_C_ERROR(&self->log, "nimbleClientInit. maximum number of participant count is too high: %zu of %zu",
                     maximumNumberOfParticipants, maximumNumberOfParticipantsAllowed)
        // return -1;
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
    nimbleClientConnectionQualityInit(&self->quality, log);

    return 0;
}

/// Destroys a nimble client and frees the allocated memory
/// @param self nimble client
void nimbleClientDestroy(NimbleClient* self)
{
    if (self->joinedGameState.gameState != 0) {
        nimbleClientGameStateDestroy(&self->joinedGameState);
    }
}

/// Disconnects the client
/// @note not implemented yet
/// @param self nimble client
void nimbleClientDisconnect(NimbleClient* self)
{
    self->state = NimbleClientStateDisconnected;
}

static void showStats(NimbleClient* self)
{
    self->statsCounter++;
    int shouldPrint = (self->statsCounter % 3000) == 0;
    if (!shouldPrint) {
        return;
    }

    const Clog* log = &self->log;

    statsIntDebug(&self->waitingStepsFromServer, log, "waiting steps from server", "steps");
    statsIntDebug(&self->stepCountInIncomingBufferOnServerStat, log, "incoming buffer count on server", "steps");
    statsIntDebug(&self->authoritativeBufferDeltaStat, log, "delta from last authoritative step on server", "steps");
    statsIntDebug(&self->outgoingStepsInQueue, log, "outgoing steps to send to server", "steps");

    statsIntPerSecondDebugOutput(&self->packetsPerSecondOut, log, "PPS Out", "packets/s");
    statsIntPerSecondDebugOutput(&self->packetsPerSecondIn, log, "PPS In", "packets/s");
    statsIntPerSecondDebugOutput(&self->simulationStepsPerSecond, log, "SIM In", "steps/s");
    statsIntPerSecondDebugOutput(&self->sentStepsDatagramCountPerSecond, log, "SSD out", "steps/s");
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
    if (self->state == NimbleClientStateIdle || self->state == NimbleClientStateDisconnected) {
        return 0;
    }

    DatagramTransportOut transportOut;
    transportOut.self = self->transport.self;
    transportOut.send = self->transport.send;

    int errorCode = nimbleClientOutgoing(self, &transportOut);
    if (errorCode < 0) {
        return errorCode;
    }

    return 0;
}

/// Looks up the participant id for the specified local user device index
/// @param self nimble client
/// @param localUserDeviceIndex application specific user device index
/// @param participantId the found participant ID
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



static void checkTickInterval(NimbleClient* self, MonotonicTimeMs now)
{
    if (self->lastUpdateMonotonicMsIsSet) {
        MonotonicTimeMs encounteredTickDuration = now - self->lastUpdateMonotonicMs;
        if (encounteredTickDuration + 10 < (int) self->expectedTickDurationMs) {
            //CLOG_C_VERBOSE(&self->log, "updating too often, time in ms since last update: %" PRIi64, encounteredTickDuration)
            return;
        }
        statsIntAdd(&self->tickDuration, (int) encounteredTickDuration);
        if (self->tickDuration.avgIsSet) {
            if (abs((int) self->expectedTickDurationMs - self->tickDuration.avg) > 10) {
                CLOG_C_VERBOSE(&self->log, "not holding tick rate: expected: %zu vs %d", self->expectedTickDurationMs,
                               self->tickDuration.avg)
            }
        }
    } else {
        self->lastUpdateMonotonicMsIsSet = true;
    }
    self->lastUpdateMonotonicMs = now;
}


static void checkIfDisconnectIsNeeded(NimbleClient* self)
{
    if (self->state != NimbleClientStateSynced) {
        return;
    }

    if (nimbleClientConnectionQualityShouldDisconnect(&self->quality))
    {
        self->state = NimbleClientStateDisconnected;
    }
}

/// Updates the nimble client
/// @param self nimble client
/// @param now current time with milliseconds resolution
/// @return negative on error
int nimbleClientUpdate(NimbleClient* self, MonotonicTimeMs now)
{
    self->loggingTickCount++;
    checkTickInterval(self, now);

    ssize_t errorCode = nimbleClientReceiveAllDatagramsFromTransport(self);
    if (errorCode < 0) {
        return (int) errorCode;
    }

    nimbleClientConnectionQualityUpdate(&self->quality, self, now);

    checkIfDisconnectIsNeeded(self);

    if (self->waitTime > 0) {
        self->waitTime--;
        return 0;
    }

    calcStats(self, now);
    showStats(self);
    sendPackets(self);

    return (int) errorCode;
}
