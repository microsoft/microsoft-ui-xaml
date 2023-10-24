// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "minpal.h"
#include "RawData.h"

// Provides a proxy for IPALMemory to non PAL types like IRawData.
// This is required until PAL types are no longer necessary.
class PALMemoryProxy : public IRawData
{
public:
    PALMemoryProxy(_In_ IPALMemory* palMemory)
        : m_palMemory(palMemory)
    {
    }

    // IRawData
    const uint8_t* GetData() const override { return reinterpret_cast<uint8_t*>(m_palMemory->GetAddress()); }
    size_t GetSize() const override { return m_palMemory->GetSize(); }

private:
    xref_ptr<IPALMemory> m_palMemory;
};
