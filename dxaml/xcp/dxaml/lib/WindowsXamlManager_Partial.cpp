// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowsXamlManager.g.h"
#include "Window_Partial.h"
#include "DesktopWindowXamlSource_Partial.h"
#include "XamlIsland_Partial.h"

#include <XamlOneCoreTransforms.h>
#include <DependencyLocator.h>
#include <FrameworkTheming.h>

#include "FrameworkApplication_Partial.h"
#include <AppModel.h>
#include <RuntimeEnabledFeatures.h>
#include "Handle.h"
#include "wrlhelper.h"
#include "host.h"
#include <FeatureFlags.h>
#include <xcpwindow.h>

#include <FrameworkUdk/Containment.h>

// Bug 45792810: Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
// Bug 46468883: [1.4 servicing] Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
#define WINAPPSDK_CHANGEID_46468883 46468883

using namespace ctl;
using namespace DirectUI;

WindowsXamlManager::WindowsXamlManager()
{
}

WindowsXamlManager::~WindowsXamlManager()
{
    VERIFYHR(Close());
}

_Check_return_ HRESULT WindowsXamlManager::CheckWindowingModelPolicy()
{
    AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;
    LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy);
    if (status != ERROR_SUCCESS)
    {
        IFC_RETURN(E_FAIL);
    }

    if (policy != AppPolicyWindowingModel_ClassicDesktop)
    {
        // This thread was already initialized in the past
        // We dont support this as there are issues with Xaml statics
        //
        // Note 1:
        // The core could have been closed at this point
        // Do not use ErrorHelper::OriginateErrorUsingResourceID to be safe.
        //
        // Note 2:
        // NOTRACE here is important. The pattern is that OriginateError will return the reported
        // error as its own return code. This allows callers to call OriginateError() and propagate
        // the error as a single step. We need to NOTRACE here so that the captured error context
        // begins at the caller of OriginateError().
        //
        IFC_NOTRACE_RETURN(ErrorHelper::OriginateError(
            __HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED),
            wrl_wrappers::HStringReference(L"Cannot activate WindowsXamlManager. This type cannot be used in a UWP app. See: https://go.microsoft.com/fwlink/?linkid=875495").Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT WindowsXamlManagerFactory::InitializeForCurrentThreadImpl(_Outptr_ xaml_hosting::IWindowsXamlManager** ppReturnValue)
{
    ComPtr<WindowsXamlManager> spCore;
    IFC_RETURN(make<WindowsXamlManager>(&spCore));
    IFC_RETURN(spCore.CopyTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT WindowsXamlManager::XamlCore::Initialize(msy::IDispatcherQueue* dq)
{
    m_dispatcherQueue = dq;
    IFCFAILFAST(m_dispatcherQueue.As(&m_dispatcherQueue3));

    //
    // Deal with Application.Current. There are a few cases here:
    //
    //   1. The app has already created an instance of Application (or a derived type) before initializing Xaml. In this
    //      case Application.Current is already set, and Xaml should hook up to it for things like OnLaunched.
    //
    //   2. The app hasn't created an instance of Application (or a derived type) before initializing Xaml, but is
    //      keeping one alive from a previous time when Xaml was initialized. This is the scenario where an app has a
    //      reference to an Application object that persists after deinitializing Xaml and is now reinitializing Xaml.
    //      In this case we want to pick up their existing instance and reuse it as Application.Current.
    //
    //   3. The app hasn't created an instance of Application before initializing Xaml. In this case we make an instance
    //      of FrameworkApplication and use that as Application.Current.
    //
    ComPtr<FrameworkApplication> frameworkApplication;
    {
        // Take the lock here to ensure FrameworkApplication::ReleaseCurrent isn't happening on another thread while
        //  we're starting up the Application (see XamlCore::Close)
        CApplicationLock lock;
        ++s_instancesInProcess;

        frameworkApplication = FrameworkApplication::GetCurrentNoRef();
        if (frameworkApplication)
        {
            // Case 1. We can no-op here - the FrameworkApplication (or its derived class) has already registered itself via
            // FrameworkApplication::Initialize when it was created.
        }
        else if (FrameworkApplication::InitializeFromPreviousApplication())
        {
            // Case 2. FrameworkApplication successfully picked up the existing instance and reused it.
            frameworkApplication = FrameworkApplication::GetCurrentNoRef();
        }
        else
        {
            // Case 3. We have to create the framework application ourselves.
            IFC_RETURN(make<FrameworkApplication>(&frameworkApplication));
        }
    }

    IFC_RETURN(frameworkApplication->StartOnCurrentThread(nullptr /*pCallback*/));

    auto pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->EnsureCoreApplicationInitialized());

    // In XamlBridge mode, we can't use WinRT-only input.  Explicitly turn it off.
    // TODO: OS bug 14726409
    XamlOneCoreTransforms::EnsureInitialized(XamlOneCoreTransforms::InitMode::ForceDisable);

    ::EnableMouseInPointer(TRUE);

    // NotifyEndOfReferenceTrackingOnThread destroys all RCWs of the current thread including NON Xaml ones
    // This will break text input as WPF maintains RCW for TSF among other things.
    pCore->DisableNotifyEndOfReferenceTrackingOnThread();

    auto shutdownStartingCallback = WRLHelper::MakeAgileCallback<
        wf::ITypedEventHandler<msy::DispatcherQueue*, msy::DispatcherQueueShutdownStartingEventArgs*>>
        ([](msy::IDispatcherQueue*, msy::IDispatcherQueueShutdownStartingEventArgs*)
    {
        tls_xamlCore->SyncCloseAllManagers();
        return S_OK;
    });

    IFCFAILFAST(m_dispatcherQueue3->add_FrameworkShutdownStarting(
        shutdownStartingCallback.Get(),
        &m_frameworkShutdownStartingToken));

    return S_OK;
}

_Check_return_ HRESULT WindowsXamlManager::Initialize()
{
    IFC_RETURN(WeakReferenceSourceNoThreadId::Initialize());

    wrl::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46468883>())
    {
        IFC_RETURN(ActivationFactoryCache::GetActivationFactoryCache()->GetDispatcherQueueStatics(&dispatcherQueueStatics));
    }
    else
    {
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
            &dispatcherQueueStatics));
    }
    IFC_RETURN(dispatcherQueueStatics->GetForCurrentThread(&m_dispatcherQueue));
    if (!m_dispatcherQueue)
    {
        IFC_RETURN(ErrorHelper::OriginateError(
            HRESULT_FROM_WIN32(ERROR_INVALID_STATE),
            XSTRING_PTR_EPHEMERAL(
                L"A DispatcherQueue must exist on the thread before performing this operation.  "
                L"Please create a DispatcherQueueController before creating a WindowsXamlManager or "
                L"DesktopWindowXamlSource object."
            ), true /*outputToDebugger*/));
    }

    if (!tls_xamlCore)
    {
        IFC_RETURN(CheckWindowingModelPolicy());

        tls_xamlCore = std::make_shared<XamlCore>();
        IFC_RETURN(tls_xamlCore->Initialize(m_dispatcherQueue.Get()));
    }

    // Take a strong ref on tls_xamlCore here just to keep it alive if the thread shuts down suddenly.
    m_xamlCore = tls_xamlCore;

    if (tls_xamlCore && tls_xamlCore->GetState() == XamlCore::State::Closing)
    {
        // The XamlCore is in the middle of an asynchronous shutdown.
        // Cancel the Close, we're going to re-use this tls_xamlCore.
        tls_xamlCore->SetState(XamlCore::State::Normal);
    }

    tls_xamlCore->AddManager(this);

    // This will make sure that it doesn't get cleared off thread in WeakReferenceSourceNoThreadId::OnFinalReleaseOffThread()
    // as it has thread-local variables and needs to be disposed off by the same thread
    AddToReferenceTrackingList();

    return S_OK;
}

