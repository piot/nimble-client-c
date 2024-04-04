/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <nimble-client/prepare_header.h>
#include <flood/out_stream.h>
#include <nimble-client/client.h>
#include <monotonic-time/lower_bits.h>

int nimbleClientPrepareHeader(NimbleClient* self, FldOutStream* outStream)
{
    orderedDatagramOutLogicPrepare(&self->orderedDatagramOut, outStream);
    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeLowerBitsMs lowerBitsMs = monotonicTimeMsToLowerBits(now);
    return fldOutStreamWriteUInt16(outStream, lowerBitsMs);
}
