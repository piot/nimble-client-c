/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef NIMBLE_CLIENT_DOWNLOAD_STATE_PART_H
#define NIMBLE_CLIENT_DOWNLOAD_STATE_PART_H

struct FldInStream;
struct NimbleClient;

int nimbleClientOnDownloadGameStatePart(struct NimbleClient* self, struct FldInStream* inStream);

#endif
