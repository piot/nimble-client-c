/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/out_stream.h>
#include <monotonic-time/lower_bits.h>
#include <nimble-client/client.h>
#include <nimble-client/prepare_header.h>

int nimbleClientPrepareHeader(NimbleClient* self, FldOutStream* outStream,
                              FldOutStreamStoredPosition* outStreamStoredPosition)
{
    fldOutStreamWriteUInt8(outStream, self->remoteConnectionId);
    if (self->remoteConnectionId != 0) {
        *outStreamStoredPosition = fldOutStreamTell(outStream);
        connectionLayerOutgoingWrite(&self->connectionLayerOutgoing, outStream, 0, 0);
    } else {
        outStreamStoredPosition->extraVerification = 0;
    }
    orderedDatagramOutLogicPrepare(&self->orderedDatagramOut, outStream);
    MonotonicTimeMs now = monotonicTimeMsNow();
    MonotonicTimeLowerBitsMs lowerBitsMs = monotonicTimeMsToLowerBits(now);
    return fldOutStreamWriteUInt16(outStream, lowerBitsMs);
}

int nimbleClientCommitHeader(NimbleClient* self, FldOutStream* outStream, FldOutStreamStoredPosition writeHashPosition)
{
    if (writeHashPosition.extraVerification == 0) {
        return 0;
    }

    FldOutStreamStoredPosition endPositionToRestore = fldOutStreamTell(outStream);
    size_t totalPacketSize = outStream->pos;
    fldOutStreamSeek(outStream, writeHashPosition);
    int writeStatus = connectionLayerOutgoingWrite(&self->connectionLayerOutgoing, outStream, outStream->p + 4,
                                                   totalPacketSize - outStream->pos - 4);
    fldOutStreamSeek(outStream, endPositionToRestore);

    return writeStatus;
}
