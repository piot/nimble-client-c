/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/in_stream.h>
#include <imprint/allocator.h>
#include <nimble-client/client.h>
#include <nimble-client/download_state_part.h>
#include <nimble-client/download_state_response.h>
#include <nimble-client/game_step_response.h>
#include <nimble-client/incoming.h>
#include <nimble-client/join_game_response.h>
#include <nimble-serialize/debug.h>

static int readAndCheckOrderedDatagram(OrderedDatagramInLogic* inLogic, FldInStream* inStream, Clog* log)
{
    int idDelta = orderedDatagramInLogicReceive(inLogic, inStream);

    if (idDelta <= 0) {
        if (idDelta == 0) {
            CLOG_C_NOTICE(log, "packets received duplicate")
        } else {
            CLOG_C_NOTICE(log, "packets received out of order")
        }
        return -91;
    } else if (idDelta > 1) {
        CLOG_C_NOTICE(log, "%d packet(s) were dropped, but accepting sequence %04X", idDelta,
                      inLogic->lastReceivedSequence)
    }

    return idDelta;
}

/// Acts on the incoming octets received from the server
/// @param self
/// @param data
/// @param len
/// @return negative on error.
int nimbleClientFeed(NimbleClient* self, const uint8_t* data, size_t len)
{
    FldInStream inStream;
    fldInStreamInit(&inStream, data, len);

    int delta = readAndCheckOrderedDatagram(&self->orderedDatagramIn, &inStream, &self->log);
    if (delta < 0) {
        return delta;
    }
    if (delta > 1) {
        LagometerPacket droppedPacket = {LagometerPacketStatusDropped, 0, 0};
        for (int i = 0; i < delta - 1; ++i) {
            lagometerAddPacket(&self->lagometer, droppedPacket);
        }
    }

    bool droppedDatagramWarning = delta > 1;
    statsHoldPositiveAdd(&self->droppingDatagramWarning, droppedDatagramWarning);

    uint8_t cmd;
    fldInStreamReadUInt8(&inStream, &cmd);
    CLOG_C_VERBOSE(&self->log, "cmd: %s", nimbleSerializeCmdToString(cmd));

    int result = -1;
    switch (cmd) {
        case NimbleSerializeCmdGameStatePart:
            result = nimbleClientOnDownloadGameStatePart(self, &inStream);
            break;
        case NimbleSerializeCmdGameStepResponse:
            result = nimbleClientOnGameStepResponse(self, &inStream);
            break;
        case NimbleSerializeCmdJoinGameResponse:
            result = nimbleClientOnJoinGameResponse(self, &inStream);
            break;
        case NimbleSerializeCmdGameStateResponse:
            result = nimbleClientOnDownloadGameStateResponse(self, &inStream);
            break;
        default:
            CLOG_C_ERROR(&self->log, "unknown message %02X", cmd)
            return -1;
    }

    if (result >= 0) {
        if (inStream.pos != inStream.size) {
            CLOG_C_ERROR(&self->log, "did not read all of it")
        }
    }

    return result;
}
