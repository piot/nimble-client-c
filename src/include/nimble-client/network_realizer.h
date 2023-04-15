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
    NimbleClientRealizeStateInGame
} NimbleClientRealizeState;

typedef struct NimbleClientRealizeSettings {
    UdpTransportInOut transport;
    struct ImprintAllocator* memory;
    struct ImprintAllocatorWithFree* blobMemory;
} NimbleClientRealizeSettings;

typedef struct NimbleClientRealize {
    NimbleClientRealizeState targetState;
    NimbleClientRealizeState state;
    NimbleClient client;
    NimbleClientRealizeSettings settings;
} NimbleClientRealize;

void nimbleClientRealizeInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings);
void nimbleClientRealizeReInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings);
void nimbleClientRealizeJoinGame(NimbleClientRealize* self, NimbleSerializeGameJoinOptions options);
void nimbleClientRealizeDestroy(NimbleClientRealize* self);
void nimbleClientRealizeReset(NimbleClientRealize* self);
void nimbleClientRealizeQuitGame(NimbleClientRealize* self);
void nimbleClientRealizeUpdate(NimbleClientRealize* self, MonotonicTimeMs now, size_t* targetFps);

#endif