_Check_return_ HRESULT WindowsXamlManager::EnqueueClose()
{
    tls_xamlCore->SetState(XamlCore::State::Closing);
    auto closeCallback = WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([core = tls_xamlCore]() -> HRESULT
    {
        // It's possible the final Close has already happened, or that it's been cancelled.  So, only perform the
        // Close if we're still in the Closing state.
        if (core->GetState() == XamlCore::State::Closing)
        {
            IFC_RETURN(core->Close());
        }
        return S_OK;
    });
    boolean enqueued = false;
    IFCFAILFAST(m_dispatcherQueue->TryEnqueue(closeCallback.Get(), &enqueued));

    // "enqueued" can be false here if the DispatcherQueue is in the shutdown process.
    // We require that DesktopWindowXamlSource and WindowsXamlManager objects are closed before the DispatcherQueue
    // is shut down.  In the future we'll make this easier for apps to do correctly.
    // http://task.ms/40672600 Make it easy for apps to shut down islands correctly (Xaml participates in "Organized Shutdown")
    FAIL_FAST_ASSERT(enqueued);
    return S_OK;
}

WindowsXamlManager::XamlCore::~XamlCore()
{
    ASSERT(m_state == State::Closed);
    ASSERT(m_managers.empty());
}

_Check_return_ HRESULT WindowsXamlManager::XamlCore::Close()
{
    ASSERT(GetState() == XamlCore::State::Closing);
    SetState(XamlCore::State::Closed);

    if (m_dispatcherQueue && m_frameworkShutdownStartingToken.value)
    {
        IFCFAILFAST(m_dispatcherQueue3->remove_FrameworkShutdownStarting(m_frameworkShutdownStartingToken));
        m_frameworkShutdownStartingToken = {};
    }

    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown))
    {
        // If EnableCoreShutdown on, then we are running under test
        // Do nothing in this case and let the core shutdown via tests.
        return S_OK;
    }

    DXamlCore* pCore = DXamlCore::GetCurrent();
    ASSERT(pCore); // Unexpected that we get here in a state where there's not a live DXamlCore on the thread.
    if (pCore)
    {
        CCoreServices* pCoreHandle = pCore->GetHandle();
        ASSERT(pCoreHandle); // Unexpected that we'd not have a valid CCoreServices at this point.

        // Close all the outstanding DesktopWindowXamlSource instances.  This releases ContentIsland and related IXP
        // objects before we shut everything down.
        if (CXamlIslandRootCollection* xamlIslandRootCollection = pCoreHandle->GetXamlIslandRootCollection())
        {
            if (CDOCollection* collection = xamlIslandRootCollection->GetChildren())
            {
                for (CDependencyObject* child : *collection)
                {
                    CXamlIslandRoot* xamlIslandRoot = do_pointer_cast<CXamlIslandRoot>(child);

                    ctl::ComPtr<DirectUI::XamlIsland> xamlIsland;
                    IFCFAILFAST(pCore->GetPeer<DirectUI::XamlIsland>(xamlIslandRoot, &xamlIsland));

                    ctl::ComPtr<IInspectable> islandOwner;
                    if (xamlIsland->TryGetOwner(&islandOwner) && islandOwner)
                    {
                        ctl::ComPtr<DirectUI::DesktopWindowXamlSource> desktopWindowXamlSource;
                        if (SUCCEEDED(islandOwner.As<DirectUI::DesktopWindowXamlSource>(&desktopWindowXamlSource)))
                        {
                            IGNOREHR(desktopWindowXamlSource->Close());
                        }
                    }
                }
            }
        }

        // Run all the async work we still need to run for this core before shutting down.
        // This allows, for example, all the pending Unloaded events to be fired.
        static_cast<CXcpDispatcher*>(pCoreHandle->GetHostSite()->GetXcpDispatcher())->Drain();

        // Make sure that all GC'ed objects are cleaned up.
        pCore->ReleaseQueuedObjects(/* bSync */ TRUE);

        // Make sure to reset the XAML schema context, whose type cache could now have keys derived from
        // stale type IDs.
        IFC_RETURN(pCoreHandle->RefreshXamlSchemaContext());

        IFC_RETURN(DXamlCore::Deinitialize(DeinitializationType::Default));
    }

    //
    // FrameworkApplication::Initialize does an unconditional ctl::addref_interface on the single FrameworkApplication
    // instance in g_pApplication. Its matching release is in FrameworkApplication::ReleaseCurrent, which was
    // conditionally called from WindowsXamlManager::XamlCore::Close when Xaml created the FrameworkApplication.
    // ReleaseCurrent must always be called, otherwise we leak the FrameworkApplication instance in scenarios where Xaml
    // doesn't create it.
    //
    // This happens when the app creates an instance of a derived Application type before initializing its first
    // WindowsXamlManager. The derived type still goes through FrameworkApplication::Initialize, which sets the global
    // g_pApplication and does an AddRef on it. We need to have a balancing Release.
    //
    // There are two special circumstances that we need to watch out for when setting up this Release:
    //
    //   1. We need to be careful to not call Release too many times. Setting the global g_pApplication is done once per
    //      process, and WindowsXamlManager cleanup is done once per thread. Blindly setting m_frameworkApplication
    //      everywhere results in too many Release calls. The naive solution is to check for the existence of
    //      g_pApplication, and only call Release from the first WindowsXamlManager created, when it hasn't been set
    //      yet, except...
    //
    //   2. Not every WindowsXamlManager runs WindowsXamlManager::XamlCore::Close. The last WindowsXamlManager on the
    //      thread to be deleted calls Close, but there's no guarantee that this is the same object as the first
    //      WindowsXamlManager that was created on the thread. So the first WindowsXamlManger created might never get to
    //      call Close at all.
    //
    // The solution is to only call the matching release on the last WindowsXamlManager to be deleted in the process.
    // Conveniently, we know when the last instance is going away because we're tracking the XamlCore instance count in
    // s_instancesInProcess.
    //
    {
        CApplicationLock applicationLock;

        --s_instancesInProcess;
        const bool lastInstanceInProcess = (s_instancesInProcess == 0);

        if (lastInstanceInProcess)
        {
            // MetadataAPI::Reset() is called in FrameworkApplication::ReleaseCurrent when m_metadataRef is reset
            FrameworkApplication::ReleaseCurrent();
        }
    }

    DependencyLocator::UninitializeThread();

    // "this" ptr may be null after this next line
    tls_xamlCore = nullptr;
    return S_OK;
}

