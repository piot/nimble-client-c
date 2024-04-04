/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_CONNECTION_QUALITY_H
#define NIMBLE_CLIENT_CONNECTION_QUALITY_H

#include <stats/hold_positive.h>
#include <clog/clog.h>
#include <monotonic-time/monotonic_time.h>

struct NimbleClient;

typedef enum NimbleConnectionQualityDisconnectReason {
    NimbleConnectionQualityDisconnectReasonKeepConnection,
    NimbleConnectionQualityDisconnectReasonNotReceivingStepsFromServer,
    NimbleConnectionQualityDisconnectReasonNotReceivingDatagramsFromServer,
} NimbleConnectionQualityDisconnectReason;

typedef struct NimbleClientConnectionQuality {
    StatsHoldPositive droppingDatagramWarning;
    StatsHoldPositive impendingDisconnectWarning;
    StatsInt latencyMsStat;
    StatsInt ratingStat;
    size_t lastLatencyMs;
    size_t ticksWithoutIncomingDatagrams;
    size_t ticksWithoutAuthoritativeStepsFromInSerialize;
    NimbleConnectionQualityDisconnectReason reason;
    size_t consideringDisconnectCounter;
    uint8_t qualityRating;
    size_t droppedDatagramCounter;
    Clog log;
} NimbleClientConnectionQuality;

void nimbleClientConnectionQualityInit(NimbleClientConnectionQuality* self, Clog log);
void nimbleClientConnectionQualityReset(NimbleClientConnectionQuality* self);

void nimbleClientConnectionQualityUpdate(NimbleClientConnectionQuality* self, struct NimbleClient* client,
                                        MonotonicTimeMs now);


const char* nimbleClientConnectionQualityDescribe(NimbleClientConnectionQuality* self, char* buf, size_t maxBufSize);

void nimbleClientConnectionQualityReceivedAuthoritativeSteps(NimbleClientConnectionQuality* self, size_t count);
void nimbleClientConnectionQualityReceivedUsableDatagram(NimbleClientConnectionQuality* self);
void nimbleClientConnectionQualityDroppedDatagrams(NimbleClientConnectionQuality* self, size_t delta);
void nimbleClientConnectionQualityGameStepLatency(NimbleClientConnectionQuality* self, size_t latencyInMs);


bool nimbleClientConnectionQualityShouldDisconnect(const NimbleClientConnectionQuality* self);

#endif
