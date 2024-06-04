// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkView.g.h"
#include "FrameworkApplication.g.h"
#include "LaunchActivatedEventArgs.g.h"

#include <MetadataAPI.h>
#include <DependencyLocator.h>
#include <MetadataResetter.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace Microsoft::WRL;

FrameworkView::FrameworkView() :
    m_pCoreApplicationView(NULL)
{
    m_ActivatedEventToken.value = 0;
}

FrameworkView::~FrameworkView()
{
}

IFACEMETHODIMP FrameworkView::Initialize(_In_ wac::ICoreApplicationView* pCoreApplicationView)
{
    HRESULT hr = S_OK;
    BOOLEAN fIsMainView = FALSE;

    IFC(pCoreApplicationView->get_IsMain(&fIsMainView));

    // init Jupiter for this thread, before we init the main ASTA, so a DXamlCore exists when the app object constructor runs
    // note:  Application.Resources will be empty until the app.xaml is loaded, which happens after the constructor runs
    IFC(DXamlCore::Initialize((fIsMainView ? InitializationType::MainView : InitializationType::Normal)));

    // if this is the main STA view, invoke the callback to create the app object
    if (fIsMainView)
    {
        IFC(FrameworkApplication::MainASTAInitialize());
    }

    IFC(DXamlCore::GetCurrent()->EnsureCoreApplicationInitialized());

    ReplaceInterface(m_pCoreApplicationView, pCoreApplicationView);
    IFC(HookCoreActivationEvents(TRUE /* fRegister */));

    // Get reference to metadata from app object after it has been initialized.
    m_metadataRef = FrameworkApplication::GetCurrentNoRef()->GetMetadataReference();

Cleanup:
    if (FAILED(hr) && fIsMainView)
    {
        // Failing the main view Initialize is fatal. The process is about to be torn down.
        // CoreApplication will Uninitialize the main view and attempt classic shutdown of the process.
        // Often the process will be terminated by PLM timeout before classic shutdown completes.
        //
        // By failing-fast here, we'll create a crashdump and Watson report that points to the real failure.
        //
        // Otherwise, there would either be no crashdump (if classic shutdown finishes) or a PLM crashdump
        // that doesn't contain any useful information about the cause of the failure.

        // Note that it's critical that if the error came from app code, we don't translate the hr as the
        // error propagates. The hr we pass here must match the thread's error info object hr if the error
        // came from app code.
        FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(hr);
    }

    RRETURN(hr);
}

IFACEMETHODIMP FrameworkView::SetWindow(_In_ wuc::ICoreWindow* pWindow)
{
    IFC_RETURN(DXamlCore::GetCurrent()->ConfigureJupiterWindow(pWindow));
    IFC_RETURN(FrameworkApplication::GetCurrentNoRef()->InvokeOnWindowCreated());
    return S_OK;
}

IFACEMETHODIMP FrameworkView::Load(_In_ HSTRING contentId)
{
    RRETURN(S_OK);
}

IFACEMETHODIMP FrameworkView::Run()
{
    RRETURN(DXamlCore::GetCurrent()->RunCoreWindowMessageLoop());
}

IFACEMETHODIMP FrameworkView::Uninitialize()
{
    HRESULT hr = S_OK;

    // if this is the main STA view, release the app object
    if (m_pCoreApplicationView)
    {
        BOOLEAN fIsMainView = FALSE;
        IFC(m_pCoreApplicationView->get_IsMain(&fIsMainView));
        if (fIsMainView)
        {
            FrameworkApplication::ReleaseCurrent();
        }
    }

    // Deinitialize DXamlCore if needed. This may have already happened
    // at this point (if the object count fell to 0 that would have
    // triggered deinit), in which case this call is a no-op.
    IFC(DXamlCore::Deinitialize());

    if (m_pCoreApplicationView)
    {
        IFC(HookCoreActivationEvents(FALSE /* fRegister */));
    }

    DependencyLocator::UninitializeThread();

Cleanup:
    // We need to release the ref to the CoreApplicationView here to break a ref cycle.
    // (CoreApplicationView references the FrameworkView, and the FrameworkView references the CoreApplicationView).
    ReleaseInterface(m_pCoreApplicationView);

    // Clear the reference to the metadata store from this thread. If this is the last reference, this will
    // reset the metadata store.
    m_metadataRef = nullptr;

    VERIFYHR(hr);
    // should we return hr as result of Uninitialize?
    RRETURN(S_OK);
}

