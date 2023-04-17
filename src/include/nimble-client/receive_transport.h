/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_RECEIVE_TRANSPORT_H
#define NIMBLE_CLIENT_RECEIVE_TRANSPORT_H

struct NimbleClient;

int nimbleClientReceiveAllInUdpBuffer(struct NimbleClient* self);

#endif
