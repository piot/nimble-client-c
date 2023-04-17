/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include "nimble-serialize/debug.h"
#include <nimble-client/client.h>
#include <nimble-client/incoming.h>
#include <nimble-client/receive_transport.h>

/// Reads from the datagram transport and feeds to the nimble client.
/// @param self
/// @return
int nimbleClientReceiveAllInUdpBuffer(NimbleClient* self)
{
#define UDP_MAX_RECEIVE_BUF_SIZE (1200)
    static uint8_t receiveBuf[UDP_MAX_RECEIVE_BUF_SIZE];
    size_t count = 0;
    while (1) {
        int octetCount = udpTransportReceive(&self->transport, receiveBuf, UDP_MAX_RECEIVE_BUF_SIZE);
        if (octetCount > 0) {
            if (self->useStats) {
                statsIntPerSecondAdd(&self->packetsPerSecondIn, 1);
            }
#if 0
            nimbleSerializeDebugHex("received", receiveBuf, octetCount);
#endif
            nimbleClientFeed(self, receiveBuf, octetCount);
            count++;
        } else if (octetCount < 0) {
            CLOG_SOFT_ERROR("nimbleClientReceiveAllInUdpBuffer: error: %d", octetCount);
            return octetCount;
        } else {
            break;
        }
    }

    return (int)count;
}
