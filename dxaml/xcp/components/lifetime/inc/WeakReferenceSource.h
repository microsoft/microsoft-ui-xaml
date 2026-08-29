// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WeakReferenceSourceNoThreadId.h"

namespace ctl
{
    /////////////////////////////////////
    //
    // WeakReferenceSource
    //
    // Adds support for:
    // * Thread Affinity and Checks
    // * Automatic ReferenceTracking through Tracking Table
    // * Queuing of objects to UI thread for destruction.
    //
    // Usually non DO classes which are world visible or exposed
    // to lifetime management rules in CLR should derive from
    // WeakReferenceSource.
    //
    /////////////////////////////////////

    class __declspec(uuid("23bfdbb1-da88-4df0-b579-fa147b9ba553")) WeakReferenceSource
        : public WeakReferenceSourceNoThreadId
    {

    protected:
        WeakReferenceSource();

        void OnFinalRelease() final;

        _Check_return_ HRESULT Initialize() override;
        _Check_return_ HRESULT CheckThread() const final;
        DWORD GetThreadID() const { return m_uThreadId; }
        DirectUI::IDXamlCore* GetCoreForObject() final;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        UINT32 m_uThreadId;
    };
}

