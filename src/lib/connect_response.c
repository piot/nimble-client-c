/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <flood/in_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/connect_response.h>
#include <nimble-serialize/client_in.h>

/// Handle join game response (NimbleSerializeCmdJoinGameResponse) from server.
/// @param self nimble protocol client
/// @param inStream stream to read from
/// @return negative on error
int nimbleClientOnConnectResponse(NimbleClient* self, FldInStream* inStream)
{
    NimbleSerializeConnectResponse response;

    int err = nimbleSerializeClientInConnectResponse(inStream, &response);
    if (err < 0) {
        return err;
    }

    if (self->state == NimbleClientStateRequestingConnect) {
        self->state = NimbleClientStateConnected;
        self->useDebugStreams = response.useDebugStreams;
        self->remoteConnectionId = response.connectionId;
        CLOG_ASSERT(self->remoteConnectionId != 0, "remote connection assigned can not be zero")
        CLOG_C_DEBUG(&self->log, "connected. remote connectionId:%hhu debug streams: %d", self->remoteConnectionId, self->useDebugStreams)
    }

    return 0;
}
