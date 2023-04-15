/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <nimble-client/client.h>
#include <nimble-client/debug.h>

static const char* stateToString(NimbleClientState state)
{
    switch (state) {
        case NimbleClientStateIdle:
            return "idle";
        case NimbleClientStateJoiningGame:
            return "joining game";
        case NimbleClientStateJoinedGame:
            return "joined game";
        case NimbleClientStatePlaying:
            return "playing the game";
        case NimbleClientStateJoiningRequestingState:
            return "requesting start downloading state from server";
        case NimbleClientStateJoiningDownloadingState:
            return "downloading state from server";
    }

    CLOG_ERROR("unknown nimble client state: %02X", state)
}

void nimbleClientDebugOutput(const NimbleClient* self)
{
    CLOG_INFO("nimbleClientState: %s", stateToString(self->state))
}
