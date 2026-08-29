// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BackgroundTaskFrameworkContext.h"
#include "Window.g.h"
#include "Grid.g.h"
#include "XamlRenderingBackgroundTask.g.h"
#include <DependencyLocator.h>
#include <DesignMode.h>
#include <windows.ui.core.corewindow-defs.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

static BackgroundTaskFrameworkContext *g_pBackgroundTaskFrameworkContext = nullptr;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the singleton instance of BackgroundTaskFrameworkContext
//      and initialized its critical section.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BackgroundTaskFrameworkContext::GlobalInit()
{
    HRESULT hr = S_OK;

    IFC(ctl::ComObject<BackgroundTaskFrameworkContext>::CreateInstance(&g_pBackgroundTaskFrameworkContext));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release the only instance of BackgroundTaskFrameworkContext.
//
//------------------------------------------------------------------------
void
BackgroundTaskFrameworkContext::GlobalDeinit()
{
    ctl::release_interface(g_pBackgroundTaskFrameworkContext);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor.
//
//------------------------------------------------------------------------
BackgroundTaskFrameworkContext::BackgroundTaskFrameworkContext()
    : m_threadInitialized(FALSE)
    , m_hrThreadInitialization(S_OK)
    , m_bStandaloneHosting(false)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor.
//
//------------------------------------------------------------------------
BackgroundTaskFrameworkContext::~BackgroundTaskFrameworkContext()
{
    VERIFYHR(PostQuitOnFrameworkThread());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This static method returns the dispatcher
//      associated with the framework thread.
//
//------------------------------------------------------------------------
/* static */_Check_return_ HRESULT
BackgroundTaskFrameworkContext::GetDispatcher(_Outptr_ wuc::ICoreDispatcher** ppValue)
{
    ASSERT(g_pBackgroundTaskFrameworkContext != NULL);
    ASSERT(g_pBackgroundTaskFrameworkContext->m_threadInitialized);
    RRETURN(g_pBackgroundTaskFrameworkContext->m_spCoreDispatcher.CopyTo(ppValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This static method returns the metadata provider for
//      xaml background tasks. This is the first instance of
//      XamlRenderingBackgroundTask to run which also implements
//      IXamlMetadataProvider.
//
//------------------------------------------------------------------------
/*static*/ xaml_markup::IXamlMetadataProvider*
BackgroundTaskFrameworkContext::GetMetadataProviderNoRef()
{
    ASSERT(g_pBackgroundTaskFrameworkContext != NULL);
    return g_pBackgroundTaskFrameworkContext->m_spMetadataProvider.Get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The static method to ensure that the framework is loaded and initialized.
//
//------------------------------------------------------------------------
/* static */_Check_return_ HRESULT
BackgroundTaskFrameworkContext::EnsureFrameworkInitialized(_In_ XamlRenderingBackgroundTask *pBackgroundTask)
{
    ASSERT(g_pBackgroundTaskFrameworkContext != NULL);
    RRETURN(g_pBackgroundTaskFrameworkContext->EnsureFrameworkInitializedImpl(pBackgroundTask));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The instance to ensure that the framework is loaded and initialized
//      for this instance..
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BackgroundTaskFrameworkContext::EnsureFrameworkInitializedImpl(_In_ XamlRenderingBackgroundTask *pBackgroundTask)
{
    HRESULT hr = S_OK;
    IFC(ReferenceTrackerManager::EnsureInitialized());

    if (m_spMetadataProvider == nullptr)
    {
        auto guard = m_Lock.lock();

        // Record the first instance of XamlRenderingBackgroundTask to
        // run and which also implements IXamlMetadataProvider
        // as the metadataprovider for process.
        if (m_spMetadataProvider == nullptr)
        {
            m_spMetadataProvider = ctl::query_interface_cast<xaml_markup::IXamlMetadataProvider>(pBackgroundTask);
        }
    }

    if (!m_threadInitialized)
    {
        // Create the framework thread under the lock.
        auto guard = m_Lock.lock();
        if (!m_threadInitialized)
        {
            IFC(CreateFrameworkThread());
        }
        ASSERT(m_threadInitialized == TRUE);
    }
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to create the framework thread
//      with appropriate thread procs.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BackgroundTaskFrameworkContext::CreateFrameworkThread()
{
    HRESULT hr = S_OK;

    IFC(m_hrThreadInitialization);

    {
        // Lambda for the  synchronous thread proceduce.
        // The create thread method doesn't return
        // until this is completely executed.
        auto syncThreadProc = [](_In_ void *pv) -> DWORD
        {
            // Take a reference to the BackgroundTaskFrameworkContext instance
            ctl::ComPtr<BackgroundTaskFrameworkContext> spContext = reinterpret_cast<BackgroundTaskFrameworkContext*>(pv);

            spContext->m_hrThreadInitialization = spContext->InitializeFrameworkThread();

            // The reference to BackgroundTaskFrameworkContext is utilized
            // by the async thread proc. Hence simply detach the comptr.
            spContext.Detach();

            return 0;
        };

        // Lambda for the asynchronous thread procedure.
        // The create thread method executes this on the new
        // thread after calling thread is unblocked.
        auto threadRunProc = [](_In_ void *pv) -> DWORD
        {
            // Attach to the reference acquired by the sync thread proc.
            ctl::ComPtr<BackgroundTaskFrameworkContext> spContext;
            spContext.Attach(reinterpret_cast<BackgroundTaskFrameworkContext*>(pv));

            if (!spContext->m_threadInitialized)
            {
                return 1;
            }

            // Release the reference before initiating the
            // core dispatcher pump.
            spContext.Reset();

            VERIFYHR(BackgroundTaskFrameworkContext::FrameworkThreadProc());
            return 0;
        };

        // Call the create thread method.
        ctl::ComPtr<BackgroundTaskFrameworkContext> spThis(this);
        if (!::SHCreateThread(threadRunProc, spThis.Get(), 0, syncThreadProc))
        {
            IFC(HResultFromKnownLastError());
        }
        else
        {
            IFC(m_hrThreadInitialization);
        }
    }
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the framework thread.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BackgroundTaskFrameworkContext::InitializeFrameworkThread()
{
    auto guard = wil::scope_exit([]()
    {
        VERIFYHR(DXamlCore::Deinitialize());
    });

    // Initialize ASTA
    IFC_RETURN(wf::Initialize(RO_INIT_SINGLETHREADED));

    // Initialize DXaml
    IFC_RETURN(DXamlCore::Initialize(InitializationType::BackgroundTask));

    ASSERT(!DesignerInterop::GetDesignerMode(DesignerMode::V2Only));

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->EnsureCoreApplicationInitialized());

    // Disable DComp atlasing when in background tasks.
    // We do this by hinting the atlas size to 0x0.
    // This will make things slow but memory is a bigger
    // concern in background tasks.
    pCore->DisableAtlas();

    // TODO: Validate BackgroundTaskFramework feature Task#29758876
    // Configure JupiterWindow without any CoreWindow
    IFC_RETURN(pCore->ConfigureJupiterWindow(nullptr /*pCoreWindow*/));

    IFC_RETURN(pCore->GetCoreDispatcher(m_spCoreDispatcher.ReleaseAndGetAddressOf()));

    // Set default content of the window
    {
        ctl::ComPtr<Grid> spGrid;
        IFC_RETURN(ctl::make(&spGrid));
        pCore->GetUwpWindowNoRef()->put_Content(spGrid.Get());
    }

    m_threadInitialized = TRUE;

    guard.release();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Thread proc to pump the dispatcher.
//
//------------------------------------------------------------------------
/*static*/_Check_return_ HRESULT
BackgroundTaskFrameworkContext::FrameworkThreadProc()
{
    HRESULT hr = S_OK;

    // pump messages
    IFC(DXamlCore::GetCurrent()->RunCoreWindowMessageLoop());

    IFC(DXamlCore::Deinitialize());

    DependencyLocator::UninitializeThread();

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to post the quit message on
//      on the dispatcher.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
BackgroundTaskFrameworkContext::PostQuitOnFrameworkThread()
{
    HRESULT hr = S_OK;
    if (m_threadInitialized)
    {
        ctl::ComPtr<wf::IAsyncAction> spAsyncAction;
        auto dispatchCallback = MakeAgileDispatcherCallback([]() -> HRESULT
        {
            ::PostQuitMessage(0);
            return S_OK;
        });
        IFC(m_spCoreDispatcher->RunAsync(
            wuc::CoreDispatcherPriority_Normal,
            dispatchCallback.Get(),
            spAsyncAction.ReleaseAndGetAddressOf()));
        m_spCoreDispatcher.Reset();
    }
Cleanup:
    RRETURN(hr);
}

void BackgroundTaskFrameworkContext::EnableStandaloneHosting()
{
    ASSERT(g_pBackgroundTaskFrameworkContext != NULL);
    g_pBackgroundTaskFrameworkContext->m_bStandaloneHosting = true;
}
