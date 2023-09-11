#include <nimble-client/client.h>
#include <nimble-client/connection_quality.h>

void nimbleClientConnectionQualityInit(NimbleClientConnectionQuality* self, Clog log)
{
    self->log = log;
    nimbleClientConnectionQualityReset(self);
}

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
/*
static float degrade(size_t v, size_t divisor)
{
    CLOG_ASSERT(divisor != 0, "divisor can not be zero")
    if (v > divisor) {
        return 1.f;
    }

    return (float) v / (float) divisor;
}
*/

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
    // CLOG_C_NOTICE(&self->log, "highJitterDegrade %f, latencyDegrade: %f, noSteps: %f, noDatagrams: %f dropping: %f
    // total: %f",
    //             highJitterDegrade, latencyDegrade, ticksWithoutStepsDegrade, ticksWithoutDatagramsDegrade,
    //             droppingPacketsDegrade,
    //           totalDegrade)

    self->qualityRating = (uint8_t) (totalDegrade * 5.f);
    // CLOG_C_NOTICE(&self->log, "qualityrating: %d", self->ratingStat.avg)

    statsIntAdd(&self->ratingStat, self->qualityRating);

    if (self->ticksWithoutIncomingDatagrams >= maxTicksWithoutDatagrams) {
        return NimbleConnectionQualityDisconnectReasonNotReceivingDatagramsFromServer;
    }

    if (self->ticksWithoutAuthoritativeStepsFromInSerialize > maxTicksWithoutSteps) {
        return NimbleConnectionQualityDisconnectReasonNotReceivingStepsFromServer;
    }

    return NimbleConnectionQualityDisconnectReasonKeepConnection;
}

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

    /*

        bool impendingDisconnectWarning = self->ticksWithoutIncomingDatagrams > 10U;
        statsHoldPositiveAdd(&self->impendingDisconnectWarning, impendingDisconnectWarning);
        if (self->ticksWithoutIncomingDatagrams >= 30U) {
            CLOG_C_NOTICE(&self->log, "no valid incoming datagrams for %zu ticks, disconnecting",
                          self->ticksWithoutIncomingDatagrams)
            self->state = NimbleClientStateDisconnected;
        }

        if (self->localParticipantCount > 0) {
            if (self->ticksWithoutAuthoritativeStepsFromInSerialize > 20U) {
                CLOG_C_NOTICE(&self->log, "no new authoritative steps for %zu ticks - disconnecting",
                              self->ticksWithoutAuthoritativeStepsFromInSerialize)
                self->state = NimbleClientStateDisconnected;
            }

            self->ticksWithoutAuthoritativeStepsFromInSerialize++;
        }

    */

    NimbleConnectionQualityDisconnectReason reason = evaluate(self);
    if (reason == NimbleConnectionQualityDisconnectReasonKeepConnection) {
        if (self->consideringDisconnectCounter > 0) {
            self->consideringDisconnectCounter--;
            if (self->consideringDisconnectCounter == 0) {
                CLOG_C_NOTICE(&self->log, "quality problems are forgiven.")
            }
        }
    } else {
        #define BUF_SIZE (128)
        char buf[BUF_SIZE];

        self->consideringDisconnectCounter++;

        if (self->consideringDisconnectCounter == 1) {
            CLOG_C_NOTICE(&self->log, "noticed quality degration. %s", nimbleClientConnectionQualityDescribe(self, buf, BUF_SIZE))
        }

        if ((self->consideringDisconnectCounter % 20) == 0) {
            CLOG_C_NOTICE(&self->log, "still thinking about disconnecting: %s", nimbleClientConnectionQualityDescribe(self, buf, BUF_SIZE))
        }

        if (self->consideringDisconnectCounter > 60) {
            self->reason = reason;
            CLOG_C_NOTICE(&self->log, "I gave up due to: %s", nimbleClientConnectionQualityDescribe(self, buf, BUF_SIZE))
        }
    }
}

void nimbleClientConnectionQualityDroppedDatagrams(NimbleClientConnectionQuality* self, size_t delta)
{
    bool droppedDatagramWarning = delta > 1;
    self->droppedDatagramCounter += delta;
    if (delta > 0) {
        CLOG_C_NOTICE(&self->log, "dropped datagram! %zu", delta)
    }
    statsHoldPositiveAdd(&self->droppingDatagramWarning, droppedDatagramWarning);
}

void nimbleClientConnectionQualityReceivedUsableDatagram(NimbleClientConnectionQuality* self)
{
    self->ticksWithoutIncomingDatagrams = 0;
    statsHoldPositiveAdd(&self->droppingDatagramWarning, false);
}

void nimbleClientConnectionQualityReceivedAuthoritativeSteps(NimbleClientConnectionQuality* self, size_t delta)
{
    (void) delta;
    self->ticksWithoutAuthoritativeStepsFromInSerialize = 0;
}

void nimbleClientConnectionQualityGameStepLatency(NimbleClientConnectionQuality* self, size_t latencyInMs)
{
    statsIntAdd(&self->latencyMsStat, (int) latencyInMs);
    // CLOG_C_NOTICE(&self->log, "set latency: %zu isSet:%d avg:%d", latencyInMs, self->latencyMsStat.avgIsSet,
    //             self->latencyMsStat.avg)
}

bool nimbleClientConnectionQualityShouldDisconnect(const NimbleClientConnectionQuality* self)
{
    return self->reason != NimbleConnectionQualityDisconnectReasonKeepConnection;
}
