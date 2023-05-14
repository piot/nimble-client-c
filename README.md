# Nimble Client

Client for the [Nimble Protocol](https://github.com/piot/nimble-serialize-c/blob/main/docs/index.adoc). Implemented in C99.

It supports the following:

* Receive an application state from a Nimble Server.
* Send predicted Steps (User Input) to a Nimble server and receive authoritative Steps back. It is allowed to send an empty array of Steps if spectating.
* Join a game with participants and receive unique participant IDs back. This allow the client to insert predicted steps to the Nimble server.
* Leave a game with specified participants.

The Nimble Client does not do any simulation, rollback, prediction or similar. Check out [Nimble Engine Client](https://github.com/piot/nimble-engine-client) for that functionality.

## Usage

### Connecting

```c
typedef struct NimbleClientRealizeSettings {
    DatagramTransport transport;
    struct ImprintAllocator* memory;
    struct ImprintAllocatorWithFree* blobMemory;
    size_t maximumSingleParticipantStepOctetCount;
    size_t maximumNumberOfParticipants;
    Clog log;
} NimbleClientRealizeSettings;

void nimbleClientRealizeInit(NimbleClientRealize* self, const NimbleClientRealizeSettings* settings);
```

### Join Game

```c
typedef struct NimbleSerializePlayerJoinOptions {
    uint8_t localIndex;
} NimbleSerializePlayerJoinOptions;

typedef struct NimbleSerializeGameJoinOptions {
    NimbleSerializeVersion applicationVersion;
    NimbleSerializePlayerJoinOptions players[8];
    size_t playerCount;
} NimbleSerializeGameJoinOptions;

void nimbleClientRealizeJoinGame(NimbleClientRealize* self, NimbleSerializeGameJoinOptions options);
```
