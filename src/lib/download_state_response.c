/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/download_state_response.h>
#include <nimble-serialize/serialize.h>

/// Handle incoming game state response (NimbleSerializeCmdGameStateResponse) from server.
/// If response haven't been received before, it sets the joinStateChannel and
/// sets the state to NimbleClientStateJoiningDownloadingState, to start receiving the game state.
/// @param self
/// @param inStream
/// @return
int nimbleClientOnDownloadGameStateResponse(NimbleClient* self, FldInStream* inStream)
{
    NimbleSerializeStateId stateId;

    nimbleSerializeInStateId(inStream, &stateId);

    uint32_t octetCount;
    fldInStreamReadUInt32(inStream, &octetCount);

    NimbleSerializeBlobStreamChannelId channelId;
    int errorCode = nimbleSerializeInBlobStreamChannelId(inStream, &channelId);
    if (errorCode < 0) {
        CLOG_SOFT_ERROR("could not serialize channel %d", errorCode)
        return errorCode;
    }

    if (channelId == self->joinStateChannel) {
        CLOG_SOFT_ERROR("already have this join state %u", self->joinStateChannel)
        return 0;
    }

    self->joinStateChannel = channelId;

    CLOG_VERBOSE("rejoin answer: stateId: %04X octetCount:%u channel:%02X", stateId, octetCount, channelId)

    blobStreamInInit(&self->blobStreamIn, self->memory, self->blobStreamAllocator, octetCount, BLOB_STREAM_CHUNK_SIZE, self->log);
    blobStreamLogicInInit(&self->blobStreamInLogic, &self->blobStreamIn);

    self->joinedGameState.stepId = stateId;
    self->joinStateId = stateId;
    self->nextStepIdToSendToServer = stateId;

    self->state = NimbleClientStateJoiningDownloadingState;
    self->waitTime = 0;

    return 0;
}
