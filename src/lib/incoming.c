/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/in_stream.h>
#include <imprint/allocator.h>
#include <nimble-client/client.h>
#include <nimble-client/connect_response.h>
#include <nimble-client/download_state_part.h>
#include <nimble-client/download_state_response.h>
#include <nimble-client/game_step_response.h>
#include <nimble-client/incoming.h>
#include <nimble-client/join_game_participants_full.h>
#include <nimble-client/join_game_response.h>
#include <nimble-client/pong.h>
#include <nimble-serialize/debug.h>

static int readAndCheckOrderedDatagram(OrderedDatagramInLogic* inLogic, FldInStream* inStream, Clog* log)
{
    (void) log;

    int idDelta = orderedDatagramInLogicReceive(inLogic, inStream);

    if (idDelta <= 0) {
        return -91;
    }

    return idDelta;
}

/// Acts on the incoming octets received from the server
/// @param self nimble protocol client
/// @param data received octet payload
/// @param len octet length of data
/// @return negative on error.
int nimbleClientFeed(NimbleClient* self, const uint8_t* data, size_t len)
{
    FldInStream inStream;
    fldInStreamInit(&inStream, data, len);
    inStream.readDebugInfo = true;

    if (len < 1) {
        return -1;
    }

    if (self->state == NimbleClientStateDisconnected) {
        CLOG_C_VERBOSE(&self->log, "we received a packet, but we are not connected. ignoring.")
        return 0;
    }

    int delta = readAndCheckOrderedDatagram(&self->orderedDatagramIn, &inStream, &self->log);
    if (delta < 0) {
        return delta;
    }
    if (delta > 1) {
        LagometerPacket droppedPacket = {LagometerPacketStatusDropped, 0, 0};
        for (int i = 0; i < delta - 1; ++i) {
            lagometerAddPacket(&self->lagometer, droppedPacket);
        }
        nimbleClientConnectionQualityDroppedDatagrams(&self->quality, (size_t) (delta - 1));
    }

    int err = nimbleClientReceivePong(self, &inStream);
    if (err < 0) {
        return err;
    }

    uint8_t cmd;
    fldInStreamReadUInt8(&inStream, &cmd);
    CLOG_C_VERBOSE(&self->log, "incoming command: %s", nimbleSerializeCmdToString(cmd))

    int result = -1;
    switch (cmd) {
        case NimbleSerializeCmdConnectResponse:
            result = nimbleClientOnConnectResponse(self, &inStream);
            break;
        case NimbleSerializeCmdServerOutBlobStream:
            result = nimbleClientOnDownloadGameStatePart(self, &inStream);
            break;
        case NimbleSerializeCmdGameStepResponse:
            result = (int) nimbleClientOnGameStepResponse(self, &inStream);
            break;
        case NimbleSerializeCmdJoinGameResponse:
            result = nimbleClientOnJoinGameResponse(self, &inStream);
            break;
        case NimbleSerializeCmdGameStateResponse:
            result = nimbleClientOnDownloadGameStateResponse(self, &inStream);
            break;
        case NimbleSerializeCmdJoinGameOutOfParticipantSlotsResponse:
            result = nimbleClientOnJoinGameParticipantOutOfSpaceResponse(self, &inStream);
            break;
        default:
            CLOG_C_SOFT_ERROR(&self->log, "unknown message %02X", cmd)
            return -1;
    }

    if (result >= 0) {
        if (inStream.pos != inStream.size) {
            CLOG_C_ERROR(&self->log, "did not read all of it")
        }
    }

    return result;
}

/* TODO:
  blobStreamInInit(&self->blobStreamIn, self->memory, self->blobStreamAllocator, octetCount, BLOB_STREAM_CHUNK_SIZE,
self->log);
blobStreamLogicInInit(&self->blobStreamInLogic, &self->blobStreamIn);

 */
