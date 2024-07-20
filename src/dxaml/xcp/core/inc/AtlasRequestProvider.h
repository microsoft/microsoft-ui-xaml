// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;

// Simple class to abstract away call to IAtlasRequestCallback
class AtlasRequestProvider
{
public:
    explicit AtlasRequestProvider(_In_ CCoreServices* core);

    bool AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat);

private:
    CCoreServices* m_coreNoRef = nullptr;
};
