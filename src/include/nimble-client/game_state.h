/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_GAME_STATE_H
#define NIMBLE_CLIENT_GAME_STATE_H

#include <stdint.h>
#include <nimble-steps/steps.h>

typedef struct NimbleClientGameState {
    const uint8_t* gameState;
    size_t gameStateOctetCount;
    StepId stepId;
    struct ImprintAllocatorWithFree* blobAllocator;
} NimbleClientGameState;

void nimbleClientGameStateInit(NimbleClientGameState* self, struct ImprintAllocatorWithFree* blobAllocator, StepId stepId, const uint8_t* gameState,
                               size_t gameStateOctetCount);
void nimbleClientGameStateReset(NimbleClientGameState* self);
void nimbleClientGameStateDestroy(NimbleClientGameState* self);
void nimbleClientGameStateDebug(const NimbleClientGameState* self, const char* debug);

#endif
