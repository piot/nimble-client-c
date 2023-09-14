/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <nimble-client/pong.h>
#include <nimble-client/client.h>
#include <flood/in_stream.h>
#include <monotonic-time/monotonic_time.h>
#include <monotonic-time/lower_bits.h>

int nimbleClientReceivePong(NimbleClient* self, FldInStream* inStream)
{
    fldInStreamCheckMarker(inStream, 0xdd);
    MonotonicTimeLowerBitsMs monotonicTimeShortMs;
    int readResult = fldInStreamReadUInt16(inStream, &monotonicTimeShortMs);
    if (readResult < 0) {
        return readResult;
    }

    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeMs sentAt = monotonicTimeMsFromLowerBits(now, monotonicTimeShortMs);

    if (now < sentAt) {
        CLOG_C_NOTICE(&self->log, "time problems in lower bits")
    } else {
        self->latencyMs = (size_t) (now - sentAt);
    }

    nimbleClientConnectionQualityGameStepLatency(&self->quality, self->latencyMs);

    if (self->useStats) {
        statsIntAdd(&self->latencyMsStat, (int) self->latencyMs);
    }

    return 0;
}
