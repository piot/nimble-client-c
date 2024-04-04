/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_PONG_H
#define NIMBLE_CLIENT_PONG_H

struct NimbleClient;
struct FldInStream;

int nimbleClientReceivePong(struct NimbleClient* client, struct FldInStream* inStream);

#endif
