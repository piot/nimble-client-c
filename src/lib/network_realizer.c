/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <imprint/allocator.h>
#include <inttypes.h>
#include <nimble-client/client.h>
#include <nimble-client/debug.h>
#include <nimble-client/network_realizer.h>

/// Initializes the state machine
/// @param self client realize
/// @param settings settings
void nimbleClientRealizeInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings)
{
    self->targetState = NimbleClientRealizeStateInit;
    self->state = NimbleClientRealizeStateInit;
    self->settings = *settings;
    nimbleClientInit(&self->client, settings->memory, settings->blobMemory, &self->settings.transport,
                     settings->maximumSingleParticipantStepOctetCount, settings->maximumNumberOfParticipants,
                     settings->applicationVersion, settings->wantsDebugStreams, settings->log);
}

void nimbleClientRealizeReInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings)
{
    self->targetState = NimbleClientRealizeStateInit;
    self->state = NimbleClientRealizeStateReInit;
    self->settings = *settings;
    nimbleClientReInit(&self->client, &self->settings.transport);
}

void nimbleClientRealizeDestroy(NimbleClientRealize* self)
{
    nimbleClientDestroy(&self->client);
}

void nimbleClientRealizeReset(NimbleClientRealize* self)
{
    nimbleClientReset(&self->client);
    self->state = NimbleClientRealizeStateCleared;
    self->joinGameRequestNonce = 0;
}

/// Starts the joining of participants
/// Sends join request to the server and hopefully gets back a join result with the participant IDs.
/// @param self client realize
/// @param request join game request
void nimbleClientRealizeJoinGame(NimbleClientRealize* self, NimbleSerializeJoinGameRequest request)
{
    request.nonce = self->joinGameRequestNonce++;
    self->client.joinGameRequest = request;
    CLOG_C_DEBUG(&self->client.log, "setting state to join game. join type: %d, secret:%" PRIX64,
                 request.joinGameType, request.connectionSecret)
    self->targetState = NimbleClientRealizeStateSynced;
    self->client.joinParticipantPhase = NimbleJoiningStateJoiningParticipant;
}

void nimbleClientRealizeQuitGame(NimbleClientRealize* self)
{
    nimbleClientDisconnect(&self->client);
    self->targetState = NimbleClientRealizeStateCleared;
}

/// Updates the state machine
/// It tries to go from the current state to the targetState
/// @param self client realize
/// @param now current time
void nimbleClientRealizeUpdate(NimbleClientRealize* self, MonotonicTimeMs now)
{
#if defined CLOG_LOG_ENABLED
    if ((self->state != NimbleClientRealizeStateSynced) || self->state != self->targetState) {
        nimbleClientRealizeDebugOutput(self);
    }
#endif

    if (self->client.state == NimbleClientStateDisconnected) {
        // If underlying nimble client has given up, we need to follow the example
        self->state = NimbleClientRealizeStateDisconnected;
        return;
    }

    if (self->state != NimbleClientRealizeStateCleared && self->targetState != NimbleClientRealizeStateInit) {
        nimbleClientUpdate(&self->client, now);
    }

    if (self->targetState == self->state) {
        return;
    }

    switch (self->targetState) {
        case NimbleClientRealizeStateInit:
            break;
        case NimbleClientRealizeStateReInit:
            break;
        case NimbleClientRealizeStateDisconnected:
            break;
        case NimbleClientRealizeStateCleared:
            if (self->state != NimbleClientRealizeStateCleared) {
                self->state = self->targetState;
            }
            break;
        case NimbleClientRealizeStateSynced:
            switch (self->client.state) {
                case NimbleClientStateIdle:
                    self->client.state = NimbleClientStateRequestingConnect;
                    self->client.waitTime = 0;
                    break;
                case NimbleClientStateConnected:
                    self->client.state = NimbleClientStateJoiningRequestingState;
                    self->client.joinParticipantPhase = NimbleJoiningStateJoiningParticipant;
                    self->client.waitTime = 0;
                    break;
                case NimbleClientStateSynced:
                    self->state = NimbleClientRealizeStateSynced;
                    self->client.waitTime = 0;
                    break;

                // If there is something in progress, do not do anything
                case NimbleClientStateRequestingConnect:
                case NimbleClientStateJoiningRequestingState:
                case NimbleClientStateJoiningDownloadingState:
                case NimbleClientStateDisconnected:
                    break;
            }

            break;
    }
}
