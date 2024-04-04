/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/join_game_participants_full.h>
#include <nimble-serialize/serialize.h>

/// Handle join game response (NimbleSerializeCmdJoinGameResponse) from server.
/// @param self nimble protocol client
/// @param inStream stream to read from
/// @return negative on error
int nimbleClientOnJoinGameParticipantOutOfSpaceResponse(NimbleClient* self, FldInStream* inStream)
{
    (void) self;
    NimbleSerializeNonce nonce;

    int err = nimbleSerializeInNonce(inStream, &nonce);
    if (err < 0) {
        return err;
    }

    self->joinParticipantPhase = NimbleJoiningStateOutOfParticipantSlots;

    return 0;
}
