// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Parser
{
    enum class XamlBufferType : uint8_t
    {
        Text,
        Binary,
        MemoryMappedResource
    };

    struct XamlBuffer
    {
        uint32_t m_count = 0;
        XamlBufferType m_bufferType = XamlBufferType::Text;
        _Field_size_bytes_(m_count) const BYTE* m_buffer = nullptr;

        bool IsBinary() const
        {
            return m_bufferType != XamlBufferType::Text;
        }
    };
}
