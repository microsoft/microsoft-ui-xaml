// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlDiagnostics.h"
#include "RuntimeObject.h"
#include "RuntimeElement.h"


IFACEMETHODIMP XamlDiagnostics::UnregisterInstance(InstanceHandle handle)
{
    std::shared_ptr<Diagnostics::RuntimeObject> instance;
    if (TryFindObjectFromHandle(handle, instance))
    {
        instance->Close();
    }

    return S_OK;
}

IFACEMETHODIMP XamlDiagnostics::TryGetDispatcherQueueForObject(_In_ InstanceHandle handle, _Outptr_ IInspectable** returnValue)
{
    *returnValue = nullptr;
    std::shared_ptr<Diagnostics::RuntimeObject> obj;
    if (TryFindObjectFromHandle(handle, obj))
    {
        if (auto queue = m_spDiagInterop->GetDispatcherQueueForThreadId(obj->GetAssociatedThreadId()))
        {
           *returnValue = queue.Detach();
        }
    }
    return S_OK;
}


