/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/out_stream.h>
#include <monotonic-time/lower_bits.h>
#include <nimble-client/client.h>
#include <nimble-client/prepare_header.h>

int nimbleClientPrepareOobHeader(NimbleClient* self, FldOutStream* outStream)
{
    (void) self;
    return fldOutStreamWriteUInt8(outStream, 0);
}

int nimbleClientPrepareHeader(NimbleClient* self, FldOutStream* outStream,
                              FldOutStreamStoredPosition* outStreamStoredPosition)
{
    CLOG_ASSERT(self->remoteConnectionId != 0, "must have a valid remote conneciton ID")
    fldOutStreamWriteUInt8(outStream, self->remoteConnectionId);
    *outStreamStoredPosition = fldOutStreamTell(outStream);
    connectionLayerOutgoingWrite(&self->connectionLayerOutgoing, outStream, 0, 0);
    orderedDatagramOutLogicPrepare(&self->orderedDatagramOut, outStream);
    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeLowerBitsMs lowerBitsMs = monotonicTimeMsToLowerBits(now);
    return fldOutStreamWriteUInt16(outStream, lowerBitsMs);
}

int nimbleClientCommitHeader(NimbleClient* self, FldOutStream* outStream, FldOutStreamStoredPosition writeHashPosition)
{
    FldOutStreamStoredPosition endPositionToRestore = fldOutStreamTell(outStream);
    size_t totalPacketSize = outStream->pos;
    fldOutStreamSeek(outStream, writeHashPosition);
    const int connectionLayerOctets = 4+1;
    int writeStatus = connectionLayerOutgoingWrite(&self->connectionLayerOutgoing, outStream, outStream->p + connectionLayerOctets,
                                                   totalPacketSize - outStream->pos - connectionLayerOctets);
    fldOutStreamSeek(outStream, endPositionToRestore);

    return writeStatus;
}
