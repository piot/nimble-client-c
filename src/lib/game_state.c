/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <imprint/allocator.h>
#include <nimble-client/game_state.h>

/// Initializes and allocates memory for the Game State
/// @param self
/// @param blobAllocator
/// @param stepId the tickId where it was captured
/// @param gameState
/// @param gameStateOctetCount
void nimbleClientGameStateInit(NimbleClientGameState* self, struct ImprintAllocatorWithFree* blobAllocator,
                               StepId stepId, const uint8_t* gameState, size_t gameStateOctetCount)
{
    self->stepId = stepId;
    self->gameState = IMPRINT_ALLOC((ImprintAllocator*) blobAllocator, gameStateOctetCount, "gamestate");
    self->gameStateOctetCount = gameStateOctetCount;
    self->blobAllocator = blobAllocator;
    tc_memcpy_octets((void*) self->gameState, gameState, gameStateOctetCount);
}

/// Free the memory allocated on initialize.
/// @param self
void nimbleClientGameStateDestroy(NimbleClientGameState* self)
{
    IMPRINT_FREE(self->blobAllocator, self->gameState);
    self->gameState = 0;
    self->gameStateOctetCount = 0;
}

/// Logs the internal information about the game state. only for debugging.
/// @param self
/// @param debug
void nimbleClientGameStateDebug(const NimbleClientGameState* self, const char* debug)
{
    CLOG_INFO("game state '%s' stepId: %08X octetCount: %zu", debug, self->stepId, self->gameStateOctetCount);
}
