// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AtlasRequestProvider.h"

AtlasRequestProvider::AtlasRequestProvider(_In_ CCoreServices* core)
: m_coreNoRef(core)
{
}

bool AtlasRequestProvider::AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat)
{
    return m_coreNoRef->AtlasRequest(width, height, pixelFormat);
}
