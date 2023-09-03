/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <nimble-client/client.h>
#include <nimble-client/debug.h>

#if defined CLOG_LOG_ENABLED
static const char* stateToString(NimbleClientState state)
{
    static const char* lookup[] = {
        "idle",
        "requesting connect",
        "requesting game state",
        "downloading game state",
        "synced",
        "disconnected",
    };

    if (state >= sizeof(lookup) / sizeof(lookup[0])) {
        CLOG_ERROR("unknown nimble client state: %02X", state)
    }

    return lookup[state];
}
#endif

/// Outputs the current state of the client to logging
/// Only for debug purposes.
/// @param self nimble client
void nimbleClientDebugOutput(const NimbleClient* self)
{
    (void) self;
    CLOG_C_VERBOSE(&self->log, "nimbleClientState: %s", stateToString(self->state))
}