_Check_return_ HRESULT FrameworkView::OnActivated(
    _In_ wac::ICoreApplicationView* pSender,
    _In_ waa::IActivatedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    TraceOnActivatedBegin();
    waa::ActivationKind activationKind;

    waa::ILaunchActivatedEventArgs*      pLaunchActivatedEventArgs = NULL;
    waa::ISearchActivatedEventArgs*      pSearchActivatedEventArgs = NULL;
    waa::IShareTargetActivatedEventArgs* pShareTargetActivatedEventArgs = NULL;
    waa::IFileActivatedEventArgs*        pFileActivatedEventArgs = NULL;
    waa::IFileOpenPickerActivatedEventArgs*  pFileOpenPickerActivatedEventArgs = NULL;
    waa::IFileSavePickerActivatedEventArgs*  pFileSavePickerActivatedEventArgs = NULL;
    waa::ICachedFileUpdaterActivatedEventArgs*  pCachedFileUpdaterActivatedEventArgs = NULL;

    IFC(pArgs->get_Kind(&activationKind));

    switch (activationKind)
    {
        case waa::ActivationKind_Launch:
            IFC(pArgs->QueryInterface(__uuidof(waa::ILaunchActivatedEventArgs), reinterpret_cast<void**>(&pLaunchActivatedEventArgs)));
            IFC(FrameworkApplication::GetCurrentNoRef()->InvokeOnLaunchActivated(pLaunchActivatedEventArgs));
            break;

        case waa::ActivationKind_Search:
            IFC(pArgs->QueryInterface(__uuidof(waa::ISearchActivatedEventArgs), reinterpret_cast<void**>(&pSearchActivatedEventArgs)));
            IFC(FrameworkApplication::GetCurrentNoRef()->OnSearchActivatedProtected(pSearchActivatedEventArgs));
            break;

        case waa::ActivationKind_ShareTarget:
            IFC(pArgs->QueryInterface(__uuidof(waa::IShareTargetActivatedEventArgs), reinterpret_cast<void**>(&pShareTargetActivatedEventArgs)));
            IFC(FrameworkApplication::GetCurrentNoRef()->OnShareTargetActivatedProtected(pShareTargetActivatedEventArgs));
            break;

        case waa::ActivationKind_File:
            IFC(pArgs->QueryInterface(__uuidof(waa::IFileActivatedEventArgs), reinterpret_cast<void**>(&pFileActivatedEventArgs)));
            IFC(FrameworkApplication::GetCurrentNoRef()->OnFileActivatedProtected(pFileActivatedEventArgs));
            break;

        case waa::ActivationKind_FileOpenPicker:
            IFC(pArgs->QueryInterface(__uuidof(waa::IFileOpenPickerActivatedEventArgs), reinterpret_cast<void**>(&pFileOpenPickerActivatedEventArgs)));
            IFC(FrameworkApplication::GetCurrentNoRef()->OnFileOpenPickerActivatedProtected(pFileOpenPickerActivatedEventArgs));
            break;

        case waa::ActivationKind_FileSavePicker:
            IFC(pArgs->QueryInterface(__uuidof(waa::IFileSavePickerActivatedEventArgs), reinterpret_cast<void**>(&pFileSavePickerActivatedEventArgs)));
            IFC(FrameworkApplication::GetCurrentNoRef()->OnFileSavePickerActivatedProtected(pFileSavePickerActivatedEventArgs));
            break;

        case waa::ActivationKind_CachedFileUpdater:
            IFC(pArgs->QueryInterface(__uuidof(waa::ICachedFileUpdaterActivatedEventArgs), reinterpret_cast<void**>(&pCachedFileUpdaterActivatedEventArgs)));
            IFC(FrameworkApplication::GetCurrentNoRef()->OnCachedFileUpdaterActivatedProtected(pCachedFileUpdaterActivatedEventArgs));
            break;

        case waa::ActivationKind_LockScreen:
            DXamlCore::GetCurrent()->SetTransparentBackground(true);
            IFC(FrameworkApplication::GetCurrentNoRef()->DispatchGenericActivation(pArgs));
            break;

        case waa::ActivationKind_LockScreenComponent:
            DXamlCore::GetCurrent()->SetTransparentBackground(true);
            IFC(FrameworkApplication::GetCurrentNoRef()->DispatchGenericActivation(pArgs));
            break;

        default:
            IFC(FrameworkApplication::GetCurrentNoRef()->DispatchGenericActivation(pArgs));
            break;
    }

Cleanup:
    if (FAILED(hr))
    {
        ErrorHelper::ReportUnhandledErrorFromWrappedDelegate(hr);
    }

    ReleaseInterface(pLaunchActivatedEventArgs);
    ReleaseInterface(pSearchActivatedEventArgs);
    ReleaseInterface(pShareTargetActivatedEventArgs);
    ReleaseInterface(pFileActivatedEventArgs);
    ReleaseInterface(pFileOpenPickerActivatedEventArgs);
    ReleaseInterface(pFileSavePickerActivatedEventArgs);
    ReleaseInterface(pCachedFileUpdaterActivatedEventArgs);

    TraceOnActivatedEnd();
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkView::HookCoreActivationEvents(bool fRegister)
{
    HRESULT hr = S_OK;

    if (fRegister)
    {
        IFC(m_pCoreApplicationView->add_Activated(
            Microsoft::WRL::Callback<wf::ITypedEventHandler<wac::CoreApplicationView*, waa::IActivatedEventArgs*>>(
                this, &FrameworkView::OnActivated).Get(),
            &m_ActivatedEventToken));
    }
    else if (m_ActivatedEventToken.value)
    {
        IFC(m_pCoreApplicationView->remove_Activated(m_ActivatedEventToken));
        m_ActivatedEventToken.value = 0;
    }

Cleanup:
    RRETURN(hr);
}

