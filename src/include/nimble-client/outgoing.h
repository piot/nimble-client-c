/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_OUTGOING_H
#define NIMBLE_CLIENT_OUTGOING_H

struct NimbleClient;
struct UdpTransportOut;

int nimbleClientOutgoing(struct NimbleClient* self, struct UdpTransportOut* transportOut);

#endif
