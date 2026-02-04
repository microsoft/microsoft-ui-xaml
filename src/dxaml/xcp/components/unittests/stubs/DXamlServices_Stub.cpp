// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DXamlServices.h>


namespace DirectUI
{
    namespace DXamlServices
    {
        __maybenull IPeerTableHost* GetPeerTableHost()
        {
            return nullptr;
        }

        bool ShouldStoreSourceInformation()
        {
            return false;
        }

        _Check_return_ HRESULT GetPeer(_In_ CDependencyObject*, _Outptr_ DependencyObject**)
        {
            return E_NOTIMPL;
        }

        wuc::ICoreWindow* GetCurrentCoreWindowNoRef()
        {
            return nullptr;
        }

        CCoreServices* GetHandle()
        {
            return nullptr;
        }

        _Check_return_ HRESULT TryGetPeer(_In_ CDependencyObject* pDO, _In_ REFIID iid, _Outptr_result_maybenull_ void** ppObject)
        {
            return E_NOTIMPL;
        }

        bool IsInBackgroundTask()
        {
            return false;
        }

        __maybenull IDXamlCore* GetDXamlCore()
        {
            return nullptr;
        }

        bool IsDXamlCoreShutdown()
        {
            return false;
        }
    }
}
