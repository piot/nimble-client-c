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
    nimbleClientInit(&self->client, settings->memory, settings->blobMemory,  &self->settings.transport);
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
    self->targetState = NimbleClientRealizeStateInGame;
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
/// @param targetFps
void nimbleClientRealizeUpdate(NimbleClientRealize* self, MonotonicTimeMs now, size_t* targetFps)
{
    if (self->state != NimbleClientRealizeStateCleared && self->targetState != NimbleClientRealizeStateInit) {
        nimbleClientUpdate(&self->client, now);
    }

    *targetFps = 60;
    if (self->client.authoritativeStepsFromServer.stepsCount > 2) {
        *targetFps = 70;
    }

    if (self->targetState == self->state)
    {
        return;
    }

    switch (self->targetState) {
        case NimbleClientRealizeStateCleared:
            if (self->state != NimbleClientRealizeStateCleared) {
                self->state = self->targetState;
            }
            break;
        case NimbleClientRealizeStateInGame:
            if (self->client.state == NimbleClientStateIdle) {
                self->client.state = NimbleClientStateJoiningGame;
                self->client.waitTime = 0;
            }
            if (self->client.state == NimbleClientStateJoinedGame) {
                self->client.state = NimbleClientStateJoiningRequestingState;
            }
            if (self->client.state == NimbleClientStatePlaying)
            {
                self->state = self->targetState;
            }
        default:
            break;
    }
}

