// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <PalResourceManager.h>
#include <xref_ptr.h>

// Keeps resource-owned bytes alive without copying or freeing them.
// The owner must support reference counting and destruction on a consumer's thread.
class CResourceMemory final : public IPALMemory, public CInterlockedReferenceCount
{
public:
    CResourceMemory(
        _In_reads_(size) XUINT8* buffer,
        XUINT32 size,
        _In_ IPALResource* owner)
        : m_owner(owner)
        , m_buffer(buffer)
        , m_size(size)
    {
        ASSERT(owner);
    }

    XUINT32 AddRef() const override { return CInterlockedReferenceCount::AddRef(); }
    XUINT32 Release() const override { return CInterlockedReferenceCount::Release(); }
    void* GetAddress() const override { return m_buffer; }
    XUINT32 GetSize() const override { return m_size; }

private:
    ~CResourceMemory() override = default;

    xref_ptr<IPALResource> m_owner;
    _Field_size_(m_size) XUINT8* m_buffer;
    XUINT32 m_size;
};
