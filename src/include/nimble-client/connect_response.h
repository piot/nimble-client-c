/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_CONNECT_RESPONSE_H
#define NIMBLE_CLIENT_CONNECT_RESPONSE_H

struct NimbleClient;
struct FldInStream;

int nimbleClientOnConnectResponse(struct NimbleClient* self, struct FldInStream* inStream);

#endif
