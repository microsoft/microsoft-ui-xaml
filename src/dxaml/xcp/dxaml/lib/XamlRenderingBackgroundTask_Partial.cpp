// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlRenderingBackgroundTask.g.h"
#include "BackgroundTaskFrameworkContext.h"
#include <windows.ui.core.corewindow-defs.h>
#include "Window.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Ctor.
//
//-------------------------------------------------------------------------
XamlRenderingBackgroundTask::XamlRenderingBackgroundTask()
{
    VERIFYHR(ReferenceTrackerManager::EnsureInitialized());
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Dtor.
//
//-------------------------------------------------------------------------
XamlRenderingBackgroundTask::~XamlRenderingBackgroundTask()
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Override QueryInterface logic to work for
//      IBackgroundTask.
//
//-------------------------------------------------------------------------
HRESULT XamlRenderingBackgroundTask::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(wab::IBackgroundTask)))
    {
        *ppObject = static_cast<wab::IBackgroundTask *>(this);
    }
    else
    {
        RRETURN(DirectUI::XamlRenderingBackgroundTaskGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Implementation of IBackgroundTask::Run method.
//      Initializes the framework and queues the app
//      implementation of OnRun to its dispatcher.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
XamlRenderingBackgroundTask::Run(
    _In_ wab::IBackgroundTaskInstance* pTaskInstance)
{
    IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_XAMLRENDERINGBACKGROUNDTASK_NOT_AVAILABLE_IN_WINUI3));

    ctl::ComPtr<wuc::ICoreDispatcher> spCoreDispatcher;
    IFC_RETURN(BackgroundTaskFrameworkContext::EnsureFrameworkInitialized(this));
    IFC_RETURN(BackgroundTaskFrameworkContext::GetDispatcher(spCoreDispatcher.ReleaseAndGetAddressOf()));

    {
        ctl::ComPtr<wf::IAsyncAction> spAsyncAction;
        ctl::ComPtr<wab::IBackgroundTaskDeferral> spDeferral;
        IFC_RETURN(pTaskInstance->GetDeferral(spDeferral.ReleaseAndGetAddressOf()));

        ctl::ComPtr<wab::IBackgroundTaskInstance> spTaskInstance(pTaskInstance);
        ctl::ComPtr<XamlRenderingBackgroundTask> strongThis(this);

        auto dispatchCallback = MakeAgileDispatcherCallback([strongThis, spTaskInstance, spDeferral]() -> HRESULT
        {
            IFC_RETURN(strongThis->OnRunProtected(spTaskInstance.Get()));
            IFC_RETURN(spDeferral->Complete());

            return S_OK;
        });

        IFC_RETURN(spCoreDispatcher->RunAsync(
                        wuc::CoreDispatcherPriority_Normal,
                        dispatchCallback.Get(),
                        spAsyncAction.ReleaseAndGetAddressOf()));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Default implementation of the OnRun method.
//      Does nothing.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
XamlRenderingBackgroundTask::OnRunImpl(
    _In_ wab::IBackgroundTaskInstance* pTaskInstance)
{
    return S_OK;
}

_Check_return_ HRESULT XamlRenderingBackgroundTaskFactory::EnableStandaloneHostingImpl()
{
    BackgroundTaskFrameworkContext::EnableStandaloneHosting();

    return S_OK;
}

_Check_return_ HRESULT XamlRenderingBackgroundTaskFactory::get_ResourcesImpl(_Outptr_result_maybenull_ xaml::IResourceDictionary** ppValue)
{
    *ppValue = nullptr;
    CValue value;

    // get_Resources can only be called from a UI thread where we have initialized DXamlCore.
    IFC_RETURN(DXamlServices::IsDXamlCoreInitialized() ? S_OK : RPC_E_WRONG_THREAD);

    // Note we're doing something a little unusual here.
    // Instead of getting a property tied to this object, we're using the internal CApplication handle
    // to get its resources. So this property ends up being a per-thread property, exposed on a global object.
    IFC_RETURN(CoreImports::DependencyObject_GetValue(
        DXamlCore::GetCurrent()->GetCoreAppHandle(),
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Application_Resources),
        &value));

    IFC_RETURN(CValueBoxer::UnboxObjectValue(&value, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ResourceDictionary), __uuidof(xaml::IResourceDictionary), reinterpret_cast<void**>(ppValue)));

    return S_OK;
}

_Check_return_ HRESULT XamlRenderingBackgroundTaskFactory::SetScalePercentageImpl(_In_ UINT percentage)
{
    // SetScalePercentageImpl can only be called from a UI thread where we have initialized DXamlCore. 
    // TODO: This function needs to improve to support multiple desktop top level windows (Win32) Task# 29571334
    
    IFC_RETURN(DXamlServices::IsDXamlCoreInitialized() ? S_OK : RPC_E_WRONG_THREAD);

    const auto& roots = DXamlServices::GetHandle()->GetContentRootCoordinator()->GetContentRoots();
    for(const auto& root : roots)
    {
        if (root->GetType() == CContentRoot::Type::CoreWindow)
        {
            const auto rootScale = RootScale::GetRootScaleForContentRoot(root);
            if (!rootScale) // Check that we still have an active tree
            {
                return S_OK;
            }

            if (percentage != 0.0f)
            {
                IFC_RETURN(rootScale->SetSystemScale(0.01f * percentage));
            }

            IFC_RETURN(DXamlCore::GetCurrent()->OnUWPWindowSizeChanged());
        }
    }

    return S_OK;
}