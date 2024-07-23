// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkApplication.g.h"
#include "DebugSettings.g.h"
#include "WindowCreatedEventArgs.g.h"
#include "ApplicationInitializationCallbackParams.g.h"
#include "Window.g.h"
#include "ResourceManagerRequestedEventArgs.g.h"
#include "UnhandledExceptionEventArgs.g.h"
#include <FrameworkTheming.h>
#include <DependencyLocator.h>
#include <MetadataResetter.h>
#include <process.h>
#include <RuntimeEnabledFeatures.h>
#include "NormalLaunchActivatedEventArgs.h"
#include "LaunchActivatedEventArgs.g.h"
#include <xstrutil.h>
#include "theming\inc\Theme.h"
#include "XamlIslandRoot.g.h"
#include <FeatureFlags.h>
#include <WindowsXamlManager_Partial.h>
#include <DesktopWindowXamlSource.g.h>
#include <DesktopWindowImpl.h>
#include <WindowingCoreContentApi.h>
#include <Microsoft.UI.Dispatching.Interop.h>
#include <Microsoft.Windows.ApplicationModel.Resources.h>

using namespace RuntimeFeatureBehavior;
using namespace DirectUI;
using namespace DirectUISynonyms;
using DirectUI::Application;

// A per-process FrameworkApplication instance, used for Application.Current. Xaml has a reference on this object.
static FrameworkApplication* g_pApplication = NULL;

//
// A per-process FrameworkApplication instance. Used in the scenario where an app instantiates an Application object and
// keeps it alive beyond deinitializing Xaml. We still want Application.Current to return nullptr (since Xaml is
// deinitialized), but if the app reinitializes Xaml we want to pick up the instance that we had before. This weak ref
// allows Xaml to keep track of the Application even when deinitialized. We use a weak ref so that the Xaml doesn't leak
// the Application object. A weak ref also lets us detect cases where the app releases the Application object before
// reinitializing Xaml, in which case Xaml can create a new default FrameworkApplication object as needed.
//
// Note that Xaml will only track the value of Application.Current when we shut down. The app can create 5 instances of
// Application, with the first being Application.Current, then release the first instance but keep the other 4 alive.
// Xaml will make no attempt to pick up one of those other 4 instances to use as Application.Current when we reinitialize.
//
// This field is non-null only when Xaml is deinitialized and needs to keep track of Application.Current. Access to it
// is also guarded by CApplicationLock.
//
static ctl::WeakRefPtr g_previousApplicationWeak;

static CRITICAL_SECTION g_csApplication;
static ctl::ComPtr<xaml::IApplicationInitializationCallback> g_spApplicationInitializationCallback;
bool CApplicationLock::s_fStaticCSInitialized = false;

_Check_return_ HRESULT RunInActivationMode()
{
    HRESULT hr = S_OK;
    wac::IFrameworkViewSource* pFrameworkViewSource = NULL;
    ctl::ComPtr<wac::ICoreApplication> spCoreApplication = NULL;

    // instantiate Jupiter's ViewProviderFactory
    IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_FrameworkViewSource).Get(), &pFrameworkViewSource));

    // call CoreApplication.Run()
    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &spCoreApplication));
    IFC(spCoreApplication->Run(pFrameworkViewSource));

Cleanup:
    ReleaseInterface(pFrameworkViewSource);
    RRETURN(hr);
}

