// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComBase.h"

namespace ctl
{
    class ComObjectBase : public INonDelegatingInspectable
    {
    public:
        ComObjectBase(_In_ IInspectable* pOuter)
        {
            m_pControllingUnknown = pOuter;
        }

        // IInspectable (non-delegating) implementation
        IFACEMETHODIMP NonDelegatingQueryInterfaceBase(REFIID iid, void **ppValue);

        // IInspectable (delegating) implementation
        IFACEMETHODIMP QueryInterfaceBase(REFIID iid, void **ppValue);

    public:
        virtual HRESULT QueryInterfaceImplBase(_In_ REFIID iid, _Outptr_ void** ppObject) = 0;
        __declspec(noinline) static HRESULT CreateInstanceBase(_In_ ComBase* pObject, bool fNoInit = false);

    protected:
        IInspectable* m_pControllingUnknown;
    };
}