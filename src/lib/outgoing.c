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
#include <nimble-client/prepare_header.h>
#include <nimble-serialize/debug.h>
#include <nimble-serialize/serialize.h>
#include <monotonic-time/lower_bits.h>

static int sendDownloadStateAck(NimbleClient* self, FldOutStream* stream)
{
    CLOG_C_VERBOSE(&self->log, "game state ack to server on channel %04X. Game State is downloading to the client",
                   self->joinStateChannel)

    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdDownloadGameStateStatus, &self->log);
    nimbleSerializeOutBlobStreamChannelId(stream, self->joinStateChannel);
    int errorCode = blobStreamLogicInSend(&self->blobStreamInLogic, stream);
    if (errorCode < 0) {
        return errorCode;
    }
    self->waitTime = 0;

    return 0;
}

static int sendConnectRequest(NimbleClient* self, FldOutStream* stream)
{
    NimbleSerializeConnectRequest connectRequest;
    connectRequest.applicationVersion = self->applicationVersion;
    connectRequest.useDebugStreams = self->wantsDebugStreams;

    CLOG_EXECUTE(char buf[32]; char buf2[32];)
    CLOG_C_DEBUG(&self->log,
                 "request connection for application version %s (nimble version %s). wants debug streams:%d",
                 nimbleSerializeVersionToString(&connectRequest.applicationVersion, buf, 32),
                 nimbleSerializeVersionToString(&g_nimbleProtocolVersion, buf2, 32), self->wantsDebugStreams)

    nimbleSerializeClientOutConnectRequest(stream, &connectRequest, &self->log);

    self->waitTime = 4;

    return 0;
}

static int sendStartDownloadStateRequest(NimbleClient* self, FldOutStream* stream)
{
    CLOG_C_VERBOSE(&self->log, "request downloading of state from server")

    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdDownloadGameStateRequest, &self->log);
    fldOutStreamWriteUInt8(stream, self->downloadStateClientRequestId);

    self->waitTime = 4;

    return 0;
}

static int sendJoinGameRequest(NimbleClient* self, FldOutStream* stream)
{
    CLOG_C_VERBOSE(&self->log, "--------------------- send join game request")

    nimbleSerializeClientOutJoinGameRequest(stream, &self->joinGameRequest, &self->log);
    self->waitTime = 4;

    return 0;
}

static int updateSyncedSubState(NimbleClient* self, FldOutStream* outStream)
{
    //CLOG_C_VERBOSE(&self->log, "participant phase: %d", self->joinParticipantPhase)
    switch (self->joinParticipantPhase) {
        case NimbleJoiningStateJoiningParticipant:
            return sendJoinGameRequest(self, outStream);
        case NimbleJoiningStateJoinedParticipant:
            return 0;
        case NimbleJoiningStateOutOfParticipantSlots:
            return 0;
    }
}

static TC_FORCE_INLINE int sendMessageUsingStream(NimbleClient* self, FldOutStream* outStream)
{
    switch (self->state) {
        case NimbleClientStateRequestingConnect:
            return sendConnectRequest(self, outStream);
        case NimbleClientStateConnected:
            return 0;
        case NimbleClientStateJoiningRequestingState:
            return sendStartDownloadStateRequest(self, outStream);
        case NimbleClientStateJoiningDownloadingState:
            return sendDownloadStateAck(self, outStream);
        case NimbleClientStateIdle:
            return 0;
        case NimbleClientStateDisconnected:
            return 0;
        case NimbleClientStateSynced:
            return updateSyncedSubState(self, outStream);
    }
}

static int handleState(NimbleClient* self, DatagramTransportOut* transportOut)
{
#define UDP_MAX_SIZE (1200)
    uint8_t buf[UDP_MAX_SIZE];

    switch (self->state) {
        case NimbleClientStateIdle:
            return 0;

        case NimbleClientStateConnected:
        case NimbleClientStateDisconnected:
            return 0;

        case NimbleClientStateRequestingConnect:
        case NimbleClientStateJoiningRequestingState:
        case NimbleClientStateJoiningDownloadingState:
        case NimbleClientStateSynced: {
            if (self->state == NimbleClientStateSynced) {
                int sendStepsError = nimbleClientSendStepsToServer(self, transportOut);
                if (sendStepsError < 0) {
                    return sendStepsError;
                }
            }

            FldOutStream outStream;
            fldOutStreamInit(&outStream, buf, UDP_MAX_SIZE);
            outStream.writeDebugInfo = true; //self->useDebugStreams;

            nimbleClientPrepareHeader(self, &outStream);

            int result = sendMessageUsingStream(self, &outStream);
            if (result < 0) {
                return result;
            }
            if (outStream.pos <= 4) {
                return 0;
            }
            orderedDatagramOutLogicCommit(&self->orderedDatagramOut);
            statsIntPerSecondAdd(&self->packetsPerSecondOut, 1);
            return transportOut->send(transportOut->self, outStream.octets, outStream.pos);
        }
    }
}

/// Sends message to server depending on nimble client state
/// @param self nimble protocol client
/// @param transportOut transport to send using
/// @return negative on error.
int nimbleClientOutgoing(NimbleClient* self, DatagramTransportOut* transportOut)
{
    if (self->state != NimbleClientStateSynced || true) {
        nimbleClientDebugOutput(self);
    }

    int result = handleState(self, transportOut);
    if (result < 0) {
        return result;
    }

    self->frame++;

    return 0;
}
