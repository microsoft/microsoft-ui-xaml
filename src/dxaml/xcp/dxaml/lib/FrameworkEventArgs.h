// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WeakReferenceSource.h"
#include "EventArgs.h" // uses CEventArgs
#include <xref_ptr.h>

namespace DirectUI
{
    class __declspec(uuid("2ff30017-c6eb-448e-9b80-c9042d33ea5b")) EventArgs:
        public ctl::WeakReferenceSource
    {
    public:
        EventArgs();
        ~EventArgs() override;

        _Check_return_ HRESULT Initialize(_In_ CEventArgs* pUnderlyingObject)
        {
            m_pUnderlyingObject = pUnderlyingObject;
            return S_OK;
        }

        CEventArgs* GetCorePeer();

        _Check_return_ HRESULT EndShutdown() override;

    protected:
        using ctl::WeakReferenceSource::Initialize;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        _Check_return_ HRESULT GetCorePeerNoRefWithValidation(_Outptr_ CEventArgs** ppPeer);
        virtual CEventArgs* CreateCorePeer();

    private:
        xref_ptr<CEventArgs> m_pUnderlyingObject;
    };
}
