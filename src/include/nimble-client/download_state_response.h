/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_DOWNLOAD_STATE_RESPONSE_H
#define NIMBLE_CLIENT_DOWNLOAD_STATE_RESPONSE_H

struct NimbleClient;
struct FldInStream;

int nimbleClientOnDownloadGameStateResponse(struct NimbleClient* self, struct FldInStream* inStream);

#endif

