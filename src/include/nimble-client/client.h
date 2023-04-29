/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_H
#define NIMBLE_CLIENT_H

#include <blob-stream/blob_stream_logic_in.h>
#include <blob-stream/blob_stream_logic_out.h>
#include <clog/clog.h>
#include <nimble-client/game_state.h>
#include <nimble-client/incoming_api.h>
#include <nimble-serialize/client_out.h>
#include <nimble-steps/pending_steps.h>
#include <nimble-steps/steps.h>
#include <ordered-datagram/in_logic.h>
#include <ordered-datagram/out_logic.h>
#include <stats/stats.h>
#include <stats/stats_per_second.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <udp-transport/udp_transport.h>

struct ImprintAllocatorWithFree;
struct ImprintAllocator;

struct FldOutStream;

typedef enum NimbleClientState {
    NimbleClientStateIdle,
    NimbleClientStateJoiningGame,
    NimbleClientStateJoinedGame,
    NimbleClientStateJoiningRequestingState,
    NimbleClientStateJoiningDownloadingState,
    NimbleClientStatePlaying,
} NimbleClientState;

#define NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT (8)

typedef struct NimbleClientParticipantEntry {
    uint8_t localUserDeviceIndex;
    uint8_t participantId;
} NimbleClientParticipantEntry;

struct ImprintAllocator;

typedef struct NimbleClient {
    int waitTime;

    NimbleClientState state;

    NimbleClientParticipantEntry localParticipantLookup[NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT];
    size_t localParticipantCount;

    UdpTransportInOut transport;

    NbsSteps outSteps;
    NbsPendingSteps authoritativePendingStepsFromServer;
    NbsSteps authoritativeStepsFromServer;
    StepId receivedStepIdByServerOnlyForDebug;
    StepId nextStepIdToSendToServer;
    NimbleClientGameState joinedGameState;
    NimbleSerializeBlobStreamChannelId joinStateChannel;

    BlobStreamLogicIn blobStreamInLogic;
    BlobStreamIn blobStreamIn;

    StepId joinStateId;

    size_t frame;
    bool useStats;
    size_t statsCounter;

    StatsInt waitingStepsFromServer;
    StatsInt stepCountInIncomingBufferOnServerStat;
    StatsInt authoritativeBufferDeltaStat;
    StatsInt outgoingStepsInQueue;

    StatsIntPerSecond packetsPerSecondOut;
    StatsIntPerSecond packetsPerSecondIn;
    StatsIntPerSecond simulationStepsPerSecond;

    struct ImprintAllocator* memory;
    struct ImprintAllocatorWithFree* blobStreamAllocator;

    uint8_t participantsConnectionIndex;

    NimbleSerializeGameJoinOptions joinGameOptions;

    OrderedDatagramOutLogic orderedDatagramOut;
    OrderedDatagramInLogic orderedDatagramIn;

    size_t maximumSingleParticipantStepOctetCount;
    size_t maximumNumberOfParticipants;
    Clog log;

} NimbleClient;

int nimbleClientInit(NimbleClient* self, struct ImprintAllocator* memory,
                     struct ImprintAllocatorWithFree* blobAllocator, UdpTransportInOut* transport,
                     size_t maximumSingleParticipantStepOctetCount, size_t maximumNumberOfParticipants, Clog log);
void nimbleClientReset(NimbleClient* self);
void nimbleClientReInit(NimbleClient* self, UdpTransportInOut* transport);
void nimbleClientDestroy(NimbleClient* self);
void nimbleClientDisconnect(NimbleClient* self);
int nimbleClientUpdate(NimbleClient* self, MonotonicTimeMs now);
int nimbleClientFindParticipantId(const NimbleClient* self, uint8_t localUserDeviceIndex, uint8_t* participantId);
int nimbleClientReJoin(NimbleClient* self);

#endif
