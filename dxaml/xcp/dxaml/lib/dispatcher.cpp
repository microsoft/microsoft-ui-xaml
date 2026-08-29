// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Dispatcher.h"
#include "Callback.h"

using DirectUI::ICallback;
using DirectUI::DispatcherImpl;
using DirectUI::DXamlCore;

//-----------------------------------------------------------------------------
//
// Implementation detail of the XAML dispatcher. PALCallbackAdapter bridges
// the dxaml layer and the core layer.
//
// PALCallbackAdapter implements IPALExecuteOnUIThread, allowing it to be used
// as a dispatcher callback in the core layer. It holds an instance of
// DirectUI::ICallback, the callback type used for dispatcher callbacks
// at the dxaml layer. When the core dispatcher executes the PALCallbackAdapter,
// it forwards the invocation on to the ICallback*.
//
//-----------------------------------------------------------------------------
class PALCallbackAdapter : public CXcpObjectBase<IPALExecuteOnUIThread>
{
public:
    _Check_return_ HRESULT Execute() override
    {
        return m_spCallback->Invoke();
    }

    PALCallbackAdapter(_In_ ctl::ComPtr<ICallback> spCallback) :
        m_spCallback(spCallback)
    {
    }

private:
    ctl::ComPtr<ICallback> m_spCallback;
};

//-----------------------------------------------------------------------------
//
// Implementation of IDispatcher::RunAsync - see dispatcher.h for comments.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT DispatcherImpl::RunAsync(_In_ ctl::ComPtr<ICallback> spCallback, bool fAllowReentrancy)
{
    HRESULT hr = S_OK;
    PALCallbackAdapter* pAdapter = NULL;

    IFCCHECK(m_pCore);
    IFCCHECK(m_pCore->IsInitialized());

    pAdapter = new PALCallbackAdapter(spCallback);

    IFC(m_pCore->GetHandle()->ExecuteOnUIThread(pAdapter, fAllowReentrancy ? ReentrancyBehavior::AllowReentrancy : ReentrancyBehavior::CrashOnReentrancy));

Cleanup:
    ReleaseInterface(pAdapter);

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Implementation of IDispatcher::OnDispatcherThread - see dispatcher.h for comments.
//
//-----------------------------------------------------------------------------
bool DispatcherImpl::OnDispatcherThread()
{
    return m_dwDispatcherThreadId == GetCurrentThreadId();
}

//-----------------------------------------------------------------------------
//
// Connects this dispatcher instance to a core.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT DispatcherImpl::Connect(_In_ DXamlCore* pCore)
{
    m_pCore = pCore;
    m_dwDispatcherThreadId = GetCurrentThreadId();
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Disconnects this dispatcher instance from its core - called when the core
// is being deinitialized.
//
// Clients may continue to reference the dispatcher, but any attempt to schedule
// new work items will fail.
//
//-----------------------------------------------------------------------------
void DispatcherImpl::Disconnect()
{
    m_pCore = NULL;
}

