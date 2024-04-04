/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <nimble-client/client.h>
#include <nimble-client/debug.h>
#include <nimble-client/network_realizer.h>

#if defined CLOG_LOG_ENABLED
static const char* nimbleClientStateToString(NimbleClientState state)
{
    static const char* lookup[] = {
        "idle",   "requesting connect", "connected", "requesting game state", "downloading game state",
        "synced", "disconnected",
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
#if defined CLOG_LOG_ENABLED
    CLOG_C_VERBOSE(&self->log, "state: %s", nimbleClientStateToString(self->state))
#else
    (void) self;
#endif
}

#if defined CLOG_LOG_ENABLED
static const char* nimbleClientRealizeStateToString(NimbleClientRealizeState state)
{
    static const char* lookup[] = {
        "init",         // NimbleClientRealizeStateInit,
        "reinit",       // NimbleClientRealizeStateReInit,
        "cleared",      // NimbleClientRealizeStateCleared,
        "synced",       // NimbleClientRealizeStateSynced,
        "disconnected", // NimbleClientRealizeStateDisconnected
    };

    if (state >= sizeof(lookup) / sizeof(lookup[0])) {
        CLOG_ERROR("unknown nimble client realize state: %02X", state)
    }

    return lookup[state];
}
#endif

/// Outputs the current state of the client to logging
/// Only for debug purposes.
/// @param self nimble client realizer
void nimbleClientRealizeDebugOutput(const NimbleClientRealize* self)
{
#if defined CLOG_LOG_ENABLED
    if (self->state != self->targetState) {
        CLOG_C_VERBOSE(&self->client.log, "realize state. now: %s, but trying to reach: %s",
                       nimbleClientRealizeStateToString(self->state),
                       nimbleClientRealizeStateToString(self->targetState))
    } else {
        CLOG_C_VERBOSE(&self->client.log, "realize state: %s", nimbleClientRealizeStateToString(self->state))
    }
#else
    (void) self;
#endif
}
