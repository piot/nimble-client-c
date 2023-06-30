/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_GAME_STEP_RESPONSE_H
#define NIMBLE_CLIENT_GAME_STEP_RESPONSE_H

struct NimbleClient;
struct FldInStream;

#include <stddef.h>

ssize_t nimbleClientOnGameStepResponse(struct NimbleClient* self, struct FldInStream* inStream);

#endif
