/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include "nimble-client/download_state_part.h"
#include "nimble-client/download_state_response.h"
#include "nimble-client/game_step_response.h"
#include "nimble-client/join_game_response.h"
#include <clog/clog.h>
#include <flood/in_stream.h>
#include <imprint/allocator.h>
#include <nimble-client/client.h>
#include <nimble-client/incoming.h>
#include <nimble-serialize/debug.h>

int nimbleClientFeed(NimbleClient* self, const uint8_t* data, size_t len)
{
    FldInStream inStream;
    fldInStreamInit(&inStream, data, len);

    int idDelta = orderedDatagramInLogicReceive(&self->orderedDatagramIn, &inStream);

    uint8_t cmd;
    fldInStreamReadUInt8(&inStream, &cmd);
    CLOG_C_VERBOSE(&self->log, "nimbleClient: cmd: %s", nimbleSerializeCmdToString(cmd));
    if (idDelta <= 0) {
        CLOG_C_NOTICE(&self->log, "packets received duplicate or out of order")
        return -91;
    } else if (idDelta > 1) {
        CLOG_C_NOTICE(&self->log, "packets were dropped, but accepting this one")
    }

    switch (cmd) {
        case NimbleSerializeCmdGameStatePart:
            return nimbleClientOnDownloadGameStatePart(self, &inStream);
        case NimbleSerializeCmdGameStepResponse:
            return nimbleClientOnGameStepResponse(self, &inStream);
        case NimbleSerializeCmdJoinGameResponse:
            return nimbleClientOnJoinGameResponse(self, &inStream);
        case NimbleSerializeCmdGameStateResponse:
            return nimbleClientOnDownloadGameStateResponse(self, &inStream);
        default:
            CLOG_ERROR("unknown message %02X", cmd)
            return -1;
    }
    return 0;
}

