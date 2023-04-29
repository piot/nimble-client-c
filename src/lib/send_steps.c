/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <nimble-client/client.h>
#include <nimble-client/send_steps.h>
#include <nimble-serialize/debug.h>
#include <nimble-serialize/serialize.h>
#include <nimble-steps-serialize/out_serialize.h>
#include <nimble-steps-serialize/pending_out_serialize.h>

static int sendStepsToStream(NimbleClient* self, FldOutStream* stream)
{
    StepId firstStepToSend = self->nextStepIdToSendToServer;

    CLOG_C_VERBOSE(&self->log, "sending game steps id:%08X, last in buffer:%08X, buffer count:%zu", firstStepToSend,
                   self->outSteps.expectedWriteId - 1, self->outSteps.stepsCount)

#define COMMAND_DEBUG "ClientOut"
    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdGameStep, COMMAND_DEBUG);

    StepId expectedStepIdFromServer;
    uint64_t clientReceiveMask = nbsPendingStepsReceiveMask(&self->authoritativePendingStepsFromServer,
                                                            &expectedStepIdFromServer);

    CLOG_C_VERBOSE(&self->log, "client is telling the server that the client is waiting for stepId %08X",
                   expectedStepIdFromServer)

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

    CLOG_C_VERBOSE(&self->log, "outSteps: sent out steps, discard old ones before %08X", self->nextStepIdToSendToServer)

    nbsStepsDiscardUpTo(&self->outSteps, self->nextStepIdToSendToServer);

    int stepsInBuffer = (int) self->outSteps.stepsCount - NimbleSerializeMaxRedundancyCount;
    if (stepsInBuffer < 0) {
        stepsInBuffer = 0;
    }
    statsIntAdd(&self->outgoingStepsInQueue, stepsInBuffer);

    // CLOG_VERBOSE("Actually sent %d (%d to %d)", stepsActuallySent,
    // firstStepToSend, firstStepToSend+stepsActuallySent-1);
    self->waitTime = 0;

    return 0;
}

int nimbleClientSendStepsToServer(NimbleClient* self, UdpTransportOut* transportOut)
{
#define UDP_MAX_SIZE (1200)
    static uint8_t buf[UDP_MAX_SIZE];
    FldOutStream outStream;
    fldOutStreamInit(&outStream, buf, UDP_MAX_SIZE);
    orderedDatagramOutLogicPrepare(&self->orderedDatagramOut, &outStream);
    sendStepsToStream(self, &outStream);
    orderedDatagramOutLogicCommit(&self->orderedDatagramOut);
    return transportOut->send(transportOut->self, outStream.octets, outStream.pos);
}
