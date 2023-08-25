/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_NETWORK_REALIZE_H
#define NIMBLE_CLIENT_NETWORK_REALIZE_H

#include <nimble-client/client.h>
#include <nimble-client/network_realizer.h>
#include <nimble-serialize/types.h>
#include <stddef.h>

struct ImprintAllocator;

typedef enum NimbleClientRealizeState {
    NimbleClientRealizeStateInit,
    NimbleClientRealizeStateReInit,
    NimbleClientRealizeStateCleared,
    NimbleClientRealizeStateSynced,
    NimbleClientRealizeStateDisconnected
} NimbleClientRealizeState;

typedef struct NimbleClientRealizeSettings {
    DatagramTransport transport;
    struct ImprintAllocator* memory;
    struct ImprintAllocatorWithFree* blobMemory;
    size_t maximumSingleParticipantStepOctetCount;
    size_t maximumNumberOfParticipants;
    NimbleSerializeVersion applicationVersion;
    Clog log;
} NimbleClientRealizeSettings;

typedef struct NimbleClientRealize {
    NimbleClientRealizeState targetState;
    NimbleClientRealizeState state;
    NimbleClient client;
    NimbleClientRealizeSettings settings;
    NimbleSerializeNonce joinGameRequestNonce;
} NimbleClientRealize;

void nimbleClientRealizeInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings);
void nimbleClientRealizeReInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings);
void nimbleClientRealizeJoinGame(NimbleClientRealize* self, NimbleSerializeGameJoinOptions options);
void nimbleClientRealizeDestroy(NimbleClientRealize* self);
void nimbleClientRealizeReset(NimbleClientRealize* self);
void nimbleClientRealizeQuitGame(NimbleClientRealize* self);
void nimbleClientRealizeUpdate(NimbleClientRealize* self, MonotonicTimeMs now);

#endif
