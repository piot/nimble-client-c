/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/join_game_response.h>
#include <nimble-serialize/client_in.h>
#include <inttypes.h>

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

    if (self->joinParticipantPhase != NimbleJoiningStateJoiningParticipant) {
        CLOG_C_VERBOSE(&self->log, "ignoring join game response. We are not in the process of joining (anymore?)")
        return 0;
    }

    self->participantsConnectionIndex = gameResponse.participantConnectionIndex;
    self->participantsConnectionSecret = gameResponse.participantConnectionSecret;
    self->localParticipantCount = gameResponse.participantCount;

    for (size_t i = 0; i < gameResponse.participantCount; ++i) {
        self->localParticipantLookup[i].localUserDeviceIndex = (uint8_t) gameResponse.participants[i].localIndex;
        self->localParticipantLookup[i].participantId = (uint8_t) gameResponse.participants[i].id;
    }

    CLOG_C_DEBUG(&self->log, "join game response. connection index %d participant count: %zu secret: %" PRIX64,
                 self->participantsConnectionIndex, self->localParticipantCount, self->participantsConnectionSecret)

    self->joinParticipantPhase = NimbleJoiningStateJoinedParticipant;
    self->waitTime = 0;
    self->joinStateChannel = 0;

    return 0;
}
