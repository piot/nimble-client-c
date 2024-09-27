/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/out_stream.h>
#include <monotonic-time/lower_bits.h>
#include <nimble-client/client.h>
#include <nimble-client/prepare_header.h>


int nimbleClientWriteHeader(NimbleClient* self, FldOutStream* outStream)
{
    CLOG_ASSERT(self->remoteConnectionId != 0, "must have a valid remote conneciton ID")
    orderedDatagramOutLogicPrepare(&self->orderedDatagramOut, outStream);
    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeLowerBitsMs lowerBitsMs = monotonicTimeMsToLowerBits(now);
    return fldOutStreamWriteUInt16(outStream, lowerBitsMs);
}

void nimbleClientCommitHeader(NimbleClient* self)
{
    orderedDatagramOutLogicCommit(&self->orderedDatagramOut);
}
