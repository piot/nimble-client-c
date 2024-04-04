/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_JOIN_GAME_PARTICIPANTS_FULL_H
#define NIMBLE_CLIENT_JOIN_GAME_PARTICIPANTS_FULL_H

struct NimbleClient;
struct FldInStream;

int nimbleClientOnJoinGameParticipantOutOfSpaceResponse(struct NimbleClient* self, struct FldInStream* inStream);

#endif