// Invoked as part of the desktop app initialization path, see startup-overview.md for more info.
_Check_return_ HRESULT FrameworkApplication::StartOnCurrentThreadImpl(_In_ xaml::IApplicationInitializationCallback* pCallback)
{
    g_spApplicationInitializationCallback = pCallback;

    // if this thread wasn't already initialized in FrameworkApplication::StartDesktop, then init it now
    if (!DXamlCore::IsInitializedStatic())
    {
        IFC_RETURN(DXamlCore::Initialize(InitializationType::IslandsOnly));
    }

    IFC_RETURN(DXamlCore::GetCurrent()->EnsureCoreApplicationInitialized());

    // ICoreWindow parameter is not created as part of the Desktop initialization path. It is only needed for UWP.
    IFC_RETURN(DXamlCore::GetCurrent()->ConfigureJupiterWindow(nullptr));

    // call the OnLaunchedProtected method
    ctl::ComPtr<NormalLaunchActivatedEventArgs> uwpLaunchActivatedEventArgs;
    IFC_RETURN(NormalLaunchActivatedEventArgs::Create(uwpLaunchActivatedEventArgs.ReleaseAndGetAddressOf()));
    IFC_RETURN(InvokeOnLaunchActivated(uwpLaunchActivatedEventArgs.Get()));

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::CreateIslandRootImpl(_Outptr_ xaml_hosting::IXamlIslandRoot** returnValue)
{
    auto dxamlCore = DXamlCore::GetCurrent();

    // http://osgvsowi/17333449 - (deliverable) This is a temporary trick to kick off XAML's normal applicaiton startup path.
    // we need to refactor islands and xaml::Window to reduce the complexity of the code as captured by the task below
    // https://microsoft.visualstudio.com/OS/_workitems/edit/37066232
    DirectUI::Window* window = DXamlCore::GetCurrent()->GetDummyWindowNoRef();
    IFC_RETURN(window->EnsureInitializedForIslands());

    // Create a new XamlIslandRoot.
    ctl::ComPtr<DirectUI::XamlIslandRoot> newIsland;
    IFC_RETURN(ctl::make<DirectUI::XamlIslandRoot>(nullptr, &newIsland));
    IFC_RETURN(ctl::do_query_interface(*returnValue, newIsland.Get()));

    // Notify core of new XamlIslandRoot
    auto coreServices = dxamlCore->GetHandle();
    coreServices->AddXamlIslandRoot(static_cast<CXamlIslandRoot*>(newIsland->GetHandle()));

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::CreateIslandRootWithContentBridgeImpl(_In_ IInspectable* owner, _In_ IInspectable* contentBridge, _Outptr_ xaml_hosting::IXamlIslandRoot** returnValue)
{
    auto dxamlCore = DXamlCore::GetCurrent();

    // http://osgvsowi/17333449 - (deliverable) This is a temporary trick to kick off XAML's normal application startup path.
    // we need to refactor islands and xaml::Window to reduce the complexity of the code as captured by the task below
    // https://microsoft.visualstudio.com/OS/_workitems/edit/37066232
    DirectUI::Window* window = DXamlCore::GetCurrent()->GetDummyWindowNoRef();
    IFC_RETURN(window->EnsureInitializedForIslands());

    // Create a new XamlIslandRoot
    ctl::ComPtr<DirectUI::XamlIslandRoot> newIsland;
    IFC_RETURN(ctl::make<DirectUI::XamlIslandRoot>(nullptr, &newIsland));
    IFC_RETURN(ctl::do_query_interface(*returnValue, newIsland.Get()));

    // Notify core of new XamlIslandRoot
    auto coreServices = dxamlCore->GetHandle();
    coreServices->AddXamlIslandRoot(static_cast<CXamlIslandRoot*>(newIsland->GetHandle()));

    newIsland.Cast<DirectUI::XamlIslandRoot>()->SetOwner(owner);

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::RemoveIslandImpl(_In_ xaml_hosting::IXamlIslandRoot* value)
{
    auto xamlIslandRoot = static_cast<CXamlIslandRoot*>(static_cast<DirectUI::XamlIslandRoot*>(value)->GetHandle());

    if (xamlIslandRoot)
    {
        auto dxamlCore = DXamlCore::GetCurrent();
        auto coreServices = dxamlCore->GetHandle();
        coreServices->RemoveXamlIslandRoot(xamlIslandRoot);
    }

    return S_OK;
}

// Shared startup for UWP and desktop apps
// See startup-overview.md for details
_Check_return_ HRESULT FrameworkApplicationFactory::StartImpl(_In_opt_ xaml::IApplicationInitializationCallback* pCallback)
{
    g_spApplicationInitializationCallback = pCallback;

    // Determine which AppPolicyWindowingModel the application is using.
    //
    //     AppPolicyWindowingModel_None
    //     AppPolicyWindowingModel_Universal (WinUI UWP)
    //     AppPolicyWindowingModel_ClassicDesktop (WinUI Desktop)
    //     AppPolicyWindowingModel_ClassicPhone
    //
    AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;
    LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy);
    if (status != ERROR_SUCCESS)
    {
        IFC_RETURN(E_FAIL);
    }

    if (policy == AppPolicyWindowingModel_ClassicDesktop)
    {
        return FrameworkApplication::StartDesktop();
    }
    else if (policy == AppPolicyWindowingModel_Universal)
    {
        return FrameworkApplication::StartUWP(pCallback);
    }

    return E_FAIL;
}

/* static */ _Check_return_ HRESULT FrameworkApplication::StartDesktop()
{
    ctl::ComPtr<ApplicationInitializationCallbackParams> pParams;
    ctl::ComPtr<xaml::Hosting::IWindowsXamlManagerStatics> windowsXamlManagerFactory;
    ctl::ComPtr<xaml::Hosting::IWindowsXamlManager> windowsXamlManager;

    wrl::ComPtr<msy::IDispatcherQueueControllerStatics> dispatcherQueueControllerStatics;
    wrl::ComPtr<msy::IDispatcherQueueController> dispatcherQueueController;
    wrl::ComPtr<msy::IDispatcherQueueController2> dispatcherQueueController2;

    IFCFAILFAST(wf::GetActivationFactory(
        wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueueController).Get(),
        &dispatcherQueueControllerStatics));
    IFCFAILFAST(dispatcherQueueControllerStatics->CreateOnCurrentThread(&dispatcherQueueController));
    IFCFAILFAST(dispatcherQueueController.As(&dispatcherQueueController2));

    // init Jupiter for this thread
    IFC_RETURN(DXamlCore::Initialize(InitializationType::IslandsOnly));

    // By spec, when an app initializes Xaml for use in a WinUI Desktop scenario, DispatcherShutdownMode
    // defaults to "OnLastWindowClose".
    DXamlCore::GetCurrent()->SetDispatcherShutdownMode(xaml::DispatcherShutdownMode_OnLastWindowClose);

    //  Invoke the ApplicationInitialization callback set by FrameworkApplication::StartImpl
    if (g_spApplicationInitializationCallback)
    {
        // Invoke the callback, usually to create a custom Application object instance
        IFCFAILFAST(ctl::ComObject<ApplicationInitializationCallbackParams>::CreateInstance(&pParams));
        IFCFAILFAST(g_spApplicationInitializationCallback->Invoke(pParams.Get()));
        g_spApplicationInitializationCallback.Reset();
    }

    // Create WindowsXamlManager (WindowsXamlManager::Initialize will call FrameworkApplication::StartOnCurrentThread())
    IFCFAILFAST(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Hosting_WindowsXamlManager).Get(), &windowsXamlManagerFactory));
    IFCFAILFAST(windowsXamlManagerFactory->InitializeForCurrentThread(&windowsXamlManager));

    // We must have an XAML application instance at this point
    if(FrameworkApplication::GetCurrentNoRef() == nullptr)
    {
        XAML_FAIL_FAST();
    }

    //  Start the main WinUI Desktop message loop
    FrameworkApplication::RunDesktopWindowMessageLoop();

    // Note we hold a reference to windowsXamlManager here, and we don't close it, because we want Xaml to shut down
    // during DispatcherQueue.ShutdownQueue (in the DispatcherQueue.FrameworkShutdownStarting event handler).  See
    // xaml-shutdown.md for more detail about the shutdown process.
    
    IFCFAILFAST(dispatcherQueueController2->ShutdownQueue());

    return S_OK;
}

/* static */ _Check_return_ HRESULT FrameworkApplication::StartUWP(_In_ xaml::IApplicationInitializationCallback* pCallback)
{
    return RunInActivationMode();
}

