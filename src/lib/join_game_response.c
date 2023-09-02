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

/*
static int readParticipantConnectionIdAndParticipants(NimbleClient* self, FldInStream* inStream)
{
    fldInStreamReadUInt8(inStream, &self->participantsConnectionIndex);

    uint8_t connectionBitMasks;
    fldInStreamReadUInt8(inStream, &connectionBitMasks);

    self->useDebugStreams = connectionBitMasks == 0x01;

    nimbleSerializeInConnectionSecret(inStream, &self->participantsConnectionSecret);

    uint8_t participantCount;
    fldInStreamReadUInt8(inStream, &participantCount);
    if (participantCount > NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT) {
        CLOG_SOFT_ERROR("we can not have more than %d local users (%d)", NIMBLE_CLIENT_MAX_LOCAL_USERS_COUNT,
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

    CLOG_C_DEBUG(&self->log, "joined game with connection secret %" PRIx64, self->participantsConnectionSecret)

    return participantCount;
}
 */

/// Handle join game response (NimbleSerializeCmdJoinGameResponse) from server.
/// @param self nimble protocol client
/// @param inStream stream to read from
/// @return negative on error
int nimbleClientOnJoinGameResponse(NimbleClient* self, FldInStream* inStream)
{
    NimbleSerializeGameResponse gameResponse;

    int err = nimbleSerializeClientInGameJoinResponse(inStream, &gameResponse);
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
