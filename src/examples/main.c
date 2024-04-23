/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/nimble-client-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <clog/console.h>
#include <flood/out_stream.h>
#include <imprint/default_setup.h>
#include <nimble-client/client.h>
#include <nimble-client/network_realizer.h>
#include <nimble-serialize/debug.h>
#include <nimble-steps-serialize/in_serialize.h>
#include <nimble-steps-serialize/out_serialize.h>
#include <stdio.h>
#include <udp-client/udp_client.h>
#include <unistd.h>

clog_config g_clog;

static ssize_t clientReceive(void* _self, uint8_t* data, size_t size)
{
    UdpClientSocket* self = _self;

    return udpClientReceive(self, data, size);
}

static int clientSend(void* _self, const uint8_t* data, size_t size)
{
    UdpClientSocket* self = _self;

    return udpClientSend(self, data, size);
}

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    g_clog.log = clog_console;
    CLOG_VERBOSE("example start")
    CLOG_VERBOSE("initialized")

    NimbleSerializeVersion exampleApplicationVersion = {0x10, 0x20, 0x30};
    FldOutStream outStream;

    uint8_t buf[1024];
    fldOutStreamInit(&outStream, buf, 1024);

    NimbleClientRealize clientRealize;
    NimbleClientRealizeSettings settings;

    ImprintDefaultSetup memory;

    DatagramTransport transport;

    imprintDefaultSetupInit(&memory, 16 * 1024 * 1024);

    int startupErr = udpClientStartup();
    if (startupErr < 0) {
        return startupErr;
    }

    UdpClientSocket udpClientSocket;
    udpClientInit(&udpClientSocket, "127.0.0.1", 27000);

    transport.self = &udpClientSocket;
    transport.receive = clientReceive;
    transport.send = clientSend;

    settings.blobMemory = &memory.slabAllocator.info;
    settings.memory = &memory.tagAllocator.info;
    settings.transport = transport;
    settings.applicationVersion = exampleApplicationVersion;

    nimbleClientRealizeInit(&clientRealize, &settings);
    nimbleClientRealizeReInit(&clientRealize, &settings);

    NimbleSerializeJoinGameRequest joinGameOptions;
    joinGameOptions.playerCount = 1;
    joinGameOptions.players[0].localIndex = 0xca;

    nimbleClientRealizeJoinGame(&clientRealize, joinGameOptions);

    uint32_t frame = 0;
    NimbleClientRealizeState reportedState;
    reportedState = NimbleClientRealizeStateInit;
    while (1) {
        usleep(16 * 1000);

        frame++;

        if ((frame % 2) == 0 && reportedState == NimbleClientRealizeStateSynced &&
            nbsStepsAllowedToAdd(&clientRealize.client.outSteps)) {
            CLOG_VERBOSE("adding out step %016X", clientRealize.client.outSteps.expectedWriteId)
            NimbleStepsOutSerializeLocalParticipants data;
            char stepString[32];
            snprintf(stepString, 32, "OneStep %016X", clientRealize.client.outSteps.expectedWriteId);

            uint8_t participantId = clientRealize.client.localParticipantLookup[0].participantId;
            if (participantId == 0) {
                CLOG_ERROR("illegal participant id")
            }
            data.participants[0].participantId = participantId;
            data.participants[0].payload = (const uint8_t*) stepString;
            data.participants[0].payloadCount = strlen(stepString) + 1;
            //data.participants[0].connectState = 0;
            data.participantCount = 1;
            uint8_t stepBuf[64];

            ssize_t octetLength = nbsStepsOutSerializeCombinedStep(&data, stepBuf, 64);
            if (octetLength < 0) {
                return (int)octetLength;
            }

            int errorCode = nbsStepsWrite(&clientRealize.client.outSteps, clientRealize.client.outSteps.expectedWriteId,
                                          stepBuf, (size_t)octetLength);
            if (errorCode < 0) {
                return errorCode;
            }
        }

        MonotonicTimeMs now = monotonicTimeMsNow();
        nimbleClientRealizeUpdate(&clientRealize, now);

        uint8_t readPayload[512];

        StepId readStepId;
        int payloadOctetCount = nimbleClientReadStep(&clientRealize.client, readPayload, 512, &readStepId);
        if (payloadOctetCount > 0) {
            struct NimbleStepsOutSerializeLocalParticipants participants;

            nbsStepsInSerializeStepsForParticipantsFromOctets(&participants, readPayload, (size_t) payloadOctetCount);
            CLOG_DEBUG("read step %016X  octetCount: %d", readStepId, payloadOctetCount)
            for (size_t i = 0; i < participants.participantCount; ++i) {
                CLOG_EXECUTE(NimbleStepsOutSerializeLocalParticipant* participant = &participants.participants[i];)
                CLOG_DEBUG(" participant %d '%s' octetCount: %zu", participant->participantId, participant->payload,
                           participant->payloadCount)
            }
        }
    }

    // imprintDefaultSetupDestroy(&memory);
}
