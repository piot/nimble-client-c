/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
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

    self->useDebugStreams = response.useDebugStreams;
    if (self->state == NimbleClientStateRequestingConnect) {
        self->state = NimbleClientStateConnected;
    }

    return 0;
}
