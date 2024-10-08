/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/download_state_response.h>
#include <nimble-serialize/serialize.h>

/// Handle incoming game state response (NimbleSerializeCmdGameStateResponse) from server.
/// If response haven't been received before, it sets the joinStateChannel and
/// sets the state to NimbleClientStateJoiningDownloadingState, to start receiving the game state.
/// @param self nimble protocol client
/// @param inStream stream to read from
/// @return negative on error
int nimbleClientOnDownloadGameStateResponse(NimbleClient* self, FldInStream* inStream)
{
    NimbleSerializeStateId stateId;

    uint8_t clientRequestId;
    fldInStreamReadUInt8(inStream, &clientRequestId);

    nimbleSerializeInStateId(inStream, &stateId);

    NimbleSerializeBlobStreamChannelId channelId;
    int errorCode = nimbleSerializeInBlobStreamChannelId(inStream, &channelId);
    if (errorCode < 0) {
        CLOG_SOFT_ERROR("could not serialize channel %d", errorCode)
        return errorCode;
    }

    if (clientRequestId != self->downloadStateClientRequestId) {
        CLOG_C_NOTICE(&self->log, "got download game state reply for another request, ignoring")
        return 0;
    }

    if (channelId == self->joinStateChannel) {
        CLOG_C_VERBOSE(&self->log, "already have this join state %u", self->joinStateChannel)
        return 0;
    }

    self->joinStateChannel = channelId;

    CLOG_C_VERBOSE(&self->log, "rejoin answer: stateId: %04X channel:%02X", stateId,channelId)


    self->joinedGameState.stepId = stateId;
    self->joinStateId = stateId;
    CLOG_C_DEBUG(&self->log, "start predicting from %08X", stateId)
    nbsStepsReInit(&self->outSteps, stateId);
    self->state = NimbleClientStateJoiningDownloadingState;
    self->waitTime = 0;

    return 0;
}
