/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <nimble-client/client.h>
#include <nimble-client/incoming.h>
#include <nimble-client/receive_transport.h>
#include <nimble-serialize/debug.h>

/// Reads from the unreliable datagram transport and feeds to the nimble client.
/// Feeds it to nimbleClientFeed().
/// @param self nimble protocol client
/// @return the number of datagrams received, or negative on error
ssize_t nimbleClientReceiveAllDatagramsFromTransport(NimbleClient* self)
{
#define UDP_MAX_RECEIVE_BUF_SIZE (1200)
    uint8_t receiveBuf[UDP_MAX_RECEIVE_BUF_SIZE];
    size_t count = 0;
    while (1) {
        ssize_t octetCount = datagramTransportReceive(&self->transport, receiveBuf, UDP_MAX_RECEIVE_BUF_SIZE);
        if (octetCount > 0) {
            if (self->useStats) {
                statsIntPerSecondAdd(&self->packetsPerSecondIn, 1);
            }
#if defined NIMBLE_CLIENT_LOG_VERBOSE
            nimbleSerializeDebugHex("received", receiveBuf, octetCount);
#endif
            int err = nimbleClientFeed(self, receiveBuf, (size_t) octetCount);
            if (err < 0) {
                return err;
            }
            nimbleClientConnectionQualityReceivedUsableDatagram(&self->quality);
            count++;
        } else if (octetCount < 0) {
            CLOG_SOFT_ERROR("nimbleClientReceiveAllDatagramsFromTransport: error: %zd", octetCount)
            return octetCount;
        } else {
            break;
        }
    }

    return (int) count;
}
