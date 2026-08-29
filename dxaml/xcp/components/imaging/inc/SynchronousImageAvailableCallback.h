// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImageProviderInterfaces.h>

// Helper class to temporarily store a reference to the software bitmap when using synchronous decoding.
class SynchronousImageAvailableCallback : public CXcpObjectBase< IImageAvailableCallback >
{
public:
    _Check_return_ HRESULT OnImageAvailable(_In_ IImageAvailableResponse* response) override
    {
        m_response = response;
        return S_OK;
    }

    IImageAvailableResponse* GetResponse() const
    {
        return m_response.get();
    }

    void Reset()
    {
        m_response = nullptr;
    }

private:
    xref_ptr<IImageAvailableResponse> m_response;
};