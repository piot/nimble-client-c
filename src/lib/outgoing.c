/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/outgoing.h>
#include <nimble-serialize/serialize.h>
#include <nimble-steps-serialize/out_serialize.h>
#include <nimble-steps-serialize/pending_out_serialize.h>
#include <nimble-serialize/debug.h>
#include <nimble-client/debug.h>

static int updateGameOutSteps(NimbleClient* self, FldOutStream* stream)
{
    StepId firstStepToSend = self->nextStepIdToSendToServer;

    CLOG_C_VERBOSE(&self->clog, "sending game steps id:%08X, last in buffer:%08X, buffer count:%zu", firstStepToSend, self->outSteps.expectedWriteId-1, self->outSteps.stepsCount)

    #define COMMAND_DEBUG "ClientOut"
    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdGameStep,COMMAND_DEBUG);

    StepId expectedStepIdFromServer;
    uint64_t clientReceiveMask = nbsPendingStepsReceiveMask(&self->authoritativePendingStepsFromServer,
                                                            &expectedStepIdFromServer);

    CLOG_C_VERBOSE(&self->clog, "client is telling the server that the client is waiting for stepId %08X", expectedStepIdFromServer)

    nbsPendingStepsSerializeOutHeader(stream, expectedStepIdFromServer, clientReceiveMask);

    int stepsActuallySent = nbsStepsOutSerialize(stream, firstStepToSend, &self->outSteps);
    if (stepsActuallySent < 0) {
        CLOG_SOFT_ERROR("problem with steps out serialize")
        return stepsActuallySent;
    }
    int errorCode = nbsStepsOutSerializeAdvanceIfNeeded(&self->nextStepIdToSendToServer, &self->outSteps);
    if (errorCode < 0) {
        return errorCode;
    }

    CLOG_VERBOSE("outSteps: sent out steps, discard old ones before %08X", self->nextStepIdToSendToServer)

    nbsStepsDiscardUpTo(&self->outSteps, self->nextStepIdToSendToServer);

    int stepsInBuffer = (int)self->outSteps.stepsCount - NimbleSerializeMaxRedundancyCount;
    if (stepsInBuffer < 0) {
        stepsInBuffer = 0;
    }
    statsIntAdd(&self->outgoingStepsInQueue, stepsInBuffer);

    // CLOG_VERBOSE("Actually sent %d (%d to %d)", stepsActuallySent,
    // firstStepToSend, firstStepToSend+stepsActuallySent-1);
    self->waitTime = 0;

    return 0;
}


#define DEBUG_PREFIX "Outgoing"

static int updateGamePlaying(NimbleClient* self, MonotonicTimeMs now, UdpTransportOut* transportOut)
{
#define UDP_MAX_SIZE (1200)
    static uint8_t buf[UDP_MAX_SIZE];
    FldOutStream outStream;
    fldOutStreamInit(&outStream, buf, UDP_MAX_SIZE);

    updateGameOutSteps(self, &outStream);
    return transportOut->send(transportOut->self, outStream.octets, outStream.pos);
}


static int updateJoinGameDownloadingState(NimbleClient* self, FldOutStream* stream)
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

static int updateJoinStartDownloadState(NimbleClient* self, FldOutStream* stream)
{
    CLOG_INFO("request downloading of state from server")

    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdDownloadGameStateRequest, DEBUG_PREFIX);
    self->waitTime = 0;

    return 0;
}

static int updateJoinGame(NimbleClient* self, FldOutStream* stream)
{
    CLOG_INFO("send join game request");
    nimbleSerializeClientOutGameJoin(stream, &self->joinGameOptions);
    self->waitTime = 120;

    return 0;
}


static TC_FORCE_INLINE int handleStreamState(NimbleClient* self, FldOutStream* outStream)
{
    switch (self->state) {
        case NimbleClientStateJoiningRequestingState:
            return updateJoinStartDownloadState(self, outStream);
        case NimbleClientStateJoiningDownloadingState:
            return updateJoinGameDownloadingState(self, outStream);
        case NimbleClientStateJoiningGame:
            return updateJoinGame(self, outStream);
        case NimbleClientStatePlaying:
        case NimbleClientStateIdle:
        case NimbleClientStateJoinedGame:
            return 0;
        default:
            CLOG_ERROR("Unknown state %d", self->state)
    }
}

static TC_FORCE_INLINE int handleState(NimbleClient* self, MonotonicTimeMs now, UdpTransportOut* transportOut)
{
#define UDP_MAX_SIZE (1200)
    static uint8_t buf[UDP_MAX_SIZE];

    switch (self->state) {
        case NimbleClientStateIdle:
            return 0;

        case NimbleClientStatePlaying:
            return updateGamePlaying(self, now, transportOut);

        default: {
            FldOutStream outStream;
            fldOutStreamInit(&outStream, buf, UDP_MAX_SIZE);
            int result = handleStreamState(self, &outStream);
            if (result < 0) {
                return result;
            }
            if (outStream.pos == 0) {
                return 0;
            }
            return transportOut->send(transportOut->self, outStream.octets, outStream.pos);
        }
    }
}

/// Sends message to server depending on nimble client state
/// @param self
/// @param now
/// @param transportOut
/// @return negative on error.
int nimbleClientOutgoing(NimbleClient* self, MonotonicTimeMs now, UdpTransportOut* transportOut)
{
    if (self->state != NimbleClientStatePlaying)
    {
        nimbleClientDebugOutput(self);
    }

    int result = handleState(self, now, transportOut);
    if (result < 0) {
        return result;
    }

    self->frame++;

    return 0;
}