_Check_return_ HRESULT WindowsXamlManager::Close()
{
    IFC_RETURN(CloseImpl(false /*synchronous*/));
    return S_OK;
}

_Check_return_ HRESULT WindowsXamlManager::CloseImpl(bool synchronous)
{
    if (m_bIsClosed)
    {
        return S_OK;
    }

    IFC_RETURN(CheckThread());

    m_bIsClosed = true;

    if (!tls_xamlCore)
    {
        // There might not be a XamlCore on the thread if the WindowsXamlManager didn't get successfully initialized.
        return S_OK;
    }

    tls_xamlCore->RemoveManager(this);

    if (tls_xamlCore->ManagerCount() == 0)
    {
        if (synchronous)
        {
            // Note we might already be in the "Closing" state.  This would mean that a Close has already been
            // scheduled.  That's fine, we'll just close synchronously here anyway, and when the scheduled
            // callback runs (see EnqueueClose), it will check to make sure the object still needs to be closed
            // before doing anything.
            tls_xamlCore->SetState(XamlCore::State::Closing);
            IFC_RETURN(tls_xamlCore->Close());
        }
        else
        {
            IFC_RETURN(EnqueueClose());
        }
    }

    return S_OK;
}

void WindowsXamlManager::XamlCore::AddManager(_In_ WindowsXamlManager* wxm)
{
    auto it = std::find(m_managers.begin(), m_managers.end(), wxm);
    if (it == m_managers.end())
    {
        m_managers.push_back(wxm);
    }
}

void WindowsXamlManager::XamlCore::RemoveManager(_In_ WindowsXamlManager* wxm)
{
    auto it = std::find(m_managers.begin(), m_managers.end(), wxm);
    ASSERT(it != m_managers.end());
    if (it != m_managers.end())
    {
        m_managers.erase(it);
    }

    if (m_managers.empty())
    {
        // Deallocate the memory
        m_managers.shrink_to_fit();
    }
}

void WindowsXamlManager::XamlCore::SyncCloseAllManagers()
{
    // Make a local copy since closing these instances synchronously will change m_instances
    std::vector<WindowsXamlManager*> managersCopy {m_managers};
    for (WindowsXamlManager* manager : managersCopy)
    {
        // When the last manager closes, it will trigger a close on this XamlCore object
        manager->CloseImpl(true /*synchronous*/);
    }
}