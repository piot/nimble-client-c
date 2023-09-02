/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_H
#define NIMBLE_CLIENT_H

#include <blob-stream/blob_stream_logic_in.h>
#include <blob-stream/blob_stream_logic_out.h>
#include <clog/clog.h>
#include <datagram-transport/transport.h>
#include <lagometer/lagometer.h>
#include <nimble-client/game_state.h>
#include <nimble-client/incoming_api.h>
#include <nimble-serialize/client_out.h>
#include <nimble-steps/pending_steps.h>
#include <nimble-steps/steps.h>
#include <ordered-datagram/in_logic.h>
#include <ordered-datagram/out_logic.h>
#include <stats/stats.h>
#include <stats/stats_per_second.h>
#include <stats/hold_positive.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct ImprintAllocatorWithFree;
struct ImprintAllocator;

struct FldOutStream;

typedef enum NimbleClientState {
    NimbleClientStateIdle,
    NimbleClientStateJoiningRequestingState,
    NimbleClientStateJoiningDownloadingState,
    NimbleClientStateSynced,
    NimbleClientStateDisconnected
} NimbleClientState;

typedef enum NimbleJoiningState {
    NimbleJoiningStateJoiningParticipant,
    NimbleJoiningStateJoinedParticipant,
    NimbleJoiningStateOutOfParticipantSlots,
} NimbleJoiningState;

#define NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT (8)

typedef struct NimbleClientParticipantEntry {
    uint8_t localUserDeviceIndex;
    uint8_t participantId;
} NimbleClientParticipantEntry;

struct ImprintAllocator;

typedef struct NimbleClient {
    int waitTime;

    NimbleClientState state;
    NimbleJoiningState joinParticipantPhase;

    NimbleClientParticipantEntry localParticipantLookup[NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT];
    size_t localParticipantCount;

    DatagramTransport transport;

    NbsSteps outSteps;
    NbsPendingSteps authoritativePendingStepsFromServer;
    NbsSteps authoritativeStepsFromServer;
    StepId receivedStepIdByServerOnlyForDebug;
    NimbleClientGameState joinedGameState;
    NimbleSerializeBlobStreamChannelId joinStateChannel;

    BlobStreamLogicIn blobStreamInLogic;
    BlobStreamIn blobStreamIn;
    uint8_t downloadStateClientRequestId;

    StepId joinStateId;

    size_t frame;
    bool useStats;
    size_t statsCounter;

    StatsInt waitingStepsFromServer;
    StatsInt stepCountInIncomingBufferOnServerStat;
    StatsInt authoritativeBufferDeltaStat;
    StatsInt latencyMsStat;
    StatsInt outgoingStepsInQueue;

    StatsIntPerSecond packetsPerSecondOut;
    StatsIntPerSecond packetsPerSecondIn;
    StatsIntPerSecond simulationStepsPerSecond;
    StatsIntPerSecond sentStepsDatagramCountPerSecond;

    struct ImprintAllocator* memory;
    struct ImprintAllocatorWithFree* blobStreamAllocator;

    uint8_t participantsConnectionIndex;
    NimbleSerializeParticipantConnectionSecret participantsConnectionSecret;

    NimbleSerializeGameJoinOptions joinGameOptions;

    OrderedDatagramOutLogic orderedDatagramOut;
    OrderedDatagramInLogic orderedDatagramIn;

    size_t maximumSingleParticipantStepOctetCount;
    size_t maximumNumberOfParticipants;
    NimbleSerializeVersion applicationVersion;
    Clog log;

    bool lastUpdateMonotonicMsIsSet;
    MonotonicTimeMs lastUpdateMonotonicMs;
    size_t expectedTickDurationMs;
    StatsInt tickDuration;

    size_t latencyMs;

    Lagometer lagometer;

    StatsHoldPositive droppingDatagramWarning;
    StatsHoldPositive impendingDisconnectWarning;
    size_t ticksWithoutIncomingDatagrams;
    size_t ticksWithoutAuthoritativeStepsFromInSerialize;

    bool useDebugStreams;
} NimbleClient;

int nimbleClientInit(NimbleClient* self, struct ImprintAllocator* memory,
                     struct ImprintAllocatorWithFree* blobAllocator, DatagramTransport* transport,
                     size_t maximumSingleParticipantStepOctetCount, size_t maximumNumberOfParticipants,
                     NimbleSerializeVersion applicationVersion, Clog log);
void nimbleClientReset(NimbleClient* self);
void nimbleClientReInit(NimbleClient* self, DatagramTransport* transport);
void nimbleClientDestroy(NimbleClient* self);
void nimbleClientDisconnect(NimbleClient* self);
int nimbleClientUpdate(NimbleClient* self, MonotonicTimeMs now);
int nimbleClientFindParticipantId(const NimbleClient* self, uint8_t localUserDeviceIndex, uint8_t* participantId);
int nimbleClientReJoin(NimbleClient* self);

#endif
