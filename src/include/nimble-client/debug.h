/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_DEBUG_H
#define NIMBLE_CLIENT_DEBUG_H

struct NimbleClient;
struct NimbleClientRealize;

void nimbleClientDebugOutput(const struct NimbleClient* self);
void nimbleClientRealizeDebugOutput(const struct NimbleClientRealize* self);

#endif
