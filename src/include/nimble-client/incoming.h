/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_INCOMING_H
#define NIMBLE_CLIENT_INCOMING_H

struct NimbleClient;

int nimbleClientFeed(struct NimbleClient* self, const uint8_t* data, size_t len);

#endif
