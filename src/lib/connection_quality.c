/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <nimble-client/client.h>
#include <nimble-client/connection_quality.h>

/// Initialize connection quality
/// @param self connection quality
/// @param log logging
void nimbleClientConnectionQualityInit(NimbleClientConnectionQuality* self, Clog log)
{
    self->log = log;
    nimbleClientConnectionQualityReset(self);
}

/// Reset all values
/// @param self connection quality
void nimbleClientConnectionQualityReset(NimbleClientConnectionQuality* self)
{
    statsHoldPositiveInit(&self->droppingDatagramWarning, 20U);
    statsHoldPositiveInit(&self->impendingDisconnectWarning, 20U);
    statsIntInit(&self->latencyMsStat, 3);
    statsIntInit(&self->ratingStat, 10);
    self->lastLatencyMs = 0;
    self->ticksWithoutAuthoritativeStepsFromInSerialize = 0;
    self->ticksWithoutIncomingDatagrams = 0;
    self->consideringDisconnectCounter = 0;
    self->droppedDatagramCounter = 0;
    self->reason = NimbleConnectionQualityDisconnectReasonKeepConnection;
}

static float degradeLowIsBetter(size_t v, size_t divisor, size_t lowerThreshold)
{
    CLOG_ASSERT(divisor != 0, "divisor can not be zero")

    if (v < lowerThreshold) {
        return 1.f;
    }

    if (v > divisor) {
        return 0.f;
    }

    return 1.f - ((float) (v - lowerThreshold) / (float) (divisor - lowerThreshold));
}

/// Evaluate the connection quality
/// @param self connection quality
/// @return the disconnect recommendation
static NimbleConnectionQualityDisconnectReason evaluate(NimbleClientConnectionQuality* self)
{
    const uint maxTicksWithoutDatagrams = 40;
    const uint maxTicksWithoutSteps = 15;

    float ticksWithoutDatagramsDegrade = degradeLowIsBetter(self->ticksWithoutIncomingDatagrams,
                                                            maxTicksWithoutDatagrams, 10);
    float ticksWithoutStepsDegrade = degradeLowIsBetter(self->ticksWithoutAuthoritativeStepsFromInSerialize,
                                                        maxTicksWithoutSteps, 10);

    float droppingPacketsDegrade = degradeLowIsBetter(self->droppedDatagramCounter, 10, 1);

    float highJitterDegrade = 1.f;
    float latencyDegrade = 1.f;

    if (self->latencyMsStat.avgIsSet) {
        size_t latencyAvg = (size_t) self->latencyMsStat.avg;
        int latencyDiff = (int) latencyAvg - (int) self->lastLatencyMs;
        self->lastLatencyMs = latencyAvg;
        if (abs(latencyDiff) > 3) {
            highJitterDegrade = 0.9f;
        }

        latencyDegrade = degradeLowIsBetter((size_t) latencyAvg, 250, 60);
    }

    float totalDegrade = ticksWithoutDatagramsDegrade * ticksWithoutStepsDegrade * latencyDegrade * highJitterDegrade *
                         droppingPacketsDegrade;

    self->qualityRating = (uint8_t) (totalDegrade * 5.f);

    statsIntAdd(&self->ratingStat, self->qualityRating);

    if (self->ticksWithoutIncomingDatagrams >= maxTicksWithoutDatagrams) {
        return NimbleConnectionQualityDisconnectReasonNotReceivingDatagramsFromServer;
    }

    if (self->ticksWithoutAuthoritativeStepsFromInSerialize > maxTicksWithoutSteps) {
        return NimbleConnectionQualityDisconnectReasonNotReceivingStepsFromServer;
    }

    return NimbleConnectionQualityDisconnectReasonKeepConnection;
}

/// Describes the current disconnect recommendation
/// @param self connection quality
/// @param buf string buffer to fill
/// @param maxBufSize maximum number of characters in buffer
/// @return the filled in buf
const char* nimbleClientConnectionQualityDescribe(NimbleClientConnectionQuality* self, char* buf, size_t maxBufSize)
{
    NimbleConnectionQualityDisconnectReason reason = evaluate(self);
    switch (reason) {
        case NimbleConnectionQualityDisconnectReasonKeepConnection:
            tc_snprintf(buf, maxBufSize, "it is all good! rating: %d", self->qualityRating);
            break;
        case NimbleConnectionQualityDisconnectReasonNotReceivingStepsFromServer:
            tc_snprintf(buf, maxBufSize,
                        "not receiving datagrams from server. %zu ticks since last valid step from server. rating: %d",
                        self->ticksWithoutAuthoritativeStepsFromInSerialize, self->qualityRating);
            break;
        case NimbleConnectionQualityDisconnectReasonNotReceivingDatagramsFromServer:
            tc_snprintf(buf, maxBufSize,
                        "not receiving datagrams from server. %zu ticks since last valid datagram. rating: %d",
                        self->ticksWithoutIncomingDatagrams, self->qualityRating);
            break;
    }

    return buf;
}

