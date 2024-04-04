/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_PREPARE_HEADER_H
#define NIMBLE_CLIENT_PREPARE_HEADER_H

struct NimbleClient;
struct FldOutStream;

int nimbleClientPrepareHeader(struct NimbleClient* self, struct FldOutStream* outStream);

#endif

