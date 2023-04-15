/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/join_game_response.h>

static int readParticipantConnectionIdAndParticipants(NimbleClient* self, FldInStream* inStream)
{
    fldInStreamReadUInt8(inStream, &self->participantsConnectionIndex);

    uint8_t participantCount;
    fldInStreamReadUInt8(inStream, &participantCount);
    if (participantCount > NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT) {
        CLOG_ERROR("we can not have more than %d local users (%d)", NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT,
                   participantCount)
        return -1;
    }

    self->localParticipantCount = participantCount;

    for (size_t i = 0; i < participantCount; ++i) {
        uint8_t localIndex;
        fldInStreamReadUInt8(inStream, &localIndex);
        uint8_t participantId;
        fldInStreamReadUInt8(inStream, &participantId);
        CLOG_INFO("** -> I am participant id: %d (localIndex:%d)", participantId, localIndex)
        self->localParticipantLookup[i].localUserDeviceIndex = localIndex;
        self->localParticipantLookup[i].participantId = participantId;
    }

    return participantCount;
}

/// Handle join game response (NimbleSerializeCmdJoinGameResponse) from server.
/// @param self
/// @param inStream
/// @return
int nimbleClientOnJoinGameResponse(NimbleClient* self, FldInStream* inStream)
{
    int participantCount = readParticipantConnectionIdAndParticipants(self, inStream);
    if (participantCount <= 0) {
        CLOG_SOFT_ERROR("couldn't read participant connection Id and participants")
        return participantCount;
    }

    CLOG_INFO("join game response. Connection index %d participants: %d",
              self->participantsConnectionIndex, participantCount)

    self->state = NimbleClientStateJoinedGame;
    self->waitTime = 0;
    self->joinStateChannel = 0;

    return 0;
}
