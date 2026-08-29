// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RawData.h"
#include <minerror.h>

class BufferMemoryProxy : public IRawData
{
public:
    _Check_return_ HRESULT SetBuffer(_In_ Microsoft::WRL::ComPtr<wsts::IBuffer> buffer)
    {
        m_buffer = buffer;

        IFC_RETURN(buffer->get_Length(&m_length));

        IFC_RETURN(m_buffer.As(&m_bufferByteAccess));
        IFC_RETURN(m_bufferByteAccess->Buffer(&m_data));

        return S_OK;
    }

    // IRawData
    const uint8_t* GetData() const override { return m_data; }
    size_t GetSize() const override { return m_length; }

private:
    Microsoft::WRL::ComPtr<wsts::IBuffer> m_buffer;
    Microsoft::WRL::ComPtr<::Windows::Storage::Streams::IBufferByteAccess> m_bufferByteAccess;

    uint8_t* m_data = nullptr;
    uint32_t m_length = 0;
};
