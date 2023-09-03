/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <inttypes.h>
#include <nimble-client/client.h>
#include <nimble-client/join_game_response.h>
#include <nimble-serialize/client_in.h>
#include <nimble-serialize/serialize.h>

/// Handle join game response (NimbleSerializeCmdJoinGameResponse) from server.
/// @param self nimble protocol client
/// @param inStream stream to read from
/// @return negative on error
int nimbleClientOnJoinGameResponse(NimbleClient* self, FldInStream* inStream)
{
    NimbleSerializeJoinGameResponse gameResponse;

    int err = nimbleSerializeClientInJoinGameResponse(inStream, &gameResponse);
    if (err < 0) {
        return err;
    }

    self->participantsConnectionIndex = gameResponse.participantConnectionIndex;
    self->useDebugStreams = gameResponse.participantConnectionSecret;
    self->participantsConnectionSecret = gameResponse.participantConnectionSecret;
    self->localParticipantCount = gameResponse.participantCount;

    for (size_t i = 0; i < gameResponse.participantCount; ++i) {
        self->localParticipantLookup[i].localUserDeviceIndex = (uint8_t) gameResponse.participants[i].localIndex;
        self->localParticipantLookup[i].participantId = (uint8_t) gameResponse.participants[i].id;
    }

    CLOG_INFO("join game response. Connection index %d participants: %zu", self->participantsConnectionIndex,
              self->localParticipantCount)

    self->joinParticipantPhase = NimbleJoiningStateJoinedParticipant;
    self->waitTime = 0;
    self->joinStateChannel = 0;
    self->ticksWithoutAuthoritativeStepsFromInSerialize = 0;

    return 0;
}
