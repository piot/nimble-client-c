/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <nimble-client/client.h>
#include <nimble-client/debug.h>

static const char* stateToString(NimbleClientState state)
{
    static const char* lookup[] = {
        "idle",
        "requesting game state",
        "downloading game state",
        "synced",
    };

    if (state < 0 || state >= sizeof(lookup) / sizeof(lookup[0])) {
        CLOG_ERROR("unknown nimble client state: %02X", state)
    }

    return lookup[state];
}

void nimbleClientDebugOutput(const NimbleClient* self)
{
    CLOG_INFO("nimbleClientState: %s", stateToString(self->state))
}
