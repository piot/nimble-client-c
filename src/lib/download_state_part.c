/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <nimble-client/download_state_part.h>
#include <nimble-client/client.h>
#include <flood/in_stream.h>
#include <nimble-serialize/serialize.h>

/// Handle incoming message NimbleSerializeCmdGameStatePart
/// Receives a blob stream chunk. If the blob stream is completed, it sets the game state to joinedGameState
/// and goes to NimbleClientStatePlaying.
/// @param self
/// @param inStream
/// @return negative numbers on error.
int nimbleClientOnDownloadGameStatePart(NimbleClient* self, FldInStream* inStream)
{
    NimbleSerializeBlobStreamChannelId channelId;

    nimbleSerializeInBlobStreamChannelId(inStream, &channelId);
    if (channelId != self->joinStateChannel) {
        CLOG_SOFT_ERROR("we received response from wrong channel %04X", self->joinStateChannel)
        return 0;
    }

    self->joinStateChannel = channelId;

    int result = blobStreamLogicInReceive(&self->blobStreamInLogic, inStream);
    if (result < 0) {
        return result;
    }

    if (blobStreamInIsComplete(&self->blobStreamIn)) {
        CLOG_DEBUG("we have downloaded the join state")
        self->state = NimbleClientStatePlaying;
        nimbleClientGameStateInit(&self->joinedGameState, self->blobStreamAllocator, self->joinStateId,
                                  self->blobStreamIn.blob, self->blobStreamIn.octetCount);
    }

    return 0;
}