_Check_return_ HRESULT FrameworkApplicationFactory::EnableFailFastOnStowedExceptionImpl()
{
    SetProcessFailFastOnErrors(true);
    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::add_UnhandledException(_In_ xaml::IUnhandledExceptionEventHandler* pValue, _Out_ EventRegistrationToken* pToken)
{
    return m_UnhandledExceptionEventSource.Add(pValue, pToken);
}

IFACEMETHODIMP FrameworkApplication::remove_UnhandledException(_In_ EventRegistrationToken token)
{
    return m_UnhandledExceptionEventSource.Remove(token);
}

_Check_return_ HRESULT STDMETHODCALLTYPE 
FrameworkApplication::add_ResourceManagerRequested(
    _In_ wf::ITypedEventHandler<IInspectable*, xaml::ResourceManagerRequestedEventArgs*>* pValue, 
    _Out_ EventRegistrationToken* pToken)
{
    return m_resourceManagerRequestedEventSource.Add(pValue, pToken);
}

_Check_return_ HRESULT STDMETHODCALLTYPE 
FrameworkApplication::remove_ResourceManagerRequested(_In_ EventRegistrationToken token)
{
    return m_resourceManagerRequestedEventSource.Remove(token);
}

_Check_return_ HRESULT GetCoreApplication(_Outptr_ wac::ICoreApplication** ppCoreApp)
{
    return ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), ppCoreApp);
}

_Check_return_ HRESULT GetCoreApplicationExit(_Outptr_ wac::ICoreApplicationExit** ppCoreAppExit)
{
    return ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), ppCoreAppExit);
}

