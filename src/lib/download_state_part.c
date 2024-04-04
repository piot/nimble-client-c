/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/download_state_part.h>
#include <nimble-serialize/serialize.h>

static void trySetInitialGameState(NimbleClient* self)
{
    if (self->state == NimbleClientStateSynced) {
        CLOG_C_NOTICE(&self->log, "we are already synced, ignoring that state is ready")
        return;
    }
    CLOG_C_INFO(&self->log, "=====================================================================")
    CLOG_C_INFO(&self->log, "we have downloaded the game state %04X", self->joinedGameState.stepId)
    self->state = NimbleClientStateSynced;
    nimbleClientGameStateInit(&self->joinedGameState, self->blobStreamAllocator, self->joinStateId,
                              self->blobStreamIn.blob, self->blobStreamIn.octetCount);
    nbsPendingStepsReset(&self->authoritativePendingStepsFromServer, self->joinedGameState.stepId);
    nbsStepsReInit(&self->authoritativeStepsFromServer, self->joinedGameState.stepId);
    // we should start predicting from this stepId as well
    nbsStepsReInit(&self->outSteps, self->joinedGameState.stepId);

}

/// Handle incoming message NimbleSerializeCmdGameStatePart
/// Receives a blob stream chunk. If the blob stream is completed, it sets the game state to joinedGameState
/// and goes to NimbleClientStatePlaying.
/// @param self nimble protocol client
/// @param inStream stream to read download game state part from
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
        trySetInitialGameState(self);
    }

    return 0;
}