/// Update the connection quality
/// @param self connection quality
/// @param client the nimble client to evaluate
/// @param now current monotonic time
void nimbleClientConnectionQualityUpdate(NimbleClientConnectionQuality* self, NimbleClient* client, MonotonicTimeMs now)
{
    if (client->state != NimbleClientStateSynced) {
        return;
    }
    (void) now;

    self->ticksWithoutAuthoritativeStepsFromInSerialize++;
    self->ticksWithoutIncomingDatagrams++;

    if (self->droppedDatagramCounter > 0) {
        self->droppedDatagramCounter--;
    }

    NimbleConnectionQualityDisconnectReason reason = evaluate(self);
    if (reason == NimbleConnectionQualityDisconnectReasonKeepConnection) {
        if (self->consideringDisconnectCounter > 0) {
            self->consideringDisconnectCounter--;
            if (self->consideringDisconnectCounter == 0) {
                CLOG_C_NOTICE(&self->log, "quality problems are forgiven.")
            }
        }
    } else {
#if defined CLOG_LOG_ENABLED
#define BUF_SIZE (128)
        char buf[BUF_SIZE];
#endif
        self->consideringDisconnectCounter++;

        if (self->consideringDisconnectCounter == 1) {
            CLOG_C_NOTICE(&self->log, "noticed quality degration. %s",
                          nimbleClientConnectionQualityDescribe(self, buf, BUF_SIZE))
        }

        if ((self->consideringDisconnectCounter % 20) == 0) {
            CLOG_C_NOTICE(&self->log, "still thinking about disconnecting: %s",
                          nimbleClientConnectionQualityDescribe(self, buf, BUF_SIZE))
        }

        if (self->consideringDisconnectCounter > 60) {
            self->reason = reason;
            CLOG_C_NOTICE(&self->log, "I gave up due to: %s",
                          nimbleClientConnectionQualityDescribe(self, buf, BUF_SIZE))
        }
    }
}

/// Inform connection quality about dropped datagrams
/// @param self connection quality
/// @param delta number of dropped datagrams
void nimbleClientConnectionQualityDroppedDatagrams(NimbleClientConnectionQuality* self, size_t delta)
{
    bool droppedDatagramWarning = delta > 1;
    self->droppedDatagramCounter += delta;
    if (delta > 0) {
        CLOG_C_NOTICE(&self->log, "dropped datagram! %zu", delta)
    }
    statsHoldPositiveAdd(&self->droppingDatagramWarning, droppedDatagramWarning);
}

/// Inform connection quality about having received a usable datagram
/// @param self connection quality
void nimbleClientConnectionQualityReceivedUsableDatagram(NimbleClientConnectionQuality* self)
{
    self->ticksWithoutIncomingDatagrams = 0;
    statsHoldPositiveAdd(&self->droppingDatagramWarning, false);
}

/// Inform connection quality about a received authoritative steps
/// @param self  connection quality
/// @param delta number of steps added to the combined authoritative step
void nimbleClientConnectionQualityReceivedAuthoritativeSteps(NimbleClientConnectionQuality* self, size_t delta)
{
    (void) delta;
    self->ticksWithoutAuthoritativeStepsFromInSerialize = 0;
}

/// Inform connection quality about the current measured step latency
/// @param self connection quality
/// @param latencyInMs latency in milliseconds
void nimbleClientConnectionQualityGameStepLatency(NimbleClientConnectionQuality* self, size_t latencyInMs)
{
    statsIntAdd(&self->latencyMsStat, (int) latencyInMs);
}

/// Checks if the recommendation is to disconnect the connection
/// @param self connection quality
/// @return true if the connection should be disconnected
bool nimbleClientConnectionQualityShouldDisconnect(const NimbleClientConnectionQuality* self)
{
    return self->reason != NimbleConnectionQualityDisconnectReasonKeepConnection;
}
