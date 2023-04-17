/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include "nimble-client/receive_transport.h"
#include <monotonic-time/monotonic_time.h>
#include <nimble-client/client.h>
#include <nimble-client/outgoing.h>

void nimbleClientReset(NimbleClient* self)
{
    nbsStepsReset(&self->outSteps);
    nbsPendingStepsReset(&self->authoritativePendingStepsFromServer, 0);
    nbsStepsInit(&self->authoritativeStepsFromServer, self->memory, 128);

    self->receivedStepIdByServerOnlyForDebug = NIMBLE_STEP_MAX;

    self->localParticipantCount = 0;
    for (size_t i = 0; i < NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT; ++i) {
        self->localParticipantLookup[i].participantId = 0;
        self->localParticipantLookup[i].localUserDeviceIndex = 0;
    }

    statsIntInit(&self->waitingStepsFromServer, 30);
    statsIntInit(&self->outgoingStepsInQueue, 60);
    statsIntInit(&self->stepCountInIncomingBufferOnServerStat, 30);

    MonotonicTimeMs now = monotonicTimeMsNow();

    statsIntInit(&self->authoritativeBufferDeltaStat, 30);
    statsIntPerSecondInit(&self->packetsPerSecondOut, now, 1000);
    statsIntPerSecondInit(&self->packetsPerSecondIn, now, 1000);
    statsIntPerSecondInit(&self->simulationStepsPerSecond, now, 1000);

    self->useStats = 1;
    self->transport.self = 0;
    self->transport.send = 0;
    self->transport.receive = 0;
    self->nextStepIdToSendToServer = NIMBLE_STEP_MAX;
    self->state = NimbleClientStateIdle;

    if (self->joinedGameState.gameState != 0) {
        nimbleClientGameStateDestroy(&self->joinedGameState);
    }
    self->joinedGameState.gameState = 0;

    orderedDatagramInLogicInit(&self->orderedDatagramIn);
    orderedDatagramOutLogicInit(&self->orderedDatagramOut);
}

void nimbleClientReInit(NimbleClient* self, UdpTransportInOut* transport)
{
    nbsStepsReInit(&self->outSteps, 0);
    nbsPendingStepsReset(&self->authoritativePendingStepsFromServer, 0);
    nbsStepsInit(&self->authoritativeStepsFromServer, self->memory, 128);

    self->receivedStepIdByServerOnlyForDebug = NIMBLE_STEP_MAX;

    self->localParticipantCount = 0;
    for (size_t i = 0; i < NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT; ++i) {
        self->localParticipantLookup[i].participantId = 0;
        self->localParticipantLookup[i].localUserDeviceIndex = 0;
    }
    statsIntInit(&self->authoritativeBufferDeltaStat, 10);
    statsIntInit(&self->waitingStepsFromServer, 60);
    statsIntInit(&self->outgoingStepsInQueue, 60);
    statsIntInit(&self->stepCountInIncomingBufferOnServerStat, 10);

    MonotonicTimeMs now = monotonicTimeMsNow();

    statsIntPerSecondInit(&self->packetsPerSecondOut, now, 1000);
    statsIntPerSecondInit(&self->packetsPerSecondIn, now, 1000);
    statsIntPerSecondInit(&self->simulationStepsPerSecond, now, 1000);
    self->useStats = 1;
    self->transport = *transport;
    self->nextStepIdToSendToServer = NIMBLE_STEP_MAX;
    self->state = NimbleClientStateIdle;
    if (self->joinedGameState.gameState != 0) {
        nimbleClientGameStateDestroy(&self->joinedGameState);
    }

    self->joinedGameState.gameState = 0;
}

int nimbleClientInit(NimbleClient* self, struct ImprintAllocator* memory,
                     struct ImprintAllocatorWithFree* blobAllocator, UdpTransportInOut* transport)
{
    clogInitFromGlobal(&self->clog, "NimbleClient");
    self->memory = memory;
    self->state = NimbleClientStateIdle;
    self->transport = *transport;
    self->blobStreamAllocator = blobAllocator;

    nbsStepsInit(&self->outSteps, memory, 3 * 1024);
    nbsPendingStepsInit(&self->authoritativePendingStepsFromServer, 0, blobAllocator);
    nbsStepsInit(&self->authoritativeStepsFromServer, self->memory, 128);
    self->joinedGameState.gameState = 0;

    nimbleClientReset(self);



    return 0;
}

void nimbleClientDestroy(NimbleClient* self)
{
    nbsStepsDestroy(&self->outSteps);
    if (self->joinedGameState.gameState != 0) {
        nimbleClientGameStateDestroy(&self->joinedGameState);
    }
}

void nimbleClientDisconnect(NimbleClient* self)
{
}

static void showStats(NimbleClient* self)
{
    self->statsCounter++;
    int shouldPrint = (self->statsCounter % 601) == 0; // self->waitingStepsFromServer.count == 0;
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
}

static void calcStats(NimbleClient* self, MonotonicTimeMs now)
{
    statsIntPerSecondUpdate(&self->packetsPerSecondOut, now);
    statsIntPerSecondUpdate(&self->packetsPerSecondIn, now);
    statsIntPerSecondUpdate(&self->simulationStepsPerSecond, now);
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

int nimbleClientUpdate(NimbleClient* self, MonotonicTimeMs now)
{
    int errorCode;

    errorCode = nimbleClientReceiveAllInUdpBuffer(self);
    if (errorCode < 0) {
        return errorCode;
    }

    self->waitTime--;
    if (self->waitTime > 0) {
        return 0;
    }

    // CLOG_INFO("nimble client update ===========");

    calcStats(self, now);
    showStats(self);
    sendPackets(self);

    // CLOG_INFO("------ nimble client");

    return errorCode;
}
