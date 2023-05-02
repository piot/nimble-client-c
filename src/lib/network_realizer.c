/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <imprint/allocator.h>
#include <nimble-client/client.h>
#include <nimble-client/network_realizer.h>

/// Initializes the state machine
/// @param self
/// @param settings
void nimbleClientRealizeInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings)
{
    self->targetState = NimbleClientRealizeStateInit;
    self->state = NimbleClientRealizeStateInit;
    self->settings = *settings;
    nimbleClientInit(&self->client, settings->memory, settings->blobMemory, &self->settings.transport,
                     settings->maximumSingleParticipantStepOctetCount, settings->maximumNumberOfParticipants,
                     settings->applicationVersion, settings->log);
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
}

void nimbleClientRealizeJoinGame(NimbleClientRealize* self, NimbleSerializeGameJoinOptions options)
{
    self->client.joinGameOptions = options;
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
/// @param self
/// @param now
/// @param targetFps the recommended tick-rate given the conditions
void nimbleClientRealizeUpdate(NimbleClientRealize* self, MonotonicTimeMs now, size_t* targetFps)
{
    if (self->state != NimbleClientRealizeStateCleared && self->targetState != NimbleClientRealizeStateInit) {
        nimbleClientUpdate(&self->client, now);
    }

    *targetFps = 60;
    if (self->client.authoritativeStepsFromServer.stepsCount > 2) {
        *targetFps = 70;
    }

    if (self->targetState == self->state) {
        return;
    }

    switch (self->targetState) {
        case NimbleClientRealizeStateCleared:
            if (self->state != NimbleClientRealizeStateCleared) {
                self->state = self->targetState;
            }
            break;
        case NimbleClientRealizeStateSynced:
            if (self->client.state == NimbleClientStateIdle) {
                self->client.state = NimbleClientStateJoiningRequestingState;
                self->client.waitTime = 0;
            } else if (self->client.state == NimbleClientStateSynced) {
                self->state = NimbleClientRealizeStateSynced;
            }
        default:
            break;
    }
}