IFACEMETHODIMP FrameworkApplication::add_Suspending(_In_ xaml::ISuspendingEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
    if (pPLMHandler != nullptr)
    {
        IFC_RETURN(pPLMHandler->AddAppSuspendHandler(pHandler, pToken));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::remove_Suspending(_In_ EventRegistrationToken token)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    // Task 31111854: FrameworkApplication::add_Suspending/remove_Suspending have invalid WinRT behavior
    if (token.value)
    {
        IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
        if (pPLMHandler != nullptr)
        {
            IFC_RETURN(pPLMHandler->RemoveAppSuspendHandler(token));
        }
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::add_Resuming(_In_ wf::IEventHandler<IInspectable*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
    if (pPLMHandler != nullptr)
    {
        IFC_RETURN(pPLMHandler->AddAppResumeHandler(pHandler, pToken));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::remove_Resuming(_In_ EventRegistrationToken token)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
    if (pPLMHandler != nullptr)
    {
        IFC_RETURN(pPLMHandler->RemoveAppResumeHandler(token));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::add_LeavingBackground(_In_ xaml::ILeavingBackgroundEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
    if (pPLMHandler != nullptr)
    {
        IFC_RETURN(pPLMHandler->AddAppLeavingBackgroundHandler(pValue, ptToken));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::remove_LeavingBackground(_In_ EventRegistrationToken tToken)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
    if (pPLMHandler != nullptr)
    {
        IFC_RETURN(pPLMHandler->RemoveAppLeavingBackgroundHandler(tToken));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::add_EnteredBackground(_In_ xaml::IEnteredBackgroundEventHandler* pValue, _Out_ EventRegistrationToken* ptToken)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
    if (pPLMHandler != nullptr)
    {
        IFC_RETURN(pPLMHandler->AddAppEnteredBackgroundHandler(pValue, ptToken));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkApplication::remove_EnteredBackground(_In_ EventRegistrationToken tToken)
{
    XAML::PLM::PLMHandler* pPLMHandler = NULL;

    IFC_RETURN(GetPLMHandlerForCallingThread(&pPLMHandler));
    if (pPLMHandler != nullptr)
    {
        IFC_RETURN(pPLMHandler->RemoveAppEnteredBackgroundHandler(tToken));
    }

    return S_OK;
}
//-----------------------------------------------------------------------------
//
// WARNING: An application instance is not guaranteed. The XAML runtime can
// operate in modes where there is no Application object.
//
// Callers must handle a NULL return or guarantee that they only run in a
// context where we have an Application instance.
//
//-----------------------------------------------------------------------------
FrameworkApplication* FrameworkApplication::GetCurrentNoRef()
{
    CApplicationLock lock;

    return g_pApplication;
}

/* static */ bool FrameworkApplication::InitializeFromPreviousApplication()
{
    CApplicationLock lock;

    // There was an Application object when Xaml last shut down. Check whether it's still alive (i.e. the app kept it
    // alive), and if so reuse it as Application.Current.
    ctl::ComPtr<FrameworkApplication> previousApplication = g_previousApplicationWeak.AsOrNull<FrameworkApplication>();
    if (previousApplication)
    {
        g_pApplication = previousApplication.Get();
        ctl::addref_interface(g_pApplication);
        return true;
    }

    return false;
}

void FrameworkApplication::ReleaseCurrent()
{
    CApplicationLock lock;

    if (g_pApplication != nullptr)
    {
        // Remember the Application.Current instance before we release it. It's possible the app is keeping the object
        // alive without Xaml, in which case Xaml should reuse the same instance should we ever get reinitialized.
        IFCFAILFAST(ctl::AsWeak(g_pApplication, &g_previousApplicationWeak));
        
        // The metadata store may hold a reference to an IXamlMetadataProvider, which is usually the
        // Application object (which derives from FrameworkApplication). In other words, there may be
        // a reference cycle. We want to break that cycle when the main FrameworkView goes away.
        g_pApplication->m_metadataRef = nullptr;
        ctl::release_interface(g_pApplication);
    }
}

std::shared_ptr<MetadataResetter> FrameworkApplication::GetMetadataReference()
{
    return m_metadataRef;
}

_Check_return_ HRESULT FrameworkApplicationFactory::get_CurrentImpl(_Outptr_result_maybenull_ xaml::IApplication** ppValue)
{
    FrameworkApplication* pInstance = FrameworkApplication::GetCurrentNoRef();

    if (pInstance)
    {
        IFC_RETURN(ctl::do_query_interface(*ppValue, pInstance));
    }
    else
    {
        *ppValue = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplicationFactory::LoadComponentImpl(
    _In_ IInspectable *pComponent,
    _In_ wf::IUriRuntimeClass *pUri)
{
    RRETURN(LoadComponentWithResourceLocation(pComponent, pUri, xaml_primitives::ComponentResourceLocation_Application));
}

_Check_return_ HRESULT FrameworkApplicationFactory::LoadComponentWithResourceLocationImpl(
    _In_ IInspectable *pComponent,
    _In_ wf::IUriRuntimeClass *pUri,
    _In_ xaml_primitives::ComponentResourceLocation resourceLocation)
{
    HRESULT hr = S_OK;

    wrl_wrappers::HString strInternalUri;
    xstring_ptr strUri;

    IFC(pUri->get_AbsoluteUri(strInternalUri.GetAddressOf()));

    IFC(xstring_ptr::CloneRuntimeStringHandle(strInternalUri.Get(), &strUri));

    IFC(FrameworkApplication::LoadComponent(
        pComponent,
        strUri,
        resourceLocation));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::LoadComponent(
    _In_ IInspectable* pComponent,
    _In_ const xstring_ptr& strUri,
    xaml_primitives::ComponentResourceLocation resourceLocation)
{
    ctl::ComPtr<xaml::IApplication> application;
    ctl::ComPtr<DependencyObject> dobj;
    CDependencyObject* pCoreDO = nullptr;
    bool fRootWasPegged = false;

    if (pComponent == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // LoadComponent can only be called from a UI thread where we have initialized DXamlCore.
    IFC_RETURN(DXamlServices::IsDXamlCoreInitialized() ? S_OK : RPC_E_WRONG_THREAD);
    DXamlCore* pCore = DXamlCore::GetCurrent();

    // extract the core DO handle from the component
    if (SUCCEEDED(pComponent->QueryInterface(__uuidof(xaml::IApplication), reinterpret_cast<void**>(application.ReleaseAndGetAddressOf()))))
    {
        // special case: when calling LoadComponent on the Application, use this thread's internal CApplication handle instead
        pCoreDO = pCore->GetCoreAppHandle();

        // If we're loading the XAML for the application and haven't yet initialized the internal CApplication,
        // then we'll cache the location from which to load it for later - we'll load it when we initialize the
        // CApplication object.
        if (!pCoreDO)
        {
            FrameworkApplication::GetCurrentNoRef()->m_appXamlPath = strUri;
            return S_OK;
        }
    }
    else
    {
        // Make sure the component is a DO. If it's already a DO, the following call
        // will do nothing but a QI. Otherwise it'll be wrapped inside an ExternalObjectReference.
        IFC_RETURN(ExternalObjectReference::ConditionalWrap(pComponent, &dobj));

        fRootWasPegged = dobj->IsPeggedNoRef();

        pCoreDO = dobj->GetHandle();
        IFCEXPECT_RETURN(pCoreDO);
    }

    HRESULT hr = CoreImports::Application_LoadComponent(
        pCore->GetHandle(),
        pCoreDO,
        strUri,
        static_cast<::ComponentResourceLocation>(resourceLocation));

    if (FAILED(hr))
    {
        // Translate to XamlParseFailed error. The CLR knows how to translate this to
        // a XamlParseException.
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_XAMLPARSEFAILED);
    }

    IFC_RETURN(hr);

    // If the component was not pegged at got pegged during this call
    // then ensure that we restore the initial state.
    // The incoming object can be pegged from the parser and we don't want
    // to disturb that state here
    if (dobj && !fRootWasPegged)
    {
        dobj->UnpegNoRef();
    }

    return S_OK;
}

FrameworkApplication::FrameworkApplication()
{
    IGNOREHR(xstring_ptr::CloneBuffer(STR_LEN_PAIR(L"ms-resource:///Files/app.xaml"), &m_appXamlPath));
}

FrameworkApplication::~FrameworkApplication()
{
    CApplicationLock lock;

    if (AppPolicyWindowingModel_ClassicDesktop != GetAppPolicyWindowingModel())
    {
        // Remove 'BackgroundActivated' event handler
        HookBackgroundActivationEvents(false);
    }

    if (g_pApplication == this)
    {
        g_pApplication = NULL;
    }

    if (m_pDebugSettings)
    {
        m_pDebugSettings->UpdatePeg(false);
    }
    ctl::release_interface(m_pDebugSettings);
}

_Check_return_ HRESULT FrameworkApplication::Initialize()
{
    {
        CApplicationLock lock;

        // Only allow a single application singleton to be created.
        IFCEXPECT_RETURN(!g_pApplication);

        g_pApplication = this;
        ctl::addref_interface(this);
    }

    IFC_RETURN(ctl::ComObject<DirectUI::DebugSettings>::CreateInstance(&m_pDebugSettings));
    IFCEXPECT_RETURN(m_pDebugSettings);   // Should never fail
    m_pDebugSettings->UpdatePeg(true);

    // Set up the metadata resetter. This object clears out the process-wide metadata when it is safe to do so (before
    // DLLs are getting unloaded, but after we're done shutting down the visual tree).
    m_metadataRef = std::make_shared<MetadataResetter>();

    // Determine which AppPolicyWindowingModel is being used. Use Application::GetAppPolicyWindowingModel() to
    // get the current Windowing model.
    //
    //     AppPolicyWindowingModel_None
    //     AppPolicyWindowingModel_Universal (WinUI UWP)
    //     AppPolicyWindowingModel_ClassicDesktop (WinUI Desktop)
    //     AppPolicyWindowingModel_ClassicPhone
    //
    LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &m_appPolicyWindowingModel);
    if (status != ERROR_SUCCESS)
    {
        IFC_RETURN(E_FAIL);
    }

    //  Register for CoreApplication's 'BackgroundActivated' event if not running as WinUI Desktop
    if (AppPolicyWindowingModel_ClassicDesktop != m_appPolicyWindowingModel)
    {
        // 'BackgroundActivated' is fired when in-process background tasks are invoked, after FrameworkView is created and
        // initialized, and the App constructor is called.
        IFC_RETURN(HookBackgroundActivationEvents(true));
    }

    IFC_RETURN(FrameworkApplicationGenerated::Initialize());

    IFC_RETURN(put_FocusVisualKindImpl(xaml::FocusVisualKind_HighVisibility));

    return S_OK;
}

CApplicationLock::CApplicationLock()
{
    EnterCriticalSection(&g_csApplication);
}

CApplicationLock::~CApplicationLock()
{
    LeaveCriticalSection(&g_csApplication);
}

bool CApplicationLock::IsInitialized()
{
    return CApplicationLock::s_fStaticCSInitialized;
}

_Check_return_ HRESULT FrameworkApplication::GlobalInit()
{
    if (0 == InitializeCriticalSectionAndSpinCount(&g_csApplication, 0x80000001))
    {
        return E_FAIL;
    }

    CApplicationLock::s_fStaticCSInitialized = true;
    return S_OK;
}

void FrameworkApplication::GlobalDeinit()
{
    if (CApplicationLock::IsInitialized())
    {
        {
            CApplicationLock lock;

            ctl::release_interface(g_pApplication);
            
            // Properly releasing the weakptr here requires vccorlib to be loaded, because it's handled
            // by vccorlib140[d]!Platform::Details::ControlBlock.  But this function is called during DLL
            // unload, and vccorlib isn't always around.  So for now we just leak the IWeakReference.
            // http://task.ms/39377823 XAML can fully unload its own DLL and dependencies after use
            g_previousApplicationWeak.Detach();
        }

        DeleteCriticalSection(&g_csApplication);
        CApplicationLock::s_fStaticCSInitialized = false;
    }
}

_Check_return_ HRESULT FrameworkApplication::MainASTAInitialize()
{
    HRESULT hr = S_OK;
    ApplicationInitializationCallbackParams *pParams = NULL;
    CApplicationLock lock;

    if (g_spApplicationInitializationCallback)
    {
        // invoke the callback
        IFC(ctl::ComObject<ApplicationInitializationCallbackParams>::CreateInstance(&pParams));
        IFC(g_spApplicationInitializationCallback->Invoke(pParams));

        // release the callback, since we no longer need it
        g_spApplicationInitializationCallback.Reset();
    }

    // we must have an application instance at this point
    IFCEXPECT(FrameworkApplication::GetCurrentNoRef());

Cleanup:
    ctl::release_interface(pParams);
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::get_ResourcesImpl(_Outptr_ xaml::IResourceDictionary** pValue)
{
    HRESULT hr = S_OK;
    CValue value;

    // get_Resources can only be called from a UI thread where we have initialized DXamlCore.
    IFC(DXamlServices::IsDXamlCoreInitialized() ? S_OK : RPC_E_WRONG_THREAD);

    // Note we're doing something a little unusual here.
    // Instead of getting a property tied to this object, we're using the internal CApplication handle
    // to get its resources. So this property ends up being a per-thread property, exposed on a global object.
    IFC(CoreImports::DependencyObject_GetValue(
        DXamlCore::GetCurrent()->GetCoreAppHandle(),
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Application_Resources),
        &value));

    IFC(CValueBoxer::UnboxObjectValue(&value, MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::ResourceDictionary), __uuidof(xaml::IResourceDictionary), reinterpret_cast<void**>(pValue)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::put_ResourcesImpl(_In_ xaml::IResourceDictionary* pResourceDictionary)
{
    HRESULT hr = S_OK;
    CValue value;
    BoxerBuffer buffer;
    DependencyObject* pMOR = NULL;

    // put_Resources can only be called from a UI thread where we have initialized DXamlCore.
    IFC(DXamlServices::IsDXamlCoreInitialized() ? S_OK : RPC_E_WRONG_THREAD);

    IFC(CValueBoxer::BoxObjectValue(&value, /* pSourceType */ NULL, pResourceDictionary, &buffer, &pMOR));

    // See the note in get_Resources to explain the use of DXamlCore::GetCurrent()->GetCoreAppHandle().
    IFC(CoreImports::DependencyObject_SetValue(
        DXamlCore::GetCurrent()->GetCoreAppHandle(),
        SetValueParams(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Application_Resources), value)));

Cleanup:
    ctl::release_interface(pMOR);
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::get_DebugSettingsImpl(_Outptr_ xaml::IDebugSettings **pValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_pDebugSettings);
    IFC(ctl::do_query_interface(*pValue, m_pDebugSettings));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::OnActivatedImpl(_In_ waa::IActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::DispatchGenericActivation(_In_ IInspectable* args)
{
    HRESULT hr = S_OK;
    waa::IActivatedEventArgs* pActivatedEventArgs = NULL;

    if (args)
    {
        IFC(args->QueryInterface(__uuidof(waa::IActivatedEventArgs), reinterpret_cast<void**>(&pActivatedEventArgs)));
    }

    IFC(OnActivatedProtected(pActivatedEventArgs));

Cleanup:
    ReleaseInterface(pActivatedEventArgs);

    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::OnLaunchedImpl(_In_ xaml::ILaunchActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnFileActivatedImpl(_In_ waa::IFileActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnSearchActivatedImpl(_In_ waa::ISearchActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnShareTargetActivatedImpl(_In_ waa::IShareTargetActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnFileOpenPickerActivatedImpl(_In_ waa::IFileOpenPickerActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnFileSavePickerActivatedImpl(_In_ waa::IFileSavePickerActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnCachedFileUpdaterActivatedImpl(_In_ waa::ICachedFileUpdaterActivatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnWindowCreatedImpl(_In_ xaml::IWindowCreatedEventArgs* args)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::OnBackgroundActivatedImpl(_In_ waa::IBackgroundActivatedEventArgs* pArgs)
{
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::InvokeOnWindowCreated()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<WindowCreatedEventArgs> spArgs;
    DXamlCore* pCore = NULL;

    pCore = DXamlCore::GetCurrent();
    IFCCATASTROPHIC(pCore);

    IFC(ctl::ComObject<WindowCreatedEventArgs>::CreateInstance<WindowCreatedEventArgs>(&spArgs));

    // InvokeOnWindowCreated is called by FrameworkView::SetWindow which only occurs in UWP mode.
    IFC(spArgs->put_Window(pCore->GetUwpWindowNoRef()));

    hr = OnWindowCreatedProtected(spArgs.Get());
    if (FAILED(hr))
    {
        IGNOREHR(ErrorHelper::ReportUnhandledError(hr));
    }
    IFC(hr);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::InvokeOnLaunchActivated(
    _In_ waa::ILaunchActivatedEventArgs* uwpLaunchActivatedEventArgs)
{
    wrl_wrappers::HString arguments;
    ctl::ComPtr<LaunchActivatedEventArgs> launchActivatedEventArgs;

    IFC_RETURN(uwpLaunchActivatedEventArgs->get_Arguments(arguments.GetAddressOf()));
    IFC_RETURN(ctl::ComObject<LaunchActivatedEventArgs>::CreateInstance<LaunchActivatedEventArgs>(&launchActivatedEventArgs));
    IFC_RETURN(launchActivatedEventArgs->put_Arguments(arguments));
    IFC_RETURN(launchActivatedEventArgs->put_UWPLaunchActivatedEventArgs(uwpLaunchActivatedEventArgs));

    // Invoke the application's custom Application.OnLaunched method
    HRESULT hr = FrameworkApplication::GetCurrentNoRef()->OnLaunchedProtected(launchActivatedEventArgs.Get());
    if (FAILED(hr))
    {
        IGNOREHR(ErrorHelper::ReportUnhandledError(hr));
    }

    IFC_RETURN(hr);

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::RaiseUnhandledExceptionEvent(_In_ HRESULT hrToReport, _In_opt_ HSTRING hstrMessage, _Inout_ bool* pfHandled)
{
    HRESULT hr = S_OK;
    DirectUI::UnhandledExceptionEventArgs* pEventArgs = NULL;
    ctl::ComPtr<DirectUI::UnhandledExceptionEventArgs> spEventArgs;
    BOOLEAN isHandled = FALSE;

    // This can be called on a non-UI thread. Don't use ctl::make or
    // TypeInfo::CreateTypedDependencyObject here (both of those do thread checking).
    IFC(ctl::ComObject<DirectUI::UnhandledExceptionEventArgs>::CreateInstance(&pEventArgs));
    spEventArgs.Attach(pEventArgs);
    pEventArgs = NULL;

    IFC_NOTRACE(spEventArgs->put_Handled(!!*pfHandled));
    IFC_NOTRACE(spEventArgs->put_Exception(hrToReport));
    IFC_NOTRACE(spEventArgs->put_Message(hstrMessage));

    IFC_NOTRACE(m_UnhandledExceptionEventSource.Raise(this, spEventArgs.Get()));

    IFC_NOTRACE(spEventArgs->get_Handled(&isHandled));
    *pfHandled = !!isHandled;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT 
FrameworkApplication::RaiseResourceManagerRequestedEvent(
    _Out_ wrl::ComPtr<mwar::IResourceManager>& resourceManager)
{
    ctl::ComPtr<DirectUI::ResourceManagerRequestedEventArgs> eventArgs;
    IFC_RETURN(ctl::make<DirectUI::ResourceManagerRequestedEventArgs>(&eventArgs));

    IFC_RETURN(m_resourceManagerRequestedEventSource.Raise(this, eventArgs.Get()));

    IFC_RETURN(eventArgs->get_CustomResourceManager(&resourceManager));

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::ExitImpl()
{
    AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;
    LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy);
    if (status != ERROR_SUCCESS)
    {
        IFC_RETURN(E_FAIL);
    }

    if (policy == AppPolicyWindowingModel_ClassicDesktop)
    {
        if (DXamlCore* dxamlCore = DirectUI::DXamlCore::GetCurrent())
        {
            const bool anyWindowLeftToClose = !dxamlCore->m_handleToDesktopWindowMap.empty();
            if (anyWindowLeftToClose)
            {
                // Close all windows.
                for (auto const& [hwnd, window] : dxamlCore->m_handleToDesktopWindowMap)
                {
                    window->Close();
                }
            }
            
            // If there were no Windows, PostQuitMessage didn't called, so call it now.
            // If DispatcherShutdownMode was OnExplicitShutdown, PostQuitMessage() didn't get called either.
            // In both cases, call PostQuitMessage() here so that Exit() reliably exits the event loop.
            if (!anyWindowLeftToClose || dxamlCore->GetDispatcherShutdownMode() == xaml::DispatcherShutdownMode_OnExplicitShutdown)
            {
                ::PostQuitMessage(0);
            }
        }
    }
    else
    {
        ctl::ComPtr<wac::ICoreApplicationExit> coreApplicationExit;

        IFC_RETURN(GetCoreApplicationExit(&coreApplicationExit));
        IFC_RETURN(coreApplicationExit->Exit());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: FrameworkApplication::put_RequestedThemeImpl
//
//  Synopsis:
//     RequestedTheme setter. Can be set only before app.xaml is loaded,
//  because Default Themes currently cannot be changed on the fly.
//------------------------------------------------------------------------

_Check_return_ HRESULT FrameworkApplication::put_RequestedThemeImpl(
    _In_ xaml::ApplicationTheme value)
{
    // RequestedTheme cannot be set after app.xaml has been loaded.
    if (!m_isRequestedThemeSettable)
    {
        IFC_RETURN(E_NOT_SUPPORTED);
    }

    // The implementation of ApplicationTheme is very messy.
    // ApplicationTheme is supposed to have global semantics, but the
    // differences between FrameworkApplication and CApplication are making it
    // hard to do this consistently. There's one single FrameworkApplication
    // object, but there can be multiple CCoreServices (e.g. secondary views),
    // each tied to its own CApplication object. XAML developers are familiar
    // with the App class which normally gets autogenerated by Visual Studio.
    // This class is tricky; it technically corresponds to the
    // FrameworkApplication singleton, but its markup actually gets loaded onto
    // CApplications. If we create a secondary view, for example, we new up a
    // CApplication and load the markup of the App class onto it; I suspect we
    // do this to reload the resources defined in that markup. However, this
    // causes an unintuitive side-effect: The properties set on the App via
    // markup will be set on the CApplication when we load it, which arguably
    // makes sense, but if the properties are set in the constructor instead,
    // these won't make it to the CApplication because the constructor actually
    // corresponds to the FrameworkApplication singleton, which is obviously
    // never called again. As you can tell by the comment below, in the case of
    // RequestedTheme, we're explicitly bypassing CApplication::SetValue, so
    // the new value never gets to the CApplication object except through the
    // parser. FrameworkApplication::put_RequestedThemeImpl simply passes the
    // value to the m_spTheming instance of the current CCoreServices. This has
    // a couple of implications:
    // 1. Setting the ApplicationTheme only impacts the current view (e.g. the
    //    current CCoreServices instance), stomping on the idea that this is a
    //    global value. Yet, since we're blocking the ApplicationTheme from
    //    being settable with m_isRequestedThemeSettable though, you could say
    //    that the implications of this are relatively controlled for now.
    // 2. The value of the ApplicationTheme is never stored in
    //    FrameworkApplication; instead, each CCoreServices has its own copy.
    //    In other words, this value with global semantics is being treated
    //    as a local value. Since there's no actual global value stored, unless
    //    there's a policy to guarantee that the local values are synchronized,
    //    it is possible to lose the actual desired value beyond recovery.
    //    Spoilers: This is actually what was happening and the reason why I
    //    added m_applicationRequestedTheme as a member of the
    //    FrameworkApplication singleton. (See https://task.ms/13616368)
    m_applicationRequestedTheme = value == xaml::ApplicationTheme_Light
        ? Theming::Theme::Light
        : Theming::Theme::Dark;

    // Note: This is special, we bypass the CApplication::SetValue so this property never actually
    // get's set on the core object. Just an annoying nuance because our parser set's this by going
    // through CApplication::SetValue. See comment above for more info.
    IFC_RETURN(DXamlCore::GetCurrent()->GetHandle()->GetFrameworkTheming()->SetRequestedTheme(value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: FrameworkApplication::get_RequestedThemeImpl
//
//  Synopsis:
//     RequestedTheme getter. Bypasses the core application object and get's
//   it directly from the framework theming object.
//------------------------------------------------------------------------

_Check_return_ HRESULT FrameworkApplication::get_RequestedThemeImpl(
    _Out_ xaml::ApplicationTheme* pValue)
{
    auto theme = DXamlCore::GetCurrent()->GetHandle()->GetFrameworkTheming()->GetBaseTheme();
    *pValue = (theme == Theming::Theme::Light ? xaml::ApplicationTheme_Light : xaml::ApplicationTheme_Dark);

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Method: FrameworkApplication::put_RequiresPointerModeImpl
//
//  Synopsis:
//     RequiresPointerMode setter. Can be set only before app.xaml is loaded,
//     because it cannot be changed on the fly since we would ignore it if it did.
//------------------------------------------------------------------------

_Check_return_ HRESULT FrameworkApplication::put_RequiresPointerModeImpl(
    _In_ xaml::ApplicationRequiresPointerMode value)
{
    // RequiresPointerMode cannot be set after app.xaml has been loaded.
    if (!m_isRequiresPointerModeSettable)
    {
        IFC_RETURN(E_NOT_SUPPORTED);
    }

    m_requiresPointerMode = value;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: FrameworkApplication::get_RequiresPointerModeImpl
//
//  Synopsis:
//     RequiresPointerMode getter.
//------------------------------------------------------------------------

_Check_return_ HRESULT FrameworkApplication::get_RequiresPointerModeImpl(
    _Out_ xaml::ApplicationRequiresPointerMode* pValue)
{
    *pValue = m_requiresPointerMode;
    RRETURN(S_OK);
}

_Check_return_ HRESULT FrameworkApplication::GetPLMHandlerForCallingThread(_Outptr_ XAML::PLM::PLMHandler** ppHandler)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    if (pCore)
    {
        *ppHandler = pCore->GetPLMHandler();
        goto Cleanup;
    }

    {
        CApplicationLock lock;

        if (!m_pPLMHandlerForMTA)
        {
            IFC(XAML::PLM::PLMHandler::CreateForMTA(NULL, &m_pPLMHandlerForMTA));
        }

        *ppHandler = m_pPLMHandlerForMTA;
    }

Cleanup:
    RRETURN(hr);
}

// Returns the xaml::Application::FocusVisualKind property value for the current FrameworkApplication instance.
FocusVisualKind FrameworkApplication::GetFocusVisualKind()
{
    xaml::FocusVisualKind currentFocusVisualKind = xaml::FocusVisualKind_HighVisibility;

    if (auto app = FrameworkApplication::GetCurrentNoRef())
    {
        VERIFYHR(app->get_FocusVisualKind(&currentFocusVisualKind));
    }

    return static_cast<FocusVisualKind>(currentFocusVisualKind);
}

Theming::Theme FrameworkApplication::GetApplicationRequestedTheme()
{
    Theming::Theme currentApplicationRequestedTheme = Theming::Theme::None;

    if (auto app = FrameworkApplication::GetCurrentNoRef())
    {
        currentApplicationRequestedTheme = app->m_applicationRequestedTheme;
    }

    return currentApplicationRequestedTheme;
}

// Returns the xaml::Application::ApplicationHighContrastAdjustment property value for the current FrameworkApplication instance.
_Check_return_ HRESULT FrameworkApplication::GetApplicationHighContrastAdjustment(
    _Out_ ApplicationHighContrastAdjustment* pApplicationHighContrastAdjustment)
{
    xaml::ApplicationHighContrastAdjustment currentApplicationHighContrastAdjustment = xaml::ApplicationHighContrastAdjustment_None;

    if (auto app = FrameworkApplication::GetCurrentNoRef())
    {
        IFC_RETURN(app->get_HighContrastAdjustmentImpl(&currentApplicationHighContrastAdjustment));
    }

    *pApplicationHighContrastAdjustment = static_cast<ApplicationHighContrastAdjustment>(currentApplicationHighContrastAdjustment);

    return S_OK;
}

// Returns the FrameworkApplication::FocusVisualKind property value.
_Check_return_ HRESULT FrameworkApplication::get_FocusVisualKindImpl(_Out_ xaml::FocusVisualKind* pValue)
{
    *pValue = m_focusVisualKind;
    return S_OK;
}

// Sets the FrameworkApplication::FocusVisualKind property value.
_Check_return_ HRESULT FrameworkApplication::put_FocusVisualKindImpl(_In_ xaml::FocusVisualKind value)
{
    if (m_focusVisualKind != value)
    {
        m_focusVisualKind = value;

        auto dxamlCore = DXamlCore::GetCurrent();
        if (dxamlCore) // May be null during initialization of XamlIslandRoot in some legacy test scenarios.
        {
            bool isReveal = m_focusVisualKind == xaml::FocusVisualKind::FocusVisualKind_Reveal;
            IFC_RETURN(dxamlCore->GetHandle()->SetFocusVisualKindIsReveal(isReveal));

            dxamlCore->OnApplicationFocusVisualKindChanged();
        }
    }

    return S_OK;
}

// Returns the FrameworkApplication::HighContrastAdjustment property value.
_Check_return_ HRESULT FrameworkApplication::get_HighContrastAdjustmentImpl(_Out_ xaml::ApplicationHighContrastAdjustment* pValue)
{
    *pValue = m_highContrastAdjustment;
    return S_OK;
}

// Sets the FrameworkApplication::HighContrastAdjustment property value.
_Check_return_ HRESULT FrameworkApplication::put_HighContrastAdjustmentImpl(_In_ xaml::ApplicationHighContrastAdjustment value)
{
    if (m_highContrastAdjustment != value)
    {
        m_highContrastAdjustment = value;
        IFC_RETURN(DXamlCore::GetCurrent()->OnApplicationHighContrastAdjustmentChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::get_WindowsImpl(_Outptr_result_maybenull_ wfc::IVectorView<xaml::Window*>** ppValue)
{
    // TODO: next iteration will remove double-vector creation- see TODO note in DXamlCore::GetAllWindows
    std::vector<DirectUI::Window*> windowVector;
    IFC_RETURN(DXamlCore::GetCurrent()->GetAllWindows(windowVector));

    ctl::ComPtr<TrackerCollection<xaml::Window*>> windowCollection;
    IFC_RETURN(ctl::make(&windowCollection));
    for (auto window : windowVector)
    {
        // DirectUI::Window is implicitly cast to xaml::Window*
        IFC_RETURN(windowCollection->Append(window));
    }

    IFC_RETURN(windowCollection->GetView(ppValue));
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::get_ShutdownModelImpl(_Out_ xaml::ShutdownModel* value)
{
    *value = m_shutdownModel;
    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::put_ShutdownModelImpl(_In_ xaml::ShutdownModel value)
{
    switch (value)
    {
        case xaml::ShutdownModel_Version1:
        case xaml::ShutdownModel_Version2:
            m_shutdownModel = value;
            return S_OK;
    }
    IFC_RETURN(E_INVALIDARG);
}

        
_Check_return_ HRESULT FrameworkApplication::get_DispatcherShutdownModeImpl(_Out_ xaml::DispatcherShutdownMode* value)
{
    auto dxamlCore = DXamlCore::GetCurrent();
    if (!dxamlCore)
    {
        IFC_RETURN(RPC_E_WRONG_THREAD);
    }

    *value = dxamlCore->GetDispatcherShutdownMode();

    return S_OK;
}


_Check_return_ HRESULT FrameworkApplication::put_DispatcherShutdownModeImpl(_In_ xaml::DispatcherShutdownMode value)
{
    auto dxamlCore = DXamlCore::GetCurrent();
    if (!dxamlCore)
    {
        IFC_RETURN(RPC_E_WRONG_THREAD);
    }

    dxamlCore->SetDispatcherShutdownMode(value);

    return S_OK;
}


_Check_return_ HRESULT FrameworkApplication::DispatchBackgroundActivated(
    _In_ IInspectable* pComponent,
    _In_ waa::IBackgroundActivatedEventArgs* pArgs)
{
    IFC_RETURN(OnBackgroundActivatedProtected(pArgs));

    return S_OK;
}

_Check_return_ HRESULT FrameworkApplication::HookBackgroundActivationEvents(bool fRegister)
{
    HRESULT hr = S_OK;

    wrl_wrappers::HStringReference coreApplicationAcid(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);
    ctl::ComPtr<wac::ICoreApplication2> spCoreApplication2 = NULL;
    IFC(ctl::GetActivationFactory(coreApplicationAcid.Get(), &spCoreApplication2));

    if (fRegister)
    {
        IFC(spCoreApplication2->add_BackgroundActivated(
            Microsoft::WRL::Callback<wf::IEventHandler<waa::BackgroundActivatedEventArgs*>>(
                this, &FrameworkApplication::DispatchBackgroundActivated).Get(),
            &m_BackgroundActivatedEventToken));
    }
    else if (m_BackgroundActivatedEventToken.value)
    {
        IFC(spCoreApplication2->remove_BackgroundActivated(m_BackgroundActivatedEventToken));
        m_BackgroundActivatedEventToken.value = 0;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FrameworkApplication::SetSynchronizationWindowImpl(UINT64 commitResizeWindow)
{
    return E_NOTIMPL;
}

void FrameworkApplication::RunDesktopWindowMessageLoop()
{
    // Start the message loop.
    MSG msg = {};
    BOOL bRet = FALSE;

    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            // If there is an error, the return value is -1. For example, the function
            // fails if hWnd is an invalid window handle (such as a window handle for
            // a window that has already been destroyed) or if lpMsg is an invalid pointer.

            // A GetMessage failure is a non-recoverable error.
            IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
        }
        else
        {
            if (!ContentPreTranslateMessage(&msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}
