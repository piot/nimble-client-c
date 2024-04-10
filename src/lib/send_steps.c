/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/send_steps.h>
#include <nimble-serialize/serialize.h>
#include <nimble-steps-serialize/out_serialize.h>
#include <nimble-steps-serialize/pending_out_serialize.h>
#include <nimble-client/prepare_header.h>

static ssize_t sendStepsToStream(NimbleClient* self, FldOutStream* stream)
{
    CLOG_C_VERBOSE(&self->log, "sending predicted steps %08X - %08X, buffer count:%zu", self->outSteps.expectedReadId,
                   self->outSteps.expectedWriteId - 1, self->outSteps.stepsCount)

    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdGameStep, &self->log);

    StepId expectedStepIdFromServer;
    uint64_t clientReceiveMask = nbsPendingStepsReceiveMask(&self->authoritativePendingStepsFromServer,
                                                            &expectedStepIdFromServer);

    CLOG_C_VERBOSE(&self->log, "telling the server that we are waiting for authoritative step %08X",
                   expectedStepIdFromServer)

    int serializeOutErr = nbsPendingStepsSerializeOutHeader(stream, expectedStepIdFromServer, clientReceiveMask);
    if (serializeOutErr < 0) {
        return serializeOutErr;
    }

    ssize_t stepsActuallySent = nbsStepsOutSerialize(stream, &self->outSteps);
    if (stepsActuallySent < 0) {
        CLOG_SOFT_ERROR("problem with steps out serialize")
        return stepsActuallySent;
    }

    //    CLOG_C_VERBOSE(&self->log, "outSteps: sent out steps, discard old ones before %08X", self->nextStepIdToSendToServer)

    int stepsInBuffer = (int) self->outSteps.stepsCount - (int) NimbleSerializeMaxRedundancyCount;
    if (stepsInBuffer < 0) {
        stepsInBuffer = 0;
    }
    statsIntAdd(&self->outgoingStepsInQueue, stepsInBuffer);

    // CLOG_VERBOSE("Actually sent %d (%d to %d)", stepsActuallySent,
    // firstStepToSend, firstStepToSend+stepsActuallySent-1);
    self->waitTime = 0;

    return stepsActuallySent;
}

/// Sends predicted steps to the server using the unreliable datagram transport
/// @param self nimble protocol clinet
/// @param transportOut transport to send on
/// @return negative on error
int nimbleClientSendStepsToServer(NimbleClient* self, DatagramTransportOut* transportOut)
{
#define UDP_MAX_SIZE (1200)
    uint8_t buf[UDP_MAX_SIZE];
    FldOutStream outStream;
    fldOutStreamInit(&outStream, buf, UDP_MAX_SIZE);
    outStream.writeDebugInfo = true;

    FldOutStreamStoredPosition restorePosition;
    nimbleClientPrepareHeader(self, &outStream, &restorePosition);

    ssize_t stepsSent = sendStepsToStream(self, &outStream);
    if (stepsSent < 0) {
        return (int)stepsSent;
    }

    int status = nimbleClientCommitHeader(self, &outStream, restorePosition);
    if (status < 0) {
        return status;
    }

    orderedDatagramOutLogicCommit(&self->orderedDatagramOut);
    CLOG_C_VERBOSE(&self->log, "send steps to server octetCount: %zu", outStream.pos)
    statsIntPerSecondAdd(&self->sentStepsDatagramCountPerSecond, 1);
    statsIntPerSecondAdd(&self->packetsPerSecondOut, 1);
    return transportOut->send(transportOut->self, outStream.octets, outStream.pos);
}
