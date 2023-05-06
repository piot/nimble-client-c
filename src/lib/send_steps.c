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
#include <monotonic-time/lower_bits.h>

static int sendStepsToStream(NimbleClient* self, FldOutStream* stream)
{
    CLOG_C_VERBOSE(&self->log, "sending game steps id:%08X, last in buffer:%08X, buffer count:%zu", self->outSteps.expectedReadId,
                   self->outSteps.expectedWriteId - 1, self->outSteps.stepsCount)

#define COMMAND_DEBUG "ClientOut"
    nimbleSerializeWriteCommand(stream, NimbleSerializeCmdGameStep, COMMAND_DEBUG);

    StepId expectedStepIdFromServer;
    uint64_t clientReceiveMask = nbsPendingStepsReceiveMask(&self->authoritativePendingStepsFromServer,
                                                            &expectedStepIdFromServer);

    CLOG_C_VERBOSE(&self->log, "client is telling the server that the client is waiting for stepId %08X",
                   expectedStepIdFromServer)

    MonotonicTimeLowerBitsMs lowerBitsMs = monotonicTimeMsToLowerBits(monotonicTimeMsNow());

    int serializeOutErr = nbsPendingStepsSerializeOutHeader(stream, expectedStepIdFromServer, clientReceiveMask, lowerBitsMs);
    if (serializeOutErr < 0) {
        return serializeOutErr;
    }

    int stepsActuallySent = nbsStepsOutSerialize(stream, &self->outSteps);
    if (stepsActuallySent < 0) {
        CLOG_SOFT_ERROR("problem with steps out serialize")
        return stepsActuallySent;
    }

    //    CLOG_C_VERBOSE(&self->log, "outSteps: sent out steps, discard old ones before %08X", self->nextStepIdToSendToServer)


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

/// Sends predicted steps to the server using the unreliable datagram transport
/// @param self
/// @param transportOut
/// @return negative on error
int nimbleClientSendStepsToServer(NimbleClient* self, UdpTransportOut* transportOut)
{
#define UDP_MAX_SIZE (1200)
    uint8_t buf[UDP_MAX_SIZE];
    FldOutStream outStream;
    fldOutStreamInit(&outStream, buf, UDP_MAX_SIZE);
    orderedDatagramOutLogicPrepare(&self->orderedDatagramOut, &outStream);
    sendStepsToStream(self, &outStream);
    orderedDatagramOutLogicCommit(&self->orderedDatagramOut);
    CLOG_C_VERBOSE(&self->log, "send steps to server %d", outStream.pos)
    statsIntPerSecondAdd(&self->sentStepsDatagramCountPerSecond, 1);
    statsIntPerSecondAdd(&self->packetsPerSecondOut, 1);
    return transportOut->send(transportOut->self, outStream.octets, outStream.pos);
}
