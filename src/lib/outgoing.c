/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/debug.h>
#include <nimble-client/outgoing.h>
#include <nimble-client/send_steps.h>
#include <nimble-serialize/debug.h>
#include <nimble-serialize/serialize.h>
#include <ordered-datagram/out_logic.h>

#define DEBUG_PREFIX "Outgoing"

static int sendDownloadStateAck(NimbleClient* self, FldOutStream* stream)
{
    CLOG_INFO("join or rejoin game state being downloaded to client. Send ack to channel ")

    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdDownloadGameStateStatus, DEBUG_PREFIX);
    nimbleSerializeOutBlobStreamChannelId(stream, self->joinStateChannel);
    int errorCode = blobStreamLogicInSend(&self->blobStreamInLogic, stream);
    if (errorCode < 0) {
        return errorCode;
    }
    self->waitTime = 0;

    return 0;
}

static int sendStartDownloadStateRequest(NimbleClient* self, FldOutStream* stream)
{
    CLOG_INFO("request downloading of state from server")

    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdDownloadGameStateRequest, DEBUG_PREFIX);
    self->waitTime = 0;

    return 0;
}

static int sendJoinGameRequest(NimbleClient* self, FldOutStream* stream)
{
    CLOG_INFO("send join game request");
    nimbleSerializeClientOutGameJoin(stream, &self->joinGameOptions);
    self->waitTime = 120;

    return 0;
}

static TC_FORCE_INLINE int sendMessageUsingStream(NimbleClient* self, FldOutStream* outStream)
{
    switch (self->state) {
        case NimbleClientStateJoiningRequestingState:
            return sendStartDownloadStateRequest(self, outStream);
        case NimbleClientStateJoiningDownloadingState:
            return sendDownloadStateAck(self, outStream);
        case NimbleClientStateJoiningGame:
            return sendJoinGameRequest(self, outStream);
        case NimbleClientStatePlaying:
        case NimbleClientStateIdle:
        case NimbleClientStateJoinedGame:
            return 0;
        default:
            CLOG_ERROR("Unknown state %d", self->state)
    }
}

static TC_FORCE_INLINE int handleState(NimbleClient* self, UdpTransportOut* transportOut)
{
#define UDP_MAX_SIZE (1200)
    static uint8_t buf[UDP_MAX_SIZE];

    switch (self->state) {
        case NimbleClientStateIdle:
            return 0;

        case NimbleClientStatePlaying:
            return nimbleClientSendStepsToServer(self, transportOut);

        default: {
            FldOutStream outStream;
            fldOutStreamInit(&outStream, buf, UDP_MAX_SIZE);
            orderedDatagramOutLogicPrepare(&self->orderedDatagramOut, &outStream);
            int result = sendMessageUsingStream(self, &outStream);
            if (result < 0) {
                return result;
            }
            if (outStream.pos <= 2) {
                return 0;
            }
            orderedDatagramOutLogicCommit(&self->orderedDatagramOut);
            return transportOut->send(transportOut->self, outStream.octets, outStream.pos);
        }
    }
}

/// Sends message to server depending on nimble client state
/// @param self
/// @param now
/// @param transportOut
/// @return negative on error.
int nimbleClientOutgoing(NimbleClient* self, UdpTransportOut* transportOut)
{
    if (self->state != NimbleClientStatePlaying) {
        nimbleClientDebugOutput(self);
    }

    int result = handleState(self, transportOut);
    if (result < 0) {
        return result;
    }

    self->frame++;

    return 0;
}
