// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CoreWindow.h>
#include "UserControl.g.h"
#include "Control.g.h"
#include "TextBoxView.g.h"
#include "TextBox.g.h"
#include "PasswordBox.g.h"
#include "RichEditBox.g.h"
#include "ItemsControl.g.h"
#include "ItemsPresenter.g.h"
#include "StaggerFunctionBase.g.h"
#include "Transition.g.h"
#include "ItemContainerGenerator.g.h"
#include "ContentPresenter.g.h"
#include "ContentControl.g.h"
#include "AutomationPeer.g.h"
#include "TextElement.g.h"
#include "Window.g.h"
#include "CommandBarElementCollection.g.h"
#include "HubSectionCollection.g.h"
#include "MenuFlyoutItemBaseCollection.g.h"
#include "FrameworkElement.g.h"
#include "VectorChangedEventArgs.g.h"
#include "FrameworkApplication.g.h"
#include "ToolTipService.g.h"
#include "Style.g.h"
#include "Hyperlink.g.h"
#include "TimelineCollection.g.h"
#include "Timeline.g.h"
#include "DataContextChangedEventArgs.g.h"
#include "IconElement.g.h"
#include "CommandBar.g.h"
#include "SwapChainPanel.g.h"
#include "DragDropInternal.h"
#include "ApplicationBarService.g.h"
#include "Popup.g.h"
#include "Hub.g.h"
#include "TypeTable.g.h"
#include "Image.g.h"
#include "Page.g.h"
#include "NodeStreamCache.h"
#include <DependencyLocator.h>
#include <ParserAPI.h>
#include <FrameworkTheming.h>
#include <CStaticLock.h>
#include "DragEventArgs.g.h"
#include "DesktopUtility.h"
#include "FlyoutBase.g.h"
#include "MenuFlyout.g.h"
#include "FlyoutBase_partial.h"
#include "MenuFlyoutPresenter_Partial.h"
#include <WindowsGraphicsDeviceManager.h>
#include "ThreadPoolService.h"
#include <FrameworkUdk/BackButtonIntegration.h>
#include "TouchHitTestingHandler.h"
#include "DCompSurfaceFactoryManager.h"
#include "DCompSurfaceMonitor.h"
#include "XamlParserCallbacks.h"
#include <ImageReloadManager.h>
#include "JupiterControl.h"
#include "TemplateBindingExpression.h"
#include "ThemeResourceExpression.h"
#include "ISupportInitialize.g.h"
#include "InternalDebugInteropModel.h"
#include <FrameworkUdk/DebugTool.h>
#include "ResourceDictionary_partial.h"
#include "DefaultStyles.h"
#include "DXamlInstanceStorage.h"
#include "StaticStore.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <application.h>
#include <DiagnosticsInterop.h>
#include <windows.ui.viewmanagement.h>
#include <ShellScalingApi.h>
#include "theming\inc\Theme.h"
#include "CustomWriterRuntimeContext.h"
#include "ExternalObjectReference_Partial.h"
#include <XamlOneCoreTransforms.h>
#include <WRLHelper.h>
#include "WindowRenderTarget.h"
#include "WindowsPresentTarget.h"
#include "DCompTreeHost.h"
#include "xcperrorresource.h"
#include "Binding.g.h"
#include "BindingExpression.g.h"
#include "PropertyPath.g.h"
#include "Callback.h"
#include "DxamlCoreTestHooks.g.h"
#include "NullKeyedResource.g.h"
#include "BuildTreeService.g.h"
#include "BudgetManager.g.h"
#include "FlyoutMetadata.h"
#include "XcpAllocationDebug.h"
#include "DebugSettings_Partial.h"
#include <windowscollections.h>
#include "RootScale.h"
#include "XamlRoot.g.h"
#include <windows.graphics.display.h>
#include <switcher.h>
#include <FrameworkUdk/VisualDiagnosticsPort.h>
#include <TextInputProducerHelper.h>
#include "UIAWrapper.h"
#include "ElementSoundPlayerService_Partial.h"
#include "ResourceGraph.h"
#include "AutomaticDragHelper.h"
#include "TextControlFlyoutHelper.h"

#include "DXamlCoreTipTests.h"

using namespace WRLHelper;
using namespace DirectUI;
using namespace DirectUISynonyms;

extern HINSTANCE g_hInstance;

// The platform-specific resource module handle - see dllentry.cpp
extern HMODULE g_platformResourcesModuleHandle;
extern void EnsurePlatformResourceModuleHandle();

// Total core instances
std::atomic<DWORD> DXamlCore::s_instanceCount = 1;

HANDLE DXamlCore::s_diagnosticToolingEvent = nullptr;
bool DXamlCore::s_enableNotifyEndOfReferenceTrackingOnThread = true;
bool DXamlCore::s_dynamicScrollbars = true;
bool DXamlCore::s_dynamicScrollbarsDirty = true;

bool TryPegManagedPeer(_In_ CDependencyObject* element, _In_ bool isShutdownException)
{
    ctl::ComPtr<DependencyObject> spDO;

    if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer(element, &spDO)))
    {
        if (spDO)
        {
            spDO->UpdatePegWithPossibleShutdownException(true, isShutdownException);
            return true;
        }
    }

    return false;
}

void TryUnpegManagedPeer(_In_ CDependencyObject* element, _In_ bool isShutdownException)
{
    ctl::ComPtr<DependencyObject> spDO;

    if (SUCCEEDED(DXamlCore::GetCurrent()->TryGetPeer(element, &spDO)))
    {
        if (spDO)
        {
            spDO->UpdatePegWithPossibleShutdownException(false, isShutdownException);
        }
    }
}

DXamlCore::DXamlCore()
:
#ifdef MUX_PRERELEASE
     m_shouldCheckGenericXamlFilePathFromMUX(true)
#else
     m_shouldCheckGenericXamlFilePathFromMUX(false)
#endif
{
    XCP_STRONG(&m_pDispatcher);
}

DXamlCore::~DXamlCore()
{
    delete m_pDefaultStyles;
    m_pDefaultStyles = NULL;

    ReleaseInterface(m_pControl);

    // Release PageNavigation complete event, if exists
    if (m_pPageNavigationCompleteEvent)
    {
        m_pPageNavigationCompleteEvent->Close();
        m_pPageNavigationCompleteEvent = NULL;
    }

    RemoveAutoHideScrollBarsChangedHandler();
    RemoveAnimationsEnabledChangedHandler();
}

_Check_return_ HRESULT DXamlCore::UpdateAnimationsEnabled()
{
    ctl::ComPtr<wuv::IUISettings> uiSettings;
    IFC_RETURN(GetUISettings(uiSettings));

    BOOLEAN isAnimationsEnabled = FALSE;

    IFC_RETURN(uiSettings->get_AnimationsEnabled(&isAnimationsEnabled));
    m_isAnimationEnabled = !!isAnimationsEnabled;

    return S_OK;
}

#if DBG
void WaitForDebugger(BOOL bInitialBreak)
{
    BOOL bDebuggerPresent = FALSE;
    while (!bDebuggerPresent)
    {
        bDebuggerPresent = IsDebuggerPresent();
        if (!bDebuggerPresent)
        {
            Sleep(1000);
        }
    }

    if (bInitialBreak)
    {
        DebugBreak();
    }
}

void WaitForDebuggerIfNeeded()
{
    HKEY hKey = NULL;

    DWORD dwWaitType = 0;
    DWORD dwWaitData = 0;
    DWORD cbWaitData = sizeof(dwWaitData);

    DWORD dwBreakType = 0;
    DWORD dwBreakData = 0;
    DWORD cbBreakData = sizeof(dwBreakData);

    LONG result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        XAML_ROOT_KEY L"\\Debug",
        0,
        KEY_QUERY_VALUE,
        &hKey);

    if (ERROR_SUCCESS == result)
    {
        result = RegQueryValueEx(
            hKey,
            L"WaitForDebugger",
            NULL,
            &dwWaitType,
            (LPBYTE) &dwWaitData,
            &cbWaitData);
    }

    if (ERROR_SUCCESS == result && REG_DWORD == dwWaitType && 1 == dwWaitData)
    {
        BOOL bInitialBreak = FALSE;

        result = RegQueryValueEx(
            hKey,
            L"InitialBreak",
            NULL,
            &dwBreakType,
            (LPBYTE) &dwBreakData,
            &cbBreakData);

        if (ERROR_SUCCESS == result && REG_DWORD == dwBreakType && 1 == dwBreakData)
        {
            bInitialBreak = TRUE;
        }

        WaitForDebugger(bInitialBreak);
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }
}
#endif // DBG

Window*
DXamlCore::GetUwpWindowNoRef()
{
    // this function must not be called outside of UWP.
    AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;
    LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy);
    if (status == ERROR_SUCCESS)
    {
        ASSERT(policy == AppPolicyWindowingModel_Universal);
        return m_uwpWindowNoRef;
    }

    return nullptr;
}

_Check_return_ HRESULT
DXamlCore::GetAssociatedWindowNoRef(
    _In_ UIElement* element,
    _Outptr_result_maybenull_  Window** windowNoRef)
{
    IFCPTR_RETURN(windowNoRef);
    if (element == nullptr)
    {
        return E_INVALIDARG;
    }
    *windowNoRef = nullptr;

    // In the short term, we will allow UWP apps to use this function in much the same way they use GetWindow().
    // This logic should be updated when MultiWindow support is enabled for UWP apps. At which time
    // retrieving the AppPolicyWindowingModel ideally would not be necessary.
    const AppPolicyWindowingModel policy = FrameworkApplication::GetCurrentNoRef()->GetAppPolicyWindowingModel();
    if (policy == AppPolicyWindowingModel_Universal)
    {
        *windowNoRef = m_uwpWindowNoRef;
        return S_OK;
    }

    // Determine the element's XamlRoot and retrieve the HostWindow property from that root.
    // Note that in a pure islands scenario, it is possible that a UIElement will not have a XamlRoot
    // if the UIElement was created in code behind and not assigned a XamlRoot. In this case, this
    // function cannot proceed but should not throw an exception.
    ctl::ComPtr<XamlRoot> xamlRoot = XamlRoot::GetImplementationForElementStatic(element);
    if (!xamlRoot)
    {
        return S_FALSE;
    }

    // Retrieve the hosting HWND from the XamlRoot.
    // As above, it is possible that the element is hosted inside of a XamlIsland, which means that
    // it will have a XamlRoot but not a hosting HWND. This is essentially equivalent to a UIElement
    // being created in code behind, as a XamlIsland is used to host Xaml content inside of an app
    // without using a Xaml window. In this case, this function cannot proceed but should not throw
    // an exception as the UIElement will not have an associated Xaml window.
    HWND xamlHwnd;
    IFC_RETURN(xamlRoot->get_HostWindow(&xamlHwnd));
    if (!xamlHwnd)
    {
        return S_FALSE;
    }

    // In a desktop context, the HostWindow actually refers to the DesktopWindowXamlSource window where the xaml lives.
    HWND parentHwnd = ::GetParent(xamlHwnd);
    if (parentHwnd == NULL)
    {
        return S_OK;
    }

    // use top-level HWND to identify the DirectUI::Window instance associated with it.
    auto windowEntry = m_handleToDesktopWindowMap.find(parentHwnd);
    if (windowEntry != m_handleToDesktopWindowMap.end())
    {
        *windowNoRef = windowEntry->second;
    }
    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::GetAllWindows(
    _Out_ std::vector<Window*>&  windowsNoRef)
{
    windowsNoRef.clear();

    // TODO: This function cannot fail. Function signature should be changed to
    // const std::vector<Window*>& DXamlCore::GetAllWindows and maintain this vector internally.
    // https://microsoft.visualstudio.com/OS/_workitems/edit/37010942
    const AppPolicyWindowingModel policy = FrameworkApplication::GetCurrentNoRef()->GetAppPolicyWindowingModel();
    if (policy == AppPolicyWindowingModel_Universal)
    {
        windowsNoRef.push_back(m_uwpWindowNoRef);
    }
    else
    {
        for (const auto& pair : m_handleToDesktopWindowMap)
        {
            windowsNoRef.push_back(pair.second);
        }
    }

    return S_OK;
}

bool DXamlCore::HasDesktopWindow() const
{
    return !m_handleToDesktopWindowMap.empty();
}

BOOLEAN
DXamlCore::IsFinalReleaseQueueEmpty()
{
    return m_spReleaseQueue == NULL || m_spReleaseQueue->IsEmpty();
}

HRESULT DXamlCore::GetFinalReleaseQueue(_Outptr_ wfc::IVectorView<IInspectable*>** queueView)
{
    wrl::ComPtr<wfci_::Vector<IInspectable*>> vector;
    IFC_RETURN(wfci_::Vector<IInspectable*>::Make(&vector));
    wrl::ComPtr<wfc::IVector<IInspectable*>> queue;
    IFC_RETURN(vector.As(&queue));
    if (m_spReleaseQueue!=nullptr)
    {
        IFC_RETURN(m_spReleaseQueue->GetQueueObjects(queue.Get()));
    }
    IFC_RETURN(queue->GetView(queueView));
    return S_OK;
}

void DXamlCore::DeleteDragDrop()
{
    delete m_pDragDrop;
    m_pDragDrop = NULL;
}

//
// This is the normal xaml runtime initialization code used by designer, shell, immersive apps.
//
// NOTE:
// If you are modifying this method, please consider if it needs to be added to the
// InitializeInstanceForXbf() method for the XBF core initialization path.
//
_Check_return_ HRESULT DXamlCore::InitializeInstance(_In_ InitializationType initializationType)
{
    HRESULT hr = S_OK;
    CCoreServices* pCore = NULL;
    SuspendingEventHandler* pSuspendingEventHandler = NULL;
    wf::IEventHandler<IInspectable*>* pResumingEventHandler = NULL;

    #if DBG
    WaitForDebuggerIfNeeded();
    #endif

    TraceInitializeCoreBegin();

    // Start TIP test
    auto initDxamlCoreTest = tip::start_and_watch_errors<DXamlInitializeCoreTest>();

    // Log the mux version
    initDxamlCoreTest->muxVersion = TipTestHelper::GetMuxVersion();

    m_state = DXamlCore::Initializing;

    // Initialize the lock used to access the peer table.
    InitializeSRWLock(&m_peerReferenceLock);

    // Take a reference on the FinalUnhandledErrorDetected event registration.
    // This reference will be released when this DXamlCore instance is deinitialized.
    IFC(ErrorHelper::GetFinalUnhandledErrorDetectedRegistration()->AddRefRegistration());

    // Create a PLM handler for this core, if this process is packaged -- and if we're not using
    // "IslandsOnly" initialization, which means we're running in a win32 context.
    if (gps->IsProcessPackaged() && (initializationType != InitializationType::IslandsOnly))
    {
        initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::packaged_process));
        IFC(XAML::PLM::PLMHandler::CreateForASTA(this, &m_pPLMHandler));
    }

    if (initializationType != InitializationType::IslandsOnly)
    {
        initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::init_type_uwp));
        // When running in UWP, we must make sure there's a DispatcherQueueController on the thread, since XAML
        // requires one to be running.  Since we don't support UWP, we don't bother to shutdown the DQC, this is
        // just to keep XAML tests running.
        wrl::ComPtr<msy::IDispatcherQueueControllerStatics> dispatcherQueueControllerStatics;

        IFCFAILFAST(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueueController).Get(),
            &dispatcherQueueControllerStatics));
        IFCFAILFAST(dispatcherQueueControllerStatics->CreateOnCurrentThread(&m_dispatcherQueueController));
    }

    if (initializationType == InitializationType::MainView)
    {
        initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::init_type_main_view));
        // We allow this temporarily only for UWP because we're not supporting it for foward-compat yet.
        //  Task 29643834: Remove use of textinputproducerinternal.h before we open-source and before we fully-support UWP
        //                 (ITextInputConsumer, ITextInputProducer, ITextInputProducerInternal)
        TextInputProducerHelper::SetAllowCallsToPrivateWindowsFunctions(true);
    }

    XamlOneCoreTransforms::EnsureInitialized(XamlOneCoreTransforms::InitMode::Normal);

    IFC(CJupiterControl::Create(&m_pControl));

    pCore = m_pControl->GetCoreServices();
    IFCEXPECT(pCore);

    IFC(pCore->SetCurrentApplication(NULL));

    // Keep a reference from the core context to the DXamlCore
    IFC(CoreImports::SetFrameworkContext(pCore, this));
    ReplaceInterface(m_hCore, pCore);
    pCore = NULL;


    // TSF3 rely on CoreDispathcer. TextServicesManager checks for a CoreDispatcher to determine UIThread and fails in desktop/ island without CoreWindow, hence
    // in Island mode force to use TSF1
    if (initializationType == InitializationType::IslandsOnly)
    {
        initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::init_type_islands_only));
        m_hCore->ForceDisableTSF3();
        initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::disabled_tsf3));
    }

    // initialize the XAML dispatcher
    IFC(ctl::ComObject<DispatcherImpl>::CreateInstance(m_spDispatcherImpl.ReleaseAndGetAddressOf()));
    IFC(m_spDispatcherImpl->Connect(this));
    initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::initialized_dispatcher));

    // Disable the legacy IME since the legacy IMEs aren't designed for the immersive environment.
    //
    // We only want to do this when not in design mode, since the design-mode process should use the legacy IMEs.
    ImmDisableLegacyIME();

    m_hCore->SetInitializationType(initializationType);

    m_pDragDrop = NULL;

    m_staticStoreGuard = StaticStore::GetInstance();

    // Initialize the default control styles cache
    m_pDefaultStyles = new DefaultStyles();

    // Force the theming object to update system color brushes as well as
    // query initial system theme.
    IFC(m_hCore->GetFrameworkTheming()->OnThemeChanged());

    IFC(EnsureEventArgs());

    IFC(Window::Create(this, &m_uwpWindowNoRef));
    initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::created_uwp_window));

    // The Window needs to be pegged because it doesn't have an entry in the PeerTable,
    // and its members can potentially be GCed.
    m_uwpWindowNoRef->UpdatePeg(true);

    IFC(ctl::make<UIAffinityReleaseQueue>(&m_spReleaseQueue));

    // Initialize Visual Diagnostics
    IFC(RegisterVisualDiagnosticsPort());

    m_state = DXamlCore::Initialized;

    // Now that DXamlCore is initialized, listen to the UISettings' AnimationsEnabledChanged event for future updates
    // and immediately refresh the m_isAnimationEnabled field.
    IFC(AddAnimationsEnabledChangedHandler());
    // m_animationsEnabledChangedToken will be zero if AddAnimationsEnabledChangedHandler() fails to QI IUISettings6
    if (m_animationsEnabledChangedToken.value != 0)
    {
        IFC(UpdateAnimationsEnabled());
    }

Cleanup:
    if (FAILED(hr))
    {
        m_state = DXamlCore::InitializationFailed;
        initDxamlCoreTest.set_flag(TIP_reason(DXamlInitializeCoreTest::reason::failed_dxamlcore_init));
        if (m_uwpWindowNoRef != nullptr)
        {
            m_uwpWindowNoRef->SetDXamlCore(nullptr);
            m_uwpWindowNoRef->UpdatePeg(false);
        }
    }

    ReleaseInterface(pCore);
    ReleaseInterface(pSuspendingEventHandler);
    ReleaseInterface(pResumingEventHandler);

    TraceInitializeCoreEnd();

    // End Tip Test - if cleanup is called
    initDxamlCoreTest.complete();

    return hr;
}

msy::IDispatcherQueue* DXamlCore::GetDispatcherQueueNoRef()
{
    auto coreServices = GetHandle();
    if (coreServices)
    {
        auto hostSite = coreServices->GetHostSite();
        if (hostSite)
        {
            auto xcpDispatcher = hostSite->GetXcpDispatcher();
            if (xcpDispatcher)
            {
                return xcpDispatcher->GetDispatcherQueueNoRef();
            }
        }
    }

    return nullptr;
}


_Check_return_ HRESULT DXamlCore::GetDispatcherQueue(_Outptr_ msy::IDispatcherQueue** ppDispatcher)
{
    *ppDispatcher = GetDispatcherQueueNoRef();
    IFCEXPECT_RETURN(*ppDispatcher);
    AddRefInterface(*ppDispatcher);
    return S_OK;
}

//
// Initializes the DXamlCore for XBF Generator
//
// This is the minimum setup required for the generator to bootstrap and work.
//
// 1. Obtain Core
// 2. Setup Error service
// 3. Setup Framework callback
// 4. Initialize Metadata store
// 5. Register DXaml namespaces with parser
// 6. Initialize Object Release queue.
//
// NOTE: Do not add any additional code paths outside of these components for
//       initialization of the core for the XBF generator. Keep this code path free of
//       any hardware dependencies (graphics, touch etc.)
//
_Check_return_ HRESULT DXamlCore::InitializeInstanceForXbf()
{
    HRESULT hr = S_OK;
    CCoreServices* pCore = NULL;

    #if DBG
    WaitForDebuggerIfNeeded();
    #endif

    TraceInitializeCoreBegin();

    m_state = DXamlCore::Initializing;

    // Initialize the lock used to access the peer table.
    InitializeSRWLock(&m_peerReferenceLock);

    IFC(ErrorHelper::GetFinalUnhandledErrorDetectedRegistration()->AddRefRegistration());

    // Obtain Core Services
    IFC(ObtainCoreServices(gps.Get(), &pCore));

    IFCEXPECT(pCore);

    pCore->SetInitializationType(InitializationType::Xbf);

    // Initialize Error Service
    IFC(pCore->CreateErrorServiceForXbfGenerator());
    IFC(pCore->SetCurrentApplication(NULL));

    // Keep a reference from the core context to the DXamlCore
    IFC(CoreImports::SetFrameworkContext(pCore, this));
    m_hCore = pCore;

    m_staticStoreGuard = StaticStore::GetInstance();

    // Initialize the object release queue
    IFC(ctl::make<UIAffinityReleaseQueue>(&m_spReleaseQueue));

    m_state = DXamlCore::Initialized;

Cleanup:
    if (FAILED(hr))
    {
        m_state = DXamlCore::InitializationFailed;
    }

    TraceInitializeCoreEnd();

    return hr;
}

_Check_return_ HRESULT DXamlCore::EnsureCoreApplicationInitialized()
{
    HRESULT hr = S_OK;
    if (!m_pDOCoreApp)
    {
        FrameworkApplication* applicationNoRef = nullptr;
        IFC(CoreImports::CreateObjectByTypeIndex(
            GetHandle(),
            KnownTypeIndex::Application,
            &m_pDOCoreApp));

        IFC(CoreImports::Application_JupiterComplete(m_pDOCoreApp));

        // automatic loading of app.xaml
        applicationNoRef = FrameworkApplication::GetCurrentNoRef();
        xstring_ptr strAppXaml = applicationNoRef ? applicationNoRef->GetAppXamlPath() : xstring_ptr::EmptyString();

        if (ShouldTryLoadAppXaml(strAppXaml))
        {
            IFCPTR(applicationNoRef);
            applicationNoRef->SetRequestedThemeNotSettable();
            applicationNoRef->SetRequiresPointerModeNotSettable();

            // Load App.xaml
            IFC(FrameworkApplication::LoadComponent(
                ctl::iinspectable_cast(applicationNoRef),
                strAppXaml));
        }

        if (applicationNoRef)
        {
            ctl::ComPtr<xaml::IDebugSettings> debugSettings;
            IGNOREHR(applicationNoRef->get_DebugSettings(&debugSettings));
            if (debugSettings)
            {
                debugSettings.Cast<DebugSettings>()->OnThreadInitialized();
            }
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        // This is how errors encountered while parsing app.xaml are bubbled up to app code.
        IGNOREHR(ErrorHelper::ReportUnhandledError(hr));
    }

    return hr;
}

bool DXamlCore::ShouldTryLoadAppXaml(const xstring_ptr& appXamlLocation) const
{
    if (appXamlLocation.IsNullOrEmpty() || IsInBackgroundTask())
    {
        return false;
    }

    xref_ptr<IPALMemory> appXamlMemory;
    bool isBinaryXaml = false;
    // don't attempt to auto-load app.xaml if it doesn't exist
    return SUCCEEDED(CoreImports::CoreServices_TryGetApplicationResource(GetHandle(), appXamlLocation, appXamlMemory.ReleaseAndGetAddressOf(), &isBinaryXaml));
}

template<class T>
_Check_return_ HRESULT DXamlCoreGetCoreApplication(_Outptr_ T** ppCoreApp)
{
    wrl_wrappers::HStringReference coreApplicationAcid(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication);

    return wf::GetActivationFactory(coreApplicationAcid.Get(), ppCoreApp);
}

_Check_return_ HRESULT DXamlCore::add_Suspending(_In_ SuspendingEventHandler* pHandler, _Out_ EventRegistrationToken* pToken)
{
    HRESULT hr = S_OK;
    wac::ICoreApplication* pCoreApp = NULL;

    IFC(DXamlCoreGetCoreApplication(&pCoreApp));
    IFC(pCoreApp->add_Suspending(pHandler, pToken));

Cleanup:
    ReleaseInterface(pCoreApp);

    return hr;
}

_Check_return_ HRESULT DXamlCore::add_Resuming(_In_ wf::IEventHandler<IInspectable*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    HRESULT hr = S_OK;
    wac::ICoreApplication* pCoreApp = NULL;

    IFC(DXamlCoreGetCoreApplication(&pCoreApp));
    IFC(pCoreApp->add_Resuming(pHandler, pToken));

Cleanup:
    ReleaseInterface(pCoreApp);

    return hr;
}

_Check_return_ HRESULT DXamlCore::add_LeavingBackground(_In_ wf::IEventHandler<wa::LeavingBackgroundEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    ctl::ComPtr<wac::ICoreApplication2> spCoreApp;
    IFC_RETURN(DXamlCoreGetCoreApplication(spCoreApp.GetAddressOf()));

    IFC_RETURN(spCoreApp->add_LeavingBackground(pHandler, pToken));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::add_EnteredBackground(_In_ wf::IEventHandler<wa::EnteredBackgroundEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    ctl::ComPtr<wac::ICoreApplication2> spCoreApp;
    DXamlCoreGetCoreApplication(spCoreApp.GetAddressOf());

    IFC_RETURN(spCoreApp->add_EnteredBackground(pHandler, pToken));

    return S_OK;
}

void DXamlCore::ReleaseWindow()
{
    if (m_uwpWindowNoRef)
    {
        SignalWindowMutation(VisualMutationType::Remove);

        m_uwpWindowNoRef->put_Content(nullptr);
        m_uwpWindowNoRef->SetDXamlCore(nullptr);
        m_uwpWindowNoRef->UpdatePeg(false);

        Window* tempWindow = m_uwpWindowNoRef;
        {
            // Set the to NULL under the reference tracking lock, in case there's a simultaneous
            // GC trying to access it
            AutoReentrantReferenceLock lock(this);
            tempWindow = m_uwpWindowNoRef;

            m_uwpWindowNoRef = NULL;
        }
        ctl::release_interface(tempWindow);
    }
}

_Check_return_ HRESULT DXamlCore::CommonShutdown()
{
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    // Make sure we are in the right state. We should only be here if we are deinitializing or if we are going idle.
    IFCEXPECT_ASSERT_RETURN(m_state == DXamlCore::Deinitializing ||
        (m_state == DXamlCore::Initialized && runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown)));

    // Reset the CustomWriterRuntimeContext cache for this thread. We do this because
    // the context cache keeps objects alive so we want to make sure those are released
    // before the core is destroyed. Diagnostics doesn't necessarily have to be enabled
    // to build up this cache. It can be built when the diagnostics ETW provider is enabled.
    auto resourceGraph = ::Diagnostics::GetResourceGraph();
    if (resourceGraph)
    {
        resourceGraph->ClearCachedContext();
    }

    if (auto application = FrameworkApplication::GetCurrentNoRef())
    {
        ctl::ComPtr<xaml::IDebugSettings> debugSettings;
        IGNOREHR(application->get_DebugSettings(&debugSettings));
        if (debugSettings)
        {
            debugSettings.Cast<DebugSettings>()->OnThreadDeinitialized();
        }
    }

    // We should inform the debug tool that the core is being deinitialized so that it can
    // release any peers it still may be holding onto. It's possible it could be holding
    // onto roots so we want to release before we reset the visual tree
    if (RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics))
    {
        SignalWindowMutation(VisualMutationType::Remove);

        if (auto interop = ::Diagnostics::GetDiagnosticsInterop(false))
        {
            interop->OnCoreDeinitialized(m_threadId);
        }
    }
    else if (m_wpDebugInterop)
    {
        wrl::ComPtr<DebugTool::IInternalDebugInterop> spDebugInterop;
        IFC_RETURN(GetDebugInterop(&spDebugInterop));
        if (spDebugInterop && spDebugInterop->GetDebugToolNoRef())
        {
            spDebugInterop->GetDebugToolNoRef()->OnDeinitializeCore();
        }
        m_wpDebugInterop.Reset();
    }

    IFC_RETURN(ClearCaches());

    // Reset the visual tree. Only do so right now if we are fully shutting down.
    // The tree will have already been reset otherwise.
    if (m_pControl && m_state == DXamlCore::Deinitializing)
    {
        IFC_RETURN(m_pControl->ResetVisualTree());
    }

    if (m_spApplicationBarService)
    {
        m_spApplicationBarService->UpdatePeg(false);
        m_spApplicationBarService.Reset();
    }

    if (m_elementSoundPlayerService)
    {
        m_elementSoundPlayerService->UpdatePeg(false);
        m_elementSoundPlayerService.Reset();
    }

    if (m_spBudgetManager)
    {
        m_spBudgetManager->UpdatePeg(false);
        m_spBudgetManager.Reset();
    }

    if (m_spFlyoutMetadata)
    {
        m_spFlyoutMetadata->UpdatePeg(false);
        m_spFlyoutMetadata.Reset();
    }

    if (m_spBuildTreeService)
    {
        m_spBuildTreeService->UpdatePeg(false);
        m_spBuildTreeService.Reset();
    }

    m_spCachedNullKeyedResource.Reset();

    CCoreServices* coreServices = GetHandle();

    if (coreServices)
    {
        // Release the custom resource loader now, before calling ShutdownAllPeers.
        // ShutdownAllPeers will disconnect RCWs, which could invalidate the custom resource loader.
        coreServices->SetCustomResourceLoader(NULL);

        // Reset IsUsingGenericXamlFilePath flag to FALSE.
        coreServices->SetIsUsingGenericXamlFilePath(false);
    }

    // Disable the XAML dispatcher, so that no more messages are posted to it.
    // Only do so right now if we are fully shutting down.
    if (m_spDispatcherImpl && m_state == DXamlCore::Deinitializing)
    {
        m_spDispatcherImpl->Disconnect();
    }

    // And remove any messages that were already posted to the dispatcher.
    // We need to do this before ShutdownAllPeers, because the dispatcher keeps
    // refs on objects, and those objects are about to go into a bad state.
    if (m_pControl)
    {
        m_pControl->PurgeThreadMessages();
    }

    m_spDataContextChangedEventArgs.Reset();
    m_spVectorChangedEventArgs.Reset();

    IFC_RETURN(ShutdownAllPeers());

    // Some queue objects need a valid CCoreServices instance to release cleanly
    // Clear the queue before we shutdown our CCoreServices instance
    ReleaseQueuedObjects(TRUE /* bSync */);

    IFC_RETURN(DCompSurfaceMonitor::CleanupDCompSurfaceCollection());

    // Deinitialize simple property callbacks for CUIElement.
    CUIElement::UnregisterSimplePropertyCallbacks();

    return S_OK;
}

void DXamlCore::CheckForLeaks()
{
    if (!m_Peers.empty())
    {
        LOG_LEAK_EX(L"Peer table on DXamlCore is not empty!");
    }

    if (!m_ReferenceTrackers.empty())
    {
        LOG_LEAK_EX(L"Reference tracker table on DXamlCore is not empty!");
    }

    CCoreServices* coreServices = GetHandle();

    if (coreServices)
    {
        coreServices->CheckForLeaks();
    }
}

_Check_return_ HRESULT DXamlCore::DeinitializeInstance()
{
    HRESULT hr = S_OK;
    HRESULT recordHr = S_OK;

    IFCEXPECT(DXamlCore::Initialized == m_state || DXamlCore::InitializationFailed == m_state || DXamlCore::Idle == m_state);

    // Disable ticks first. We were seeing potential errors during XamlPresenter shutdown that left Xaml in a bad state,
    // which crahed on a null UIThreadScheduler in the browser host when a tick came in. Disable ticks first thing to
    // stop that from happening. The JupiterControl will be released later during cleanup. Just stopping ticks is enough,
    // since we're shutting down anyway.
    if (m_pControl)
    {
        m_pControl->SetTicksEnabled(false);
    }

    // Shut down any drag in progress, and delete the DragDrop instance.
    DeleteDragDrop();

    // Disable the Core from being GC walked but don't unregister
    // because we want to make sure that the OnFinalRelease happens
    // on the correct thread.
    IFC(ReferenceTrackerManager::DisableCore(this));

    // lock the peer table while we change the DXamlCore state
    // as there is in check in the OnReferenceTrackingStarted to test if
    // the DXamlCore is undergoing shutdown.
    {
        AutoReentrantReferenceLock peerTableLock(this);

        m_state = DXamlCore::Deinitializing;
    }

    // Destroy the TouchHitTestingHandler for this core.
    SAFE_DELETE(m_pTouchHitTestingHandler);

    // Destroy the PLM handler for this core.
    SAFE_DELETE(m_pPLMHandler);

    ReleaseWindow();

    ctl::release_interface(m_pCompositionTargetRenderingEvent);
    ctl::release_interface(m_pCompositionTargetRenderedEvent);

    // Release Focus Manager static events
    ctl::release_interface(m_pFocusManagerGotFocusEvent);
    ctl::release_interface(m_pFocusManagerLostFocusEvent);
    ctl::release_interface(m_pFocusManagerGettingFocusEvent);
    ctl::release_interface(m_pFocusManagerLosingFocusEvent);

    if (m_pDOCoreApp)
    {
        ReleaseInterface(m_pDOCoreApp);
        m_pDOCoreApp = NULL;
    }

    IFC(CommonShutdown());

    if (m_hCore)
    {
        RECORDFAILURE(CoreImports::SetFrameworkContext(m_hCore, NULL));
        ReleaseInterface(m_hCore);
    }

    if (m_pControl)
    {
        m_pControl->Deinitialize();
        ReleaseInterface(m_pControl);
    }

    m_staticStoreGuard.reset();

    UnregisterVisualDiagnosticsPort();

    // Release our reference on the FinalUnhandledErrorDetected event registration.
    IFC(ErrorHelper::GetFinalUnhandledErrorDetectedRegistration()->ReleaseRegistration());

    IFC(recordHr);

    IFC(DCompSurfaceMonitor::CleanupDCompSurfaceCollection());

Cleanup:
    m_state = DXamlCore::Deinitialized;

    return hr;
}

_Check_return_ HRESULT DXamlCore::DeinitializeInstanceToIdle()
{
    RemoveAutoHideScrollBarsChangedHandler();

    if (m_pDOCoreApp)
    {
        // CApplication is one of the few (if not only) thing that can have resources attached
        // to it prior to setting window content.  Since we don't properly reset the graphics device
        // and resources if we haven't set any content, we need to ensure that those resources
        // get released if we still have a hold of them.  Note: This must occur before
        // CommonShutdown or we end up leaking the DXAML peer.
        CApplication* pApplication = nullptr;
        IFC_RETURN(DoPointerCast(pApplication, m_pDOCoreApp));
        pApplication->CleanupDeviceRelatedResourcesRecursive(true);
    }

    // When we are shutting down to idle we want to wait until until the soundplayer
    // thread is shutdown and all the allocated objects on that thread are released.
    {
        if (auto soundPlayerService = TryGetElementSoundPlayerServiceNoRef())
        {
            soundPlayerService->WaitForThreadOnTearDown();
        }
    }

    IFC_RETURN(CommonShutdown());

    if (m_pControl)
    {
        m_pControl->SetTicksEnabled(false);
        m_pControl->DisconnectUIA();
        const auto pJupiterWindow = m_pControl->GetJupiterWindow();
        if (pJupiterWindow)
        {
            // Make the window give up its input site adapter.
            pJupiterWindow->UninitializeInputSiteAdapterForCoreWindow();
        }
    }

    CCoreServices* coreServices = GetHandle();

    if (coreServices != nullptr)
    {
        IFC_RETURN(coreServices->ShutdownToIdle());
    }

    m_staticStoreGuard.reset();

    IFC_RETURN(DCompSurfaceMonitor::CleanupDCompSurfaceCollection());
    IFC_RETURN(DCompSurfaceMonitor::DeInitialize());

    DCompSurfaceFactoryManager::Deinitialize();

    // Destroy the release queue
    m_spReleaseQueue.Reset();

    m_state = State::Idle;

    // If another test leaked, there could be leftover objects in these maps. Ignore
    // any re-allocations caused by shrink_to_fit.
#if XCP_MONITOR
    auto ignoreLeaksFromShrinkToFit = XcpDebugStartIgnoringLeaks();
#endif
    m_LayoutUpdatedEventSources.shrink_to_fit();
    m_textControlFlyoutHelperMap.shrink_to_fit();

    m_registeredControlsForSettingsChanged.shrink_to_fit();

    return S_OK;
}


void DXamlCore::EnsureDragDrop()
{
    if(m_pDragDrop == NULL)
    {
        m_pDragDrop = new DragDrop();
    }
}

void DXamlCore::SetIsWinRTDndOperationInProgress(bool inProgress)
{
    GetDragDrop()->SetIsWinRTDndOperationInProgress(inProgress);
}

/*static*/
bool DXamlCore::IsWinRTDndOperationInProgress()
{
    return DXamlCore::GetCurrent()->GetDragDrop()->IsWinRTDndOperationInProgress();
}

// Clears the internal caches. This needs to be called when the application shuts down, to release cached objects and
// allow the global object count to go down to 0.
_Check_return_ HRESULT
DXamlCore::ClearCaches()
{
    HRESULT hr = S_OK;

    // Release CompositionTarget event handlers
    ctl::release_interface(m_pCompositionTargetRenderingEvent);
    ctl::release_interface(m_pCompositionTargetRenderedEvent);
    ctl::release_interface(m_pCompositionTargetSurfaceContentsLostEvent);

    // Release Focus Manager static events
    ctl::release_interface(m_pFocusManagerGotFocusEvent);
    ctl::release_interface(m_pFocusManagerLostFocusEvent);
    ctl::release_interface(m_pFocusManagerGettingFocusEvent);
    ctl::release_interface(m_pFocusManagerLosingFocusEvent);

    // Release FocusManager event handlers
    ctl::release_interface(m_focusedElementRemovedEvent);

    // Clear the built-in style cache.
    delete m_pDefaultStyles;

    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    // Don't reallocate m_pDefaultStyles if EnableCoreShutdown is enabled
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown))
    {
        m_pDefaultStyles = NULL;
    }
    else
    {
        m_pDefaultStyles = new DefaultStyles();
    }

    // Clear the LayoutUpdated callback cache.
    m_LayoutUpdatedEventSources.clear();

    // Clear the AutoDragHelper cache.
    m_autoDragHelperMap.clear();

    // Clear the TextControlFlyoutHelper cache.
    m_textControlFlyoutHelperMap.clear();

    // Clear the app bars list
    if (m_spApplicationBarService)
    {
        ctl::ComPtr<IApplicationBarService> spApplicationBarService;
        IFC(ctl::do_query_interface(spApplicationBarService, m_spApplicationBarService.Get()));
        IFC(spApplicationBarService->ClearCaches());
    }

    if (m_spBuildTreeService)
    {
        IFC(m_spBuildTreeService->ClearWork());
    }

    ctl::release_interface(m_pToolTipServiceMetadata);

    // Clear the radio button groups by name list and the selected indices list.
    if (m_pRadioButtonGroupsByName)
    {
        m_pRadioButtonGroupsByName->Clear();
    }

    delete m_pRadioButtonGroupsByName;
    m_pRadioButtonGroupsByName = NULL;

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::NotifyImmersiveColorsChanged()
{
    HRESULT hr = S_OK;

    if (m_pDefaultStyles)
    {
        IFC(m_pDefaultStyles->RefreshImmersiveColors());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::OnThemeChanged()
{
    if (!m_hCore)
    {
        return S_OK;
    }

    IFC_RETURN(m_hCore->GetFrameworkTheming()->OnThemeChanged());

    return S_OK;
}

// Invoked when the xaml::Application::FocusVisualKind property changed.
// Ensures the current focused element's focus rect gets re-rendered according to the new property value.
void
DXamlCore::OnApplicationFocusVisualKindChanged()
{
    if (m_pDOCoreApp != nullptr)
    {
        checked_cast<CApplication>(m_pDOCoreApp)->OnFocusVisualKindChanged();
    }
}

_Check_return_ HRESULT
DXamlCore::OnApplicationHighContrastAdjustmentChanged()
{
    if (m_pDOCoreApp != nullptr)
    {
        CApplication* pApplication = nullptr;
        IFC_RETURN(DoPointerCast(pApplication, m_pDOCoreApp));
        IFC_RETURN(pApplication->OnHighContrastAdjustmentChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::UpdateFontScale(_In_ XFLOAT newFontScale)
{
    HRESULT hr = S_OK;

    if (m_hCore)
    {
        IFC(m_hCore->UpdateFontScale(newFontScale));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::GetNonLocalizedErrorString(_In_ XUINT32 resourceStringID, _Out_ HSTRING* errorString)
{
    xstring_ptr strError;
    xruntime_string_ptr strRuntimeError;

    IXcpBrowserHost* pBrowserHost = NULL;

    IFCPTR_RETURN(errorString);
    IFCPTR_RETURN(m_hCore);

    pBrowserHost = m_hCore->GetBrowserHost();
    IFCPTR_RETURN(pBrowserHost);

    IFC_RETURN(pBrowserHost->GetNonLocalizedErrorString(resourceStringID, &strError));

    IFC_RETURN(strError.Promote(&strRuntimeError));

    *errorString = strRuntimeError.DetachHSTRING();
    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::GetLocalizedResourceString(_In_ XUINT32 resourceStringID, _Out_ HSTRING* resourceString)
{
    xstring_ptr strResource;
    xruntime_string_ptr strRuntimeResource;

    IXcpBrowserHost* pBrowserHost = NULL;

    IFCPTR_RETURN(resourceString);
    IFCPTR_RETURN(m_hCore);

    pBrowserHost = m_hCore->GetBrowserHost();
    IFCPTR_RETURN(pBrowserHost);

    IFC_RETURN(pBrowserHost->GetLocalizedResourceString(resourceStringID, &strResource));

    IFC_RETURN(strResource.Promote(&strRuntimeResource));

    *resourceString = strRuntimeResource.DetachHSTRING();
    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::GetNonLocalizedResourceString(_In_ XUINT32 resourceStringID, _Out_ HSTRING* resourceString)
{
    xstring_ptr strResource;
    xruntime_string_ptr strRuntimeResource;

    IXcpBrowserHost* pBrowserHost = NULL;

    IFCPTR_RETURN(resourceString);
    IFCPTR_RETURN(m_hCore);

    pBrowserHost = m_hCore->GetBrowserHost();
    IFCPTR_RETURN(pBrowserHost);

    IFC_RETURN(pBrowserHost->GetNonLocalizedResourceString(resourceStringID, &strResource));

    IFC_RETURN(strResource.Promote(&strRuntimeResource));

    *resourceString = strRuntimeResource.DetachHSTRING();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Load binary data identified by pResourceName from the
//      platform resource module.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
DXamlCore::GetResourceBytes(
    _In_z_ const WCHAR* pszResourceName,
    _Out_ Parser::XamlBuffer* pBuffer)
{
    HRESULT hr = S_OK;
    Parser::XamlBuffer result;

    EnsurePlatformResourceModuleHandle();

    HRSRC rc = FindResource(g_platformResourcesModuleHandle, pszResourceName, MAKEINTRESOURCE(TEXTFILE));
    if (!rc)
    {
        IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
    }

    DWORD cbXbf = SizeofResource(g_platformResourcesModuleHandle, rc);
    if (0 == cbXbf)
    {
        IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
    }

    HGLOBAL rcData = LoadResource(g_platformResourcesModuleHandle, rc);
    if (!rcData)
    {
        IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
    }

    result.m_count = cbXbf;
    result.m_buffer = static_cast<const XUINT8*>(LockResource(rcData));
    result.m_bufferType = Parser::XamlBufferType::MemoryMappedResource;
    *pBuffer = result;

    return hr;
}

_Check_return_
HRESULT
DXamlCore::GetDebugInterop(_Outptr_result_maybenull_ DebugTool::IInternalDebugInterop** ppDebugInterop)
{
    return m_wpDebugInterop.CopyTo(ppDebugInterop);
}

_Check_return_
HRESULT
DXamlCore::SetDebugInterop(_In_ DebugTool::IInternalDebugInterop* pDebugInterop)
{
    return wrl::AsWeak(pDebugInterop, &m_wpDebugInterop);
}

_Check_return_ HRESULT
DXamlCore::ActivatePeer(_In_ KnownTypeIndex nTypeIndex, _COM_Outptr_ DependencyObject** ppObject)
{
    HRESULT hr = S_OK;
    CDependencyObject* pCoreDO = NULL;

    // Activate core peer first.
    IFC(CoreImports::CreateObjectByTypeIndex(
        GetHandle(),
        nTypeIndex,
        &pCoreDO));

    // Activate framework peer next.
    IFC(GetPeer(pCoreDO, ppObject));

Cleanup:
    ReleaseInterface(pCoreDO);
    return hr;
}

_Check_return_ HRESULT
DXamlCore::GetPeer(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(NULL, pDO, GetPeerPrivateCreateMode::CreateIfNecessary, FALSE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::GetPeerWithInternalRef(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(NULL, pDO, GetPeerPrivateCreateMode::CreateIfNecessary, /*bInternalRef*/ TRUE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::GetPeer(_In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(NULL, pDO, GetPeerPrivateCreateMode::CreateIfNecessary, hClass, FALSE, NULL, ppObject);
}


_Check_return_ HRESULT
DXamlCore::TryGetPeer(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(NULL, pDO, GetPeerPrivateCreateMode::GetOnly, FALSE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::TryGetPeerWithInternalRef(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(NULL, pDO, GetPeerPrivateCreateMode::GetOnly, /*bInternalRef*/ TRUE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::TryGetPeer(_In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(NULL, pDO, GetPeerPrivateCreateMode::GetOnly, hClass, FALSE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::TryGetPeer(_In_ CDependencyObject* pDO, _Out_ bool *pIsPendingDelete, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(NULL, pDO, GetPeerPrivateCreateMode::GetOnly, FALSE, pIsPendingDelete, ppObject);
}

_Check_return_ HRESULT
DXamlCore::GetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(pOuter, pDO, GetPeerPrivateCreateMode::CreateIfNecessary, FALSE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::GetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(pOuter, pDO, GetPeerPrivateCreateMode::CreateIfNecessary, hClass, FALSE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::GetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _In_ bool fCreatePegged, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(pOuter, pDO, GetPeerPrivateCreateMode::CreateIfNecessary, hClass, FALSE, /* fCreatePegged */ fCreatePegged, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::TryGetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(pOuter, pDO, GetPeerPrivateCreateMode::GetOnly, FALSE, NULL, ppObject);
}

_Check_return_ HRESULT
DXamlCore::TryGetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(pOuter, pDO, GetPeerPrivateCreateMode::GetOnly, hClass, FALSE, NULL, ppObject);
}

// Get a peer if it already exists.  Otherwise, if possible, create one.  It's possible if this object's peer has never been created
// before, or if it's resurrectable.
_Check_return_ HRESULT
DXamlCore::TryGetOrCreatePeer(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject)
{
    return GetPeerPrivate(/*pOuter*/ NULL, pDO, GetPeerPrivateCreateMode::TryCreateIfNecessary, /*bInternalRef*/ FALSE, /*pbIsPendingDelete*/ NULL, ppObject);
}



_Check_return_ HRESULT
DXamlCore::GetPeerPrivate(
    _In_opt_ IInspectable* pOuter,
    _In_ CDependencyObject* pDO,
    _In_ GetPeerPrivateCreateMode createMode,
    _In_ bool bInternalRef,
    _Out_opt_ bool *pIsPendingDelete,
    _Out_ DependencyObject** ppObject)
{
    HRESULT hr = S_OK;
    KnownTypeIndex hClass = KnownTypeIndex::Object;

    IFCPTR(pDO);
    IFCPTR(ppObject);

    IFC(CoreImports::DependencyObject_GetTypeIndex(pDO, &hClass));

    // Don't use IFC; failing this call is an expected case and no need to create debug output.
    hr = GetPeerPrivate(pOuter, pDO, createMode, hClass, bInternalRef, pIsPendingDelete, ppObject);
    if( FAILED(hr) ) goto Cleanup;

Cleanup:

    return hr;
}

_Check_return_ HRESULT
DXamlCore::GetPeerPrivate(
    _In_opt_ IInspectable* pOuter,
    _In_ CDependencyObject* pCoreDO,
    _In_ GetPeerPrivateCreateMode createMode,
    _In_ KnownTypeIndex hClass,
    _In_ bool bInternalRef,
    _Out_opt_ bool *pIsPendingDelete,
    _Out_ DependencyObject** ppObject)
{
    return GetPeerPrivate(pOuter, pCoreDO, createMode, hClass, bInternalRef, /* fCreatePegged */ FALSE, pIsPendingDelete, ppObject);
}


_Check_return_ HRESULT
DXamlCore::GetPeerPrivate(
    _In_opt_ IInspectable* pOuter,
    _In_ CDependencyObject* pCoreDO,
    _In_ GetPeerPrivateCreateMode createMode,
    _In_ KnownTypeIndex hClass,
    _In_ bool bInternalRef,
    _In_ bool fCreatePegged,
    _Out_opt_ bool *pIsPendingDelete,
    _Out_ DependencyObject** ppObject)
{
    HRESULT hr = S_OK;
    DependencyObject* pDO = NULL;
    bool fForceStrong = false;
    ctl::ComPtr<IInspectable> spNewInstance;

#if DBG
    bool fFailedResurrection = false;
#endif

#if XCP_MONITOR
    if (ReferenceTrackerManager::IsPeerStressEnabled())
    {
        IFC(ReferenceTrackerManager::TriggerCollection());
    }
#endif

    {
        IFCPTR(pCoreDO);
        IFCPTR(ppObject);

        *ppObject = NULL;
        if (pIsPendingDelete)
        {
            *pIsPendingDelete = FALSE;
        }

        // Check if we already have a peer for this object.
        if (DependencyObject* pDONoRef = pCoreDO->GetDXamlPeer())
        {
            // It exists, so we just return that.
            *ppObject = pDONoRef;

            // Get a ref-count on the target, possibly going directly to the inner object
            if (bInternalRef)
            {
                // We take a reference directly on the inner object, rather than going to any
                // outer/controlling object, because the controlling object might have been garbage collected
                // already.
                ctl::addref_interface_inner(pDONoRef);
            }
            else
            {
                AutoReentrantReferenceLock lock(this);

                // Don't allow peers to be resolved that are no longer reachable. If you really need to resolve
                // such a peer, pass in bInternalRef=true to get a reference to the inner object.
                // We don't allow this because the GC will collect this object soon. We cannot rely on proactive
                // ReleaseQueuedObjects calls, because the GC may not have queued the final release yet (because it
                // will happen on a different thread).
                if (pDONoRef->IsReachable())
                {
                    ctl::addref_interface(pDONoRef);
                }
                // The exception to this rule are resurrectable objects (such as SolidColorBrush and
                // UIElementCollection). If the object is marked as not-reachable, we may get a release from the GC
                // soon. However, because of the following add-ref, that release will simply be a regular release,
                // rather than a final release.
                else if (pDONoRef->AllowResurrection())
                {
                    pDONoRef->Resurrect();
                    ctl::addref_interface(pDONoRef);
                }
                else
                {
                    *ppObject = NULL;

                    if (pIsPendingDelete)
                    {
                        // Peer is not reachable and is not resurrectable
                        *pIsPendingDelete = TRUE;
                    }

                    // Don't assert or Fail if this is a TryGetPeer call.
                    if (createMode == GetPeerPrivateCreateMode::CreateIfNecessary)
                    {
                        #if DBG
                        // We should never get here. Either pDONoRef needs to be marked as "AllowResurrectable", or
                        // we should make sure we keep it alive proper (so that it's reachable).
                        // (Don't assert here, because we're holding the AutoReentrantReferenceLock, and asserting can trigger the CLR,
                        // and cause it to block waiting for GC, which then hangs waiting on this lock.)
                        fFailedResurrection = TRUE;
                        #endif

                        IFC(E_FAIL);
                    }
                }
            }
        }

        // If it doesn't exist, can we create one?  We can create one if the caller specified CreateIfNecessary, or if the
        // caller specified TryCreateIfNecessary and it's OK to create.  It's OK to create in that case if the peer is resurrectable
        // (which generally means its currently stateless).
        else if (createMode == GetPeerPrivateCreateMode::CreateIfNecessary
                 || (createMode == GetPeerPrivateCreateMode::TryCreateIfNecessary && pCoreDO->AllowPeerResurrection()))
        {
            IFCEXPECT(pCoreDO->GetTypeIndex() != KnownTypeIndex::UnknownType);

            // Instantiate the peer using the type table.
            CREATEPFNFX pfnCreate = c_aTypeActivations[static_cast<int>(pCoreDO->GetTypeIndex())].m_pfnCreateFramework;
            if (!pfnCreate)
            {
                // Type doesn't have a framework peer.
                hr = E_FAIL;
                goto Cleanup;
            }

            IFC_NOTRACE(DependencyObject::CreateAggregableDO(pfnCreate, pCoreDO, pOuter, &spNewInstance));

            // This event is only enabled when the user turns on the Diagnostics provider. Generally this isn't
            // done during perf profiling as there are VERY verbose events that get enabled. However, it is used for AppAnalysis
            // so that we can pass information back to VS allowing them to link AppAnalysis to the LVT. XamlDiagnostics uses
            // InstanceHandle representations of the IInspectable and AppAnalysis uses CDependencyObject. App Analysis should
            // still work without peers, since they won't always be around if XamlDiag isn't in use and that's why we use this event to
            // relate the CDepenencyObject to the IInspectable. This has the added benefit of updating AppAnalysis if any stateless
            // peers get cleaned up while xaml diag isn't connected (i.e. attach scenarios)
            if (EventEnabledPeerCreatedInfo())
            {
                TracePeerCreatedInfo(reinterpret_cast<UINT64>(pCoreDO), reinterpret_cast<UINT64>(spNewInstance.Get()));
            }

            pDO = static_cast<DependencyObject*>(static_cast<ctl::ComBase*>(spNewInstance.Detach()));

            #if DBG
            ReferenceTrackerLogCreate::Log( pDO, pCoreDO );
            #endif


            // Don't update tracked references while the ReferenceTrackerManager is running
            {
                AutoReentrantReferenceLock peerTableLock(this);

                m_Peers.insert(pDO);
                pCoreDO->SetDXamlPeer(pDO);

                IFC(CoreImports::DependencyObject_ShouldCreatePeerWithStrongRef(pCoreDO, &fForceStrong));

                if (fForceStrong)
                {
                    pDO->PegNoRef();
                }

                // Set the create-time peg used by ReferenceTrackerManager, which protects the object
                // from removal until it's protected either by a reference tracker source, or by another object.
                pDO->SetReferenceTrackerPeg();

                if (fCreatePegged)
                {
                    // For cases where neither the Visual Tree nor an RCW is holding a reference and we
                    // need it to be temporarily reachable until it gets parented, Peg to make it a Root.
                    // Eg. RootScrollViewer
                    pDO->UpdatePeg(true);
                }
            }

            // For core types which are extended by composition we should report that such controls are custom types
            // thus RPInvokes for overrides (MeasureOverride, ArrangeOverride, etc.) will be invoked for such types.
            IFC(CoreImports::NotifyHasManagedPeer(
                pCoreDO,
                valueObject,
                /* bIsCustomType */ pDO->IsComposed() || pDO->GetHasState() || pDO->IsExternalObjectReference(),
                fForceStrong));

            // NOTE:
            // Do not include PrepareState in the PeerTable lock because PrepareState can call into Managed (eg. GetOtherMetadataProviders)
            // and cause a deadlock with a background GC.

            // allow constructed managed peers to setup state
            IFC(pDO->PrepareState());

            *ppObject = pDO;
            pDO = NULL;

        }
    }

Cleanup:

    #if DBG
    // Bug 39525050: [WinUI3]NavigationView hits an assert that crashes CHK builds after WinUI 2.8 port
    //ASSERT( !fFailedResurrection );
    #endif

    ctl::release_interface(pDO);
    return hr;
}

void
DXamlCore::RemovePeer(_In_ DependencyObject* pDO)
{
    if (DXamlCore::Initialized == m_state || DXamlCore::Deinitializing == m_state)
    {
        // Don't update tracked references while the ReferenceTrackerManager is running
        AutoReentrantReferenceLock peerTableLock(this);
        CDependencyObject* pCoreDO = pDO->GetHandle();

        // Ordinarily, just remove the entry from the map
        m_Peers.erase(pDO);

        if (pCoreDO)
        {
            pCoreDO->SetDXamlPeer(nullptr);
        }

    }
}

void DXamlCore::AddToReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem)
{
    if (pItem != NULL)
    {
        // Don't update tracked references while the ReferenceTrackerManager is running
        AutoReentrantReferenceLock peerTableLock(this);

#if DBG
        // ASSERT visible to Prefast fre build but references DBG-only method
        ASSERT(!IsInReferenceTrackingList(pItem));
#endif

        m_ReferenceTrackers.insert(pItem);
        ctl::addref_expected(pItem, ctl::ExpectedRef_Tree);

    }
}

void
DXamlCore::RemoveFromReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem)
{
    if (DXamlCore::Initialized == m_state || DXamlCore::Deinitializing == m_state || DXamlCore::Idle == m_state)
    {
        if (pItem != NULL)
        {
            auto it = m_ReferenceTrackers.find(pItem);

            if (it != m_ReferenceTrackers.end())
            {
                // Don't update tracked references while the ReferenceTrackerManager is running

                AutoReentrantReferenceLock peerTableLock(this);

                ctl::release_expected(pItem);

                // Ordinarily, just remove the entry from the map
                m_ReferenceTrackers.erase(pItem);
#if DBG
                // ASSERT visible to Prefast fre build but references DBG-only method
                ASSERT(!IsInReferenceTrackingList(pItem));
#endif

#if XCP_MONITOR
                if (m_ReferenceTrackers.empty())
                {
                    m_ReferenceTrackers.clear();
                }
#endif


            }
        }
    }

}

_Check_return_ HRESULT DXamlCore::ShutdownAllPeers()
{
    HRESULT hr = S_OK;
    HRESULT recordHr = S_OK;

    // RS5 Bug #16045535:  If VSM sets a long-lived animated value on a DO, this DO will leak when used in a secondary View.
    // The issue here is that the CModifiedValue destructor doesn't run until the XAML tree is finally being torn down, which
    // happens after we've shutdown all the peers.  Once this happens, the CModifiedValue cannot release its expected reference
    // because the CDO is no longer connected to its DXAML peer.
    // The general-purpose fix is to keep track of all CModifiedValue's that have taken an expected reference, and explicitly
    // clear these references here, just before shutting down all the peers.
    GetHandle()->ClearValuesWithExpectedReference();

    // Notify the ReferenceTrackerManager
    if (NotifyEndOfReferenceTrackingOnThread())
    {
        ReferenceTrackerManager::OnShutdownAllPeers();
    }

    // Separate the framework peers from their core peers, so that when we start
    // to release everything, we're not bothering to call back into the framework from the core.

    // Reference cycles can cause DXaml peers to remain in the peer table (e.g. a Canvas referencing its XamlLightCollection, which
    // references a XamlLight inside, which was defined by the app and references back to the Canvas that it's connected to). Once we
    // break the cycle in BeginShutdown (via DisconnectManagedPeer -> ResetReferencesFromSparsePropertyValues to remove the reference
    // from the CCanvas to the CXamlLightCollection), these peers can all get deleted and removed from the peer list. That means the
    // list can change as we iterate over it, so make a copy first and make sure each entry is still in the original list before
    // calling BeginShutdown on it.
    PeerTable clonedTable(m_Peers);
    for (auto it = clonedTable.begin(); it != clonedTable.end(); ++it)
    {
        if (m_Peers.find(*it) != m_Peers.end())
        {
            (*it)->BeginShutdown();
        }
    }

    // Go to each framework peer and release everything it references, including
    // the reference to the core peer.

    while (!m_Peers.empty())
    {
        DependencyObject* pDO = *m_Peers.begin();
        CDependencyObject* pCoreDO = pDO->GetHandle();
        if (pCoreDO)
        {
            pCoreDO->SetDXamlPeer(nullptr);
        }
        VERIFYRECORDFAILURE(pDO->EndShutdown());
        m_Peers.erase(pDO);
    }

    while (!m_ReferenceTrackers.empty())
    {
        auto pTracker = *m_ReferenceTrackers.begin();
        VERIFYHR(pTracker->EndShutdown());
        m_ReferenceTrackers.erase(pTracker);
    }

    ASSERT(m_Peers.empty());
    ASSERT(m_ReferenceTrackers.empty());
    IFC(recordHr);

Cleanup:
    return hr;
}

//
//  Keep elements with LayoutUpdated event listeners in a global map, used in OnLayoutUpdated
//  We register the element itself, not the event source or event handler, so that we can keep a
//  valid weak reference (in a C# application, weak references to elements are cleaned automatically).
_Check_return_ HRESULT
DXamlCore::RegisterLayoutUpdatedEventSource(_In_ FrameworkElement *pFrameworkElement)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spFrameworkElement = pFrameworkElement;
    ctl::WeakRefPtr spWeakReference = nullptr;

    // Check that the insertion element is a new one after ensuring that all elements still exist.
    // If we don't do this, we can occasionally run into the situation where a pointer to an element
    // is added to this list, then it gets deleted, then another pointer to an element is allocated
    // at the exact same memory address, and then we *think* that that element is already in this list
    // when in fact it's a different element, just at the same memory address.

    // We need to make a copy of the pairs to remove since removing elements from the map we're iterating over
    // can run into trouble.
    std::vector<std::pair<HANDLE, ctl::WeakRefPtr>> handlersToRemove;

    for (auto it = m_LayoutUpdatedEventSources.begin(); it != m_LayoutUpdatedEventSources.cend(); ++it)
    {
        ctl::ComPtr<IFrameworkElement> spFrameworkElement2;
        IFC(it->second.As<IFrameworkElement>(&spFrameworkElement2));

        if (!spFrameworkElement2)
        {
            handlersToRemove.push_back(*it);
        }
    }

    for (auto it = handlersToRemove.cbegin(); it != handlersToRemove.cend(); ++it)
    {
        m_LayoutUpdatedEventSources.erase(it->first);
    }

    ASSERT(m_LayoutUpdatedEventSources.find(pFrameworkElement) == m_LayoutUpdatedEventSources.cend());

    // Only keep a weak reference to the element so that we don't prevent its destruction
    IFC(spFrameworkElement.AsWeak(&spWeakReference));
    m_LayoutUpdatedEventSources[pFrameworkElement] = spWeakReference;

Cleanup:
    return hr;
}

//
//  Remove element from LayoutUpdated event listeners from a global map, used in OnLayoutUpdated
//
void DXamlCore::UnregisterLayoutUpdatedEventSource(_In_ FrameworkElement *pFrameworkElement)
{
    auto iterator = m_LayoutUpdatedEventSources.find(pFrameworkElement);

    if (iterator != m_LayoutUpdatedEventSources.cend())
    {
        m_LayoutUpdatedEventSources.erase(iterator);
    }
}

_Check_return_ HRESULT
DXamlCore::RegisterEventSource(_In_ IUntypedEventSource* pEventSource, _In_ bool bCoreEvent)
{
    IFCPTR_RETURN(pEventSource);
    if (bCoreEvent)
    {
        DependencyObject* pTarget = static_cast<DependencyObject*>(pEventSource->GetTargetNoRef());
        IFCEXPECT_RETURN(pTarget);

        CValue notUsed;
        CDependencyObject* pObject = pTarget->GetHandle();
        KnownEventIndex eventIndex = pEventSource->GetHandle();

        if(pObject)
        {
            IFC_RETURN(pObject->AddEventListener(EventHandle(eventIndex), &notUsed, EVENTLISTENER_CLR, nullptr, /* fHandledEventsToo */ true));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::UnregisterEventSource(_In_ IUntypedEventSource* pEventSource, _In_ bool bCoreEvent)
{
    IFCPTR_RETURN(pEventSource);
    if (bCoreEvent)
    {
        DependencyObject* pTarget = static_cast<DependencyObject*>(pEventSource->GetTargetNoRef());
        IFCEXPECT_RETURN(pTarget);

        if (!pTarget->GetHandle())
        {
            // This can happen when we're in the middle of shutdown.
            // If the core peer has been disconnected, nothing to do here -
            // the event listener will already have been removed.
            return S_OK;
        }

        CDependencyObject* pObject = pTarget->GetHandle();
        KnownEventIndex eventIndex = pEventSource->GetHandle();
        CValue notUsed;

        IFCEXPECT_RETURN(pObject);

        IFC_RETURN(pObject->RemoveEventListener(EventHandle(eventIndex), &notUsed));
    }

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::FireEvent(
    _In_ CDependencyObject* pCoreListener,
    _In_ KnownEventIndex eventId,
    _In_ CDependencyObject* pCoreSender,
    _In_opt_ CEventArgs* pCoreArgs,
    _In_ XUINT32 flags)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spListener;
    ctl::ComPtr<DependencyObject> spSender;
    ctl::ComPtr<IInspectable> spArgs;
    bool isArgsTypeExist = false;
    CCoreServices* coreServices = GetHandle();

    IFCPTR(pCoreSender);

    IFC(TryGetPeer(pCoreSender, &spSender));
    if (spSender == nullptr
        // We should always have a peer. The exception is if a test just reset the visual tree. We can have outstanding
        // events like Unloaded that are still queued up to fire. They can be ignored.
        && (coreServices->IsFrameAfterVisualTreeReset()
        // ReplayPointerUpdate can request future frames if it gets throttled, which can result in additional frames
        // after a tree reset. But those frames won't have anything to render, so check for that too.
            || !coreServices->HasRenderedFrame()))
    {
        // Do nothing. This was an event queued up before the test reset the visual tree.
    }
    else
    {
        IFCCHECK(spSender != NULL);

        if (pCoreArgs != NULL)
        {
            IFC(pCoreArgs->GetFrameworkPeer(&spArgs));
        }

        // TODO: remove this BOOLEAN when 98214 is fixed, it's a work around for that as it leads to failure of 3 P0s
        isArgsTypeExist = TRUE;

        if (pCoreListener)
        {
            IFC(GetPeer(pCoreListener, &spListener));
            IFC(spListener->FireEvent(eventId, ctl::as_iinspectable(spSender.Get()), spArgs.Get()));
        }
        else
        {
            if (static_cast<XUINT32>(eventId) <= static_cast<XUINT32>(LastControlEvent))
            {
                // This is a control event.
                IFC(DirectUI::Control::FireEvent(eventId, spSender.Get(), spArgs.Get()));
            }
        }
    }

Cleanup:
    if(FAILED(hr) && isArgsTypeExist)
    {
        ErrorHelper::ReportUnhandledError(hr);
    }

    if (spSender && (flags & EVENT_SENDER_PEGGED))
    {
        spSender->UpdatePegWithPossibleShutdownException(false, TRUE /*ShutdownException*/);
    }
    return hr;
}

_Check_return_ HRESULT
DXamlCore::RaiseEvent(_In_ CDependencyObject* target, _In_opt_ CEventArgs* pCoreArgs, _In_ ManagedEvent eventId)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spSender;
    ctl::ComPtr<xaml::IFrameworkElement> spSenderAsIFE;
    ctl::ComPtr<IInspectable> spArgs;
    ctl::ComPtr<xaml::ISizeChangedEventArgs> spSizeChangedArgs;
    bool isArgsTypeExist = false;

    if (target)
    {
        IFC(GetPeer(target, &spSender));
    }
    if (pCoreArgs)
    {
        IFC(pCoreArgs->GetFrameworkPeer(&spArgs));
    }

    // TODO: remove this BOOLEAN when 98214 is fixed, it's a work around for that as it leads to failure of 3 P0s
    isArgsTypeExist = TRUE;

    switch (eventId)
    {
        case ManagedEvent::ManagedEventSizeChanged:
            {
                IFC(spSender.As(&spSenderAsIFE));
                IFC(spArgs.As(&spSizeChangedArgs));
                IFC(spSenderAsIFE.Cast<FrameworkElement>()->OnSizeChanged(spSenderAsIFE.Get(), spSizeChangedArgs.Get()));
            }
            break;

        case ManagedEvent::ManagedEventLayoutUpdated:
            {
                IFC(OnLayoutUpdated(ctl::as_iinspectable(spArgs.Get())));
            }
            break;

        case ManagedEvent::ManagedEventInheritanceContextChanged:
            IGNOREHR(spSender->OnInheritanceContextChanged());
            break;

        case ManagedEvent::ManagedEventRendering:
            {
                IFC(OnRenderingEvent(ctl::as_iinspectable(spArgs.Get())));
            }
            break;
        case ManagedEvent::ManagedEventRendered:
            {
                ctl::ComPtr<xaml_media::IRenderedEventArgs> renderedEventArgs;
                IFC(spArgs.As(&renderedEventArgs));
                IFC(OnRenderedEvent(renderedEventArgs.Get()));
            }
            break;
        case ManagedEvent::ManagedEventSurfaceContentsLost:
            {

                IFC(OnSurfaceContentsLostEvent(ctl::as_iinspectable(spArgs.Get())));
            }
            break;
        case ManagedEvent::ManagedEventFocusedElementRemoved:
            {
                ctl::ComPtr<xaml_input::IFocusedElementRemovedEventArgs> focusedElementRemovedArgs;
                IFC(spArgs.As(&focusedElementRemovedArgs));
                IFC(OnFocusedElementRemovedEvent(focusedElementRemovedArgs.Get()));
            }
            break;
        case ManagedEvent::ManagedEventGotFocus:
            {
                ctl::ComPtr<xaml_input::IFocusManagerGotFocusEventArgs> gotEventArgs;
                IFC(spArgs.As(&gotEventArgs));
                IFC(OnFocusManagerGotFocusEvent(gotEventArgs.Get()));
            }
        break;
        case ManagedEvent::ManagedEventLostFocus:
            {
                ctl::ComPtr<xaml_input::IFocusManagerLostFocusEventArgs> lostEventArgs;
                IFC(spArgs.As(&lostEventArgs));
                IFC(OnFocusManagerLostFocusEvent(lostEventArgs.Get()));
            }
        break;
        case ManagedEvent::ManagedEventGettingFocus:
            {
                ctl::ComPtr<xaml_input::IGettingFocusEventArgs> gettingEventArgs;
                IFC(spArgs.As(&gettingEventArgs));
                IFC(OnFocusManagerGettingFocusEvent(gettingEventArgs.Get()));
            }
        break;
        case ManagedEvent::ManagedEventLosingFocus:
            {
                ctl::ComPtr<xaml_input::ILosingFocusEventArgs> losingEventArgs;
                IFC(spArgs.As(&losingEventArgs));
                IFC(OnFocusManagerLosingFocusEvent(losingEventArgs.Get()));
            }
        break;
        default:
            IFC(E_NOTIMPL);
            break;
    }

Cleanup:
    if(FAILED(hr) && isArgsTypeExist)
    {
        ErrorHelper::ReportUnhandledError(hr);
    }
    return hr;
}

_Check_return_ HRESULT
DXamlCore::OnLayoutUpdated(_In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    std::vector<std::pair<HANDLE, ctl::WeakRefPtr>> handlers;

    // Copy the list first to prevent re-entrancy problems
    std::for_each(m_LayoutUpdatedEventSources.cbegin(), m_LayoutUpdatedEventSources.cend(),
        [&handlers](const std::pair<HANDLE, ctl::WeakRefPtr> &element) {
            handlers.push_back(element); });

    // Walk all the elements that have LayoutUpdated event listeners
    // We cannot use handlers.cbegin() const iterator here because
    // WeakRefPtr.As can change the WeakRefPtr state during InternalResolve
    for (auto it = handlers.begin(); it != handlers.cend(); ++it)
    {
        ctl::ComPtr<IFrameworkElement> spFrameworkElement;
        IFC(it->second.As<IFrameworkElement>(&spFrameworkElement));

        // See if the element still exists
        // Note. Even though our key is IFrameworkElement* we should never use it as pointer and deref or cast it.
        if (spFrameworkElement)
        {
            // Invoke the event source.
            IFC(spFrameworkElement.Cast<FrameworkElement>()->RaiseLayoutUpdated(/*pSender*/ NULL, pArgs));
        }
        else
        {
            // Remove dead entries.
            m_LayoutUpdatedEventSources.erase(it->first);
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::OnFocusManagerGotFocusEvent(_In_ xaml_input::IFocusManagerGotFocusEventArgs* pArgs)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>> spEventSource;
    IFC_RETURN(GetFocusManagerGotFocusEventSource(spEventSource.ReleaseAndGetAddressOf()));
    IFC_RETURN(spEventSource->Raise(NULL, pArgs));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetFocusManagerGotFocusEventSource(_Outptr_
    CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>** ppEventSource)
{
    CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>* pEventSource;

    if (m_pFocusManagerGotFocusEvent != NULL)
    {
        pEventSource = m_pFocusManagerGotFocusEvent;
    }
    else
    {
        IFC_RETURN((ctl::ComObject<CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>>::CreateInstance(&pEventSource)));
        pEventSource->Initialize(KnownEventIndex::FocusManager_GotFocus, NULL, FALSE /*bCoreEvent*/
#if DBG
            , TRUE // This is a static event
#endif
        );
        m_pFocusManagerGotFocusEvent = pEventSource;
    }

    *ppEventSource = pEventSource;
    ctl::addref_interface(pEventSource);

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::OnFocusManagerLostFocusEvent(_In_ xaml_input::IFocusManagerLostFocusEventArgs* pArgs)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>> spEventSource;
    IFC_RETURN(GetFocusManagerLostFocusEventSource(spEventSource.ReleaseAndGetAddressOf()));
    IFC_RETURN(spEventSource->Raise(NULL, pArgs));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetFocusManagerLostFocusEventSource(_Outptr_
    CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>** ppEventSource)
{
    CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>* pEventSource;

    if (m_pFocusManagerLostFocusEvent != NULL)
    {
        pEventSource = m_pFocusManagerLostFocusEvent;
    }
    else
    {
        IFC_RETURN((ctl::ComObject<CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>>::CreateInstance(&pEventSource)));
        pEventSource->Initialize(KnownEventIndex::FocusManager_LostFocus, NULL, FALSE /*bCoreEvent*/
#if DBG
            , TRUE // This is a static event
#endif
        );
        m_pFocusManagerLostFocusEvent = pEventSource;
    }

    *ppEventSource = pEventSource;
    ctl::addref_interface(pEventSource);

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::OnFocusManagerGettingFocusEvent(_In_ xaml_input::IGettingFocusEventArgs* pArgs)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>> spEventSource;
    IFC_RETURN(GetFocusManagerGettingFocusEventSource(spEventSource.ReleaseAndGetAddressOf()));
    IFC_RETURN(spEventSource->Raise(NULL, pArgs));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetFocusManagerGettingFocusEventSource(_Outptr_
    CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>** ppEventSource)
{
    CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>* pEventSource;

    if (m_pFocusManagerGettingFocusEvent != NULL)
    {
        pEventSource = m_pFocusManagerGettingFocusEvent;
    }
    else
    {
        IFC_RETURN((ctl::ComObject<CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>>::CreateInstance(&pEventSource)));
        pEventSource->Initialize(KnownEventIndex::FocusManager_GettingFocus, NULL, FALSE /*bCoreEvent*/
#if DBG
            , TRUE // This is a static event
#endif
        );
        m_pFocusManagerGettingFocusEvent = pEventSource;
    }

    *ppEventSource = pEventSource;
    ctl::addref_interface(pEventSource);

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::OnFocusManagerLosingFocusEvent(_In_ xaml_input::ILosingFocusEventArgs* pArgs)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>> spEventSource;
    IFC_RETURN(GetFocusManagerLosingFocusEventSource(spEventSource.ReleaseAndGetAddressOf()));
    IFC_RETURN(spEventSource->Raise(NULL, pArgs));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetFocusManagerLosingFocusEventSource(_Outptr_
    CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>** ppEventSource)
{
    CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>* pEventSource;

    if (m_pFocusManagerLosingFocusEvent != NULL)
    {
        pEventSource = m_pFocusManagerLosingFocusEvent;
    }
    else
    {
        IFC_RETURN((ctl::ComObject<CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>>::CreateInstance(&pEventSource)));
        pEventSource->Initialize(KnownEventIndex::FocusManager_LosingFocus, NULL, FALSE /*bCoreEvent*/
#if DBG
            , TRUE // This is a static event
#endif
        );
        m_pFocusManagerLosingFocusEvent = pEventSource;
    }

    *ppEventSource = pEventSource;
    ctl::addref_interface(pEventSource);

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::OnRenderingEvent(_In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource = NULL;
    IFC(GetRenderingEventSource(&pEventSource));
    IFC(pEventSource->Raise(NULL, pArgs));

Cleanup:
    ctl::release_interface(pEventSource);
    return hr;
}

_Check_return_ HRESULT DXamlCore::GetRenderingEventSource(_Outptr_
    CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>** ppEventSource)
{
    HRESULT hr = S_OK;
    CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource;

    if (m_pCompositionTargetRenderingEvent != NULL)
    {
        pEventSource = m_pCompositionTargetRenderingEvent;
    }
    else
    {
        hr = ctl::ComObject<CEventSource<wf::IEventHandler<IInspectable*>,  IInspectable, IInspectable>>::CreateInstance(&pEventSource);
        IFC(hr);
        pEventSource->Initialize(KnownEventIndex::CompositionTarget_Rendering, NULL, FALSE /*bCoreEvent*/
            #if DBG
            , TRUE // This is a static event
            #endif
            );
        m_pCompositionTargetRenderingEvent = pEventSource;
    }

    *ppEventSource = pEventSource;
    ctl::addref_interface(pEventSource);

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::OnRenderedEvent(_In_ IRenderedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>* pEventSource = NULL;
    IFC(GetRenderedEventSource(&pEventSource));
    IFC(pEventSource->Raise(NULL, pArgs));

Cleanup:
    ctl::release_interface(pEventSource);
    return hr;
}

_Check_return_ HRESULT DXamlCore::GetRenderedEventSource(_Outptr_
    CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>** ppEventSource)
{
    HRESULT hr = S_OK;
    CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>* pEventSource;

    if (m_pCompositionTargetRenderedEvent != NULL)
    {
        pEventSource = m_pCompositionTargetRenderedEvent;
    }
    else
    {
        hr = ctl::ComObject<CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>,  IInspectable, xaml_media::IRenderedEventArgs>>::CreateInstance(&pEventSource);
        IFC(hr);
        pEventSource->Initialize(KnownEventIndex::CompositionTarget_Rendered, NULL, FALSE /*bCoreEvent*/
            #if DBG
            , TRUE // This is a static event
            #endif
            );
        m_pCompositionTargetRenderedEvent = pEventSource;
    }

    *ppEventSource = pEventSource;
    ctl::addref_interface(pEventSource);

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::OnSurfaceContentsLostEvent(_In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource = NULL;
    IFC(GetSurfaceContentsLostEventSource(&pEventSource));
    IFC(pEventSource->Raise(NULL, pArgs));

Cleanup:
    ctl::release_interface(pEventSource);
    return hr;
}


_Check_return_ HRESULT DXamlCore::GetSurfaceContentsLostEventSource(_Outptr_
    CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>** ppEventSource)
{
    HRESULT hr = S_OK;
    CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* pEventSource;

    if (m_pCompositionTargetSurfaceContentsLostEvent != NULL)
    {
        pEventSource = m_pCompositionTargetSurfaceContentsLostEvent;
    }
    else
    {
        hr = ctl::ComObject<CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>>::CreateInstance(&pEventSource);
        IFC(hr);
        pEventSource->Initialize(KnownEventIndex::CompositionTarget_SurfaceContentsLost, NULL, FALSE /*bCoreEvent*/
            #if DBG
            , TRUE // This is a static event
            #endif
            );
        m_pCompositionTargetSurfaceContentsLostEvent = pEventSource;
    }

    *ppEventSource = pEventSource;
    ctl::addref_interface(pEventSource);

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::OnFocusedElementRemovedEvent(_In_ IFocusedElementRemovedEventArgs* pArgs)
{
    ctl::ComPtr<CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>> eventSource;

    IFC_RETURN(GetFocusedElementRemovedEventSource(eventSource.ReleaseAndGetAddressOf()));
    IFC_RETURN(eventSource->Raise(nullptr, pArgs));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetFocusedElementRemovedEventSource(_Outptr_
    CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>** ppEventSource)
{
    CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>* eventSource = nullptr;

    if (m_focusedElementRemovedEvent != nullptr)
    {
        eventSource = m_focusedElementRemovedEvent;
    }
    else
    {
        IFC_RETURN((ctl::ComObject<CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>>::CreateInstance(&eventSource)));
        eventSource->Initialize(KnownEventIndex::FocusManager_FocusedElementRemoved, nullptr, FALSE /*bCoreEvent*/
#if DBG
            , TRUE // This is a static event
#endif
            );

        m_focusedElementRemovedEvent = eventSource;
    }

    *ppEventSource = eventSource;
    ctl::addref_interface(eventSource);

    return S_OK;
}


_Check_return_ HRESULT DXamlCore::RunCoreWindowMessageLoop()
{
    HRESULT hr = S_OK;
    HRESULT recordHr = S_OK;

    IFCEXPECT(m_pControl);
    IFC(m_pControl->RunCoreWindowMessageLoop());

    // We want to disconnect from UIA forcefully here, we must call this API before any cleanup as it starts pumping messages and utilise re-entrancy.
    if (m_pControl)
    {
        m_pControl->DisconnectUIA();
    }

    // for now, do some deinit here until we have a better story for when deinit happens
    // media deinit code that runs when the visual tree is torn down will fail when we do the real deinit inside DllMain
    IFC(m_pControl->ResetVisualTree());
    IFC(ClearCaches());

    ReleaseInterface(m_pDispatcher);

    // Release the Window and its content, we no longer need it and releasing it now may allow us to clean up the DXamlCore for this thread
    //
    // TODO: We should revisit this and probably refactor. We used to have a design where the DXamlCore lifetime was controlled
    // by having any outstanding objects. In that design, we released the window here because it's possible that the window and its content
    // are the last outstanding objects, and this would let us cleanup the core. However, we no longer have that design - the DXamlCore
    // lifetime is explicitly controlled. We should just deinitialize the window as part of deinitializing the core. Doing so would allow
    // controls to still reference the window instance during core deinit and not need a null check.
    //
    if (m_uwpWindowNoRef)
    {
        IFC(m_uwpWindowNoRef->put_Content(NULL));
    }

    // Unregister the touch hit testing handler now, because our reference to the CoreWindow is about to be released.
    if (m_uwpWindowNoRef && m_pTouchHitTestingHandler && WindowType::CoreWindow == m_uwpWindowNoRef->GetWindowType())
    {
        ctl::ComPtr<wuc::ICoreWindow> spCoreWindow;
        IFC(m_uwpWindowNoRef->get_CoreWindow(&spCoreWindow));
        IFC(m_pTouchHitTestingHandler->Unregister(spCoreWindow.Get()));
    }

    IFC(recordHr);

Cleanup:
    ReleaseWindow();

    return hr;
}

_Check_return_ HRESULT DXamlCore::ConfigureJupiterWindow(_In_opt_ wuc::ICoreWindow* pCoreWindow)
{
    auto cleanup = wil::scope_exit([&]()
    {
        TraceCreateWindowEnd();
    });

    TraceCreateWindowBegin();

    IFCEXPECT_RETURN(m_pControl);
    IFC_RETURN(m_pControl->ConfigureJupiterWindow(pCoreWindow));

    if(pCoreWindow)
    {
        IFC_RETURN(SetCoreWindow(pCoreWindow));
    }
    else // CoreWindow does not exist in Win32/ Islands
    {
        IFC_RETURN(XamlPalSetCoreWindow());
    }

    return S_OK;
}

wgrd::IDisplayInformation* DXamlCore::GetDisplayInformationNoRef()
{
    if (!m_isDisplayInformationInitialized)
    {
        // In Desktop/ Island without corewindow/ coreview, GetForCurrentView won't work.
        // TODO: Once we enable XamlOneCoreTransforms, we will need to address DisplayInformation with alternative in order to support Islands on WCOS
        ctl::ComPtr<wgrd::IDisplayInformationStatics> displayInformationStatics;
        ctl::ComPtr<wgrd::IDisplayInformation> displayInformation;
        if (SUCCEEDED(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &displayInformationStatics)))
        {
            IGNOREHR(displayInformationStatics->GetForCurrentView(&m_displayInformation));
        }

        m_isDisplayInformationInitialized = true;
    }
    return m_displayInformation.Get();
}

UINT GetSystemDpi()
{
    HDC hDC = GetDC(nullptr);
    UINT systemDpi = hDC ? GetDeviceCaps(hDC, LOGPIXELSY) : 96;
    if (hDC)
        ReleaseDC(nullptr, hDC);

    return systemDpi;
}

_Check_return_ HRESULT DXamlCore::SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow)
{
    ctl::ComPtr<ICoreWindowInterop> spCoreWindowInterop;
    wrl::ComPtr<DebugTool::IInternalDebugInterop> spDebugInterop;
    HWND hwndCoreWindow {};
    wrl::ComPtr<msy::IDispatcherQueueStatics> queueStatics;

    // Set the CoreWindow into the Jupiter Window held by this core.
    IFCEXPECT_RETURN(m_uwpWindowNoRef);

    IFC_RETURN(m_uwpWindowNoRef->SetCoreWindow(pCoreWindow));

    if (TouchHitTestingHandler::IsTouchHitTestingEnabled(GetHandle()))
    {
        // Create and register the TouchHitTestingHandler for this core.
        m_pTouchHitTestingHandler = new TouchHitTestingHandler();
        IFC_RETURN(m_pTouchHitTestingHandler->Register(pCoreWindow));
    }

    // Cache the CoreDispatcher. The CoreDispatcher is free-threaded (the CoreWindow is not)
    // so we can cache it and just smuggle the pointer to other apartments when we need it.
    IFCEXPECT_RETURN(!m_pDispatcher);
    IFC_RETURN(pCoreWindow->get_Dispatcher(&m_pDispatcher));

    IFC_RETURN(ctl::do_query_interface(spCoreWindowInterop, pCoreWindow));
    IFC_RETURN(spCoreWindowInterop->get_WindowHandle(&hwndCoreWindow));

    // Initiates the launch of XamlDiagnostics.
    if (RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics))
    {
        auto interop = ::Diagnostics::GetDiagnosticsInterop(true);
        interop->Launch(GetDispatcherQueueNoRef());
    }
    else
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetDebugInterop(&spDebugInterop));
        if (spDebugInterop && spDebugInterop->GetDebugToolNoRef())
        {
            // We need to set the core dispatcher here, there can be a race condition where the core window
            // is not created the first time the debug tool is created so the dispatcher is null.
            IGNOREHR(spDebugInterop->GetDebugToolNoRef()->SetCoreDispatcher(GetCoreDispatcherNoRef()));
            IGNOREHR(spDebugInterop->GetDebugToolNoRef()->Launch(static_cast<HWND>(m_pControl->GetXcpControlWindow()), DebugTool::EnvironmentMap()));
        }
    }

    IFC_RETURN(XamlPalSetCoreWindow());

    m_hCore->GetInputServices()->SetCoreWindow(pCoreWindow);

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::InitializeImpl(_In_ InitializationType initializationType)
{
    DXamlInstanceStorage::Handle hInstance = NULL;
    IFC_RETURN(DXamlInstanceStorage::GetValue(&hInstance));

    DXamlCore* pCore = static_cast<DXamlCore*>(hInstance);

    if (pCore)
    {
        const auto state = pCore->GetState();

        // If there already is a DXamlCore instance for this thread then ensure it's initialized
        // or that we are idle and that's how we want to initialize.
        IFCEXPECT_ASSERT_RETURN(DXamlCore::Initialized == state || DXamlCore::Idle == state && InitializationType::FromIdle == initializationType)
    }
    else
    {
        pCore = new DXamlCore();
    }

    // Don't do anything if we are already initialized
    if (pCore->GetState() != DXamlCore::Initialized)
    {
        // if instance initialization failed, we need to undo what was done
        // Note that destructor too accesses the instance and must be available through
        // GetCurrent() - even though it was not successfully initialized
        auto undoOnFailure = wil::scope_exit([&pCore]() {
            delete pCore;
            pCore = nullptr;
            VERIFYHR(DXamlInstanceStorage::SetValue(NULL));
        });

        // The ReferenceTrackerManager is a singleton that's shared between threads, so we need to hold a reference to
        // protect it from going away (in case another thread calls ReferenceTrackerManager::UnregisterCore before we call
        // RegisterCore).
        ctl::ComPtr<ReferenceTrackerManager> keepAlive;
        IFC_RETURN(ReferenceTrackerManager::EnsureInitialized(&keepAlive));

        // Note that during InitializeInstance, the instance must be available through
        // GetCurrent() - even though it isn't fully initialized yet. That's because
        // many code paths called from InitializeInstance use GetCurrent().
        // If initializing from an idle state, then we already have set the value in
        // DXamlInstanceStorage
        if (InitializationType::FromIdle != initializationType)
        {
            IFC_RETURN(DXamlInstanceStorage::SetValue(pCore));
        }

        if (initializationType == InitializationType::FromIdle)
        {
            IFC_RETURN(pCore->InitializeInstanceFromIdle());
        }
        else if (initializationType != InitializationType::Xbf)
        {
            IFC_RETURN(pCore->InitializeInstance(initializationType));
        }
        else
        {
            IFC_RETURN(pCore->InitializeInstanceForXbf());
        }

        // Now that we have a good core, we can register with the ReferenceTrackerManager
        if (InitializationType::FromIdle != initializationType)
        {
            IFC_RETURN(ReferenceTrackerManager::RegisterCore(pCore));
        }

        // Initialize simple property callbacks for CUIElement.  This needs to happen regardless of initialization type,
        // including from idle, since when the core is deinitialized to idle it destroys all simple properties and their callback tables.
        CUIElement::RegisterSimplePropertyCallbacks();

        // at this point we have a properly initialized instance
        IFCEXPECT_ASSERT_RETURN(DXamlCore::Initialized == pCore->GetState());

        // We can dismiss the guard now that we know we have successfully initialized
        undoOnFailure.release();
    }
    else
    {
        // Tests shut down by calling DeinitializeInstanceToIdle on the DXamlCore, which will deinitialize the
        // DCompSurfaceFactoryManager. The intent is for the next test to pick up the same DXamlCore and call
        // InitializeInstanceFromIdle on it, which reinitializes the DCompSurfaceFactoryManager.
        //
        // WPF tests can can initialize the DXamlCore on a separate UI thread, which means they don't find the
        // previous DXamlCore to call InitializeInstanceFromIdle. They also don't go through DllMain again, so
        // make sure that the DCompSurfaceFactoryManager is initialized.
        DCompSurfaceFactoryManager::EnsureInitialized();
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  RegisterVisualDiagnosticsPort:
//      Registers a system wide message with core messaging.
//      Diagnostics tools like Visual Studio will attempt to set up initial
//      handshake with the app using this message port.
//      The first time this is called, also creates and signals an event that
//      Visual Studio listens for, letting them know they are dealing
//      with a WinUI app and that it's debuggable.
//-------------------------------------------------------------------------
_Check_return_ HRESULT DXamlCore::RegisterVisualDiagnosticsPort()
{
    IFC_RETURN(StringCchPrintf(m_pszVisualDiagEndpointName, _countof(m_pszVisualDiagEndpointName), L"%s%d", L"WinUIVisualDiagConnection", s_instanceCount++));
    IFCFAILFAST(VisualDiagnosticsPort_Register(m_pszVisualDiagEndpointName, DXamlCore::OnVisualDiagInit, this));
    if (s_diagnosticToolingEvent == nullptr)
    {
        WCHAR eventName[100];
        IFC_RETURN(StringCchPrintf(eventName, _countof(eventName), L"%s%d", L"WinUIDebuggable", GetCurrentProcessId()));

        // Create the event so it defaults to true, and that when a waiter is released the event stays signalled.  We want
        // this event to be valid for the lifetime of the process, and VS may query the event multiple times.
        // VS doesn't keep a handle open for the event after checking it (with an instant timeout), so the event
        // will be automatically closed when the WinUI process terminates since it's the only process
        // that keeps its handle open.
        s_diagnosticToolingEvent = CreateEvent(nullptr /*eventAttributes*/ , TRUE /*manualReset*/, TRUE /*initialState*/, eventName /*eventName*/ );
        IFCEXPECT_ASSERT_RETURN(s_diagnosticToolingEvent != nullptr);
    }
    return S_OK;
}

void DXamlCore::UnregisterVisualDiagnosticsPort()
{
    VisualDiagnosticsPort_Unregister();
}

//-------------------------------------------------------------------------
//
//  OnVisualDiagInit:
//      Main entry point for Visual Diagnostics API
//      This routine gets called by Core Messaging in response to a post on
//      the port opened through RegisterVisualDiagnosticsPort
//-------------------------------------------------------------------------
_Check_return_ HRESULT CALLBACK
DXamlCore::OnVisualDiagInit(_In_ void * userContext, _In_ const void * pPayload, _In_ int size)
{
    DXamlCore * pCore = static_cast<DXamlCore *>(userContext);

    if (size != sizeof(DebugTool::Msg_ConnectToVisualTree))
    {
        ASSERT(FALSE && L"Should have valid size");
        return E_FAIL;
    }

    const DebugTool::Msg_ConnectToVisualTree* pMsg = DebugTool::Msg_ConnectToVisualTree::SafeCast(pPayload, size);

    // Try launching Xaml Diagnostics.
    IGNOREHR(DebugTool::CreateDiagnostics(pCore->m_pControl, pMsg));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::Initialize(_In_ InitializationType initializationType)
{
    return DXamlCore::InitializeImpl(initializationType);
}

_Check_return_ HRESULT DXamlCore::Deinitialize(_In_ DeinitializationType deinitializeType)
{
    HRESULT hr = S_OK;

    DXamlInstanceStorage::Handle hInstance = NULL;
    DXamlCore* pCore = NULL;
    ::IReferenceTrackerManager *pReferenceTrackerManager = NULL;

    IFC(DXamlInstanceStorage::GetValue(&hInstance));

    if (!hInstance)
    {
        // No instance stored on this thread - nothing to do.
        goto Cleanup;
    }

    pCore = static_cast<DXamlCore*>(hInstance);

    // We must be in one of these states when Deinitialize is called.
    IFCEXPECT_ASSERT(DXamlCore::Initialized          == pCore->GetState() ||
                     DXamlCore::InitializationFailed == pCore->GetState() ||
                     DXamlCore::Idle                 == pCore->GetState());

    // We need to keep a reference alive to the ReferenceTrackerManager as it
    // will uninitialize the callbacks if this were the last core. We need the callbacks
    // for OnShutdownAllPeers callback to work in DeinitializeInstance.

    if (deinitializeType == DeinitializationType::Default)
    {
        pReferenceTrackerManager = ReferenceTrackerManager::GetNoRef();
        AddRefInterface(pReferenceTrackerManager);

        IFC(pCore->DeinitializeInstance());
        VERIFYHR(ReferenceTrackerManager::UnregisterCore(pCore));

        delete pCore;
        pCore = NULL;
        IFC(DXamlInstanceStorage::SetValue(NULL));
    }
    else if (deinitializeType == DeinitializationType::ToIdle)
    {
        IFC(pCore->DeinitializeInstanceToIdle());
    }

    DependencyLocator::UninitializeThread();

Cleanup:
    ReleaseInterface(pReferenceTrackerManager);
    return hr;
}

bool DXamlCore::IsInitializingStatic()
{
    const DXamlCore* core = GetCurrent();
    return core && core->IsInitializing();
}

bool DXamlCore::IsIdleStatic()
{
    DXamlCore* pCore = GetCurrent();
    return pCore && pCore->IsIdle();
}

bool DXamlCore::IsInitializedStatic()
{
    DXamlCore* pCore = GetCurrent();
    if (pCore && pCore->IsInitialized())
    {
        return true;
    }

    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (pCore && runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown))
    {
        return pCore->IsIdle();
    }

    return false;
}

bool DXamlCore::IsShutdownStatic()
{
    DXamlCore* pCore = GetCurrent();
    return !pCore || pCore->IsShutdown();
}

bool DXamlCore::IsShuttingDownStatic()
{
    DXamlCore* pCore = GetCurrent();
    return !pCore || pCore->IsShuttingDown();
}

DXamlCore* DXamlCore::GetCurrent()
{
    DXamlInstanceStorage::Handle hInstance = NULL;

    // During GC we should never get the thread's core, GC can run on any thread, and getting that thread's
    // core is a sign of a problem.
    ReferenceTrackerManager::AssertActive(FALSE);

    VERIFYHR(DXamlInstanceStorage::GetValue(&hInstance));

    return static_cast<DXamlCore*>(hInstance);
}

DXamlCore* DXamlCore::GetCurrentNoCreate()
{
    return GetCurrent();
}

// Given a DependencyObject, get back the DXamlCore
DXamlCore* DXamlCore::GetFromDependencyObject( DependencyObject *pDO )
{
    // If this DO doesn't have a core peer, we can't get the DXamlCore.
    // We should have a core peer, though, so that there's an entry in the peer table.
    // Resolve this as part of bug #101268.
    if( pDO->GetHandle() == NULL)
        return NULL;

    XHANDLE context = pDO->GetHandle()->GetContext()->GetFrameworkContext();

    // Note that it's valid for context to be NULL. When DXamlCore is deinitialized
    // it sets the framework context associated CCoreServices to NULL.

    return reinterpret_cast<DXamlCore*>(context);
}

ElementSoundPlayerService* DXamlCore::TryGetElementSoundPlayerServiceNoRef() const
{
    if (m_elementSoundPlayerService)
    {
        // Invoking GetElementSoundPlayerServiceNoRef instead of returning m_elementSoundPlayerService directly
        // to get the benefits of traces and ability to change GetElementSoundPlayerServiceNoRef without TryGetElementSoundPlayerServiceNoRef.
        return const_cast<DXamlCore*>(this)->GetElementSoundPlayerServiceNoRef();
    }
    else
    {
        return nullptr;
    }
}

ElementSoundPlayerService* DXamlCore::GetElementSoundPlayerServiceNoRef()
{
    auto cleanup = wil::scope_exit([&]()
    {
        TraceGetSoundPlayerServiceEnd();
    });

    TraceGetSoundPlayerServiceBegin();

    if (!m_elementSoundPlayerService)
    {
        IFCFAILFAST(ctl::make(&m_elementSoundPlayerService));

        // Special root on DXamlCore
        m_elementSoundPlayerService->UpdatePeg(true);
    }

    return m_elementSoundPlayerService.Get();
}


_Check_return_ HRESULT DXamlCore::GetBudgetManager(_Out_ ctl::ComPtr<BudgetManager>& spManager)
{
    if (!m_spBudgetManager.Get())
    {
        IFC_RETURN(ctl::make(&m_spBudgetManager));

        // Special root on DXamlCore
        m_spBudgetManager->UpdatePeg(true);
    }

    spManager = m_spBudgetManager;
    return S_OK;
}


_Check_return_ HRESULT
DXamlCore::GetToolTipServiceMetadata(
    _Out_ ToolTipServiceMetadata*& pToolTipServiceMetadata,
    bool createIfNeeded /*= true*/)
{
    CStaticLock lock;

    if (!m_pToolTipServiceMetadata && createIfNeeded)
    {
        IFC_RETURN(ctl::ComObject<ToolTipServiceMetadata>::CreateInstance(&m_pToolTipServiceMetadata));
    }

    pToolTipServiceMetadata = m_pToolTipServiceMetadata;

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetBuildTreeService(_Out_ ctl::ComPtr<BuildTreeService>& spService)
{
    HRESULT hr = S_OK;

    if (m_spBuildTreeService == NULL)
    {
        IFC(ctl::make(&m_spBuildTreeService));

        // Special root on DXamlCore
        m_spBuildTreeService->UpdatePeg(true);
    }

    spService = m_spBuildTreeService;

Cleanup:
    return hr;
}

_Check_return_ HRESULT DXamlCore::GetUISettings(_Out_ ctl::ComPtr<wuv::IUISettings>& spUISettings)
{
    if (m_spUISettings == nullptr)
    {
        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), m_spUISettings.ReleaseAndGetAddressOf()));
    }

    spUISettings = m_spUISettings;
    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetFlyoutMetadata(
    _Out_ FlyoutMetadata** ppFlyoutMetadata)
{
    HRESULT hr = S_OK;

    if (!m_spFlyoutMetadata)
    {
        IFC(ctl::make(&m_spFlyoutMetadata));
        m_spFlyoutMetadata->UpdatePeg(true);
    }

    IFC(m_spFlyoutMetadata.CopyTo(ppFlyoutMetadata));

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::GetRadioButtonGroupsByName(
    _In_ bool ensure,
    _Out_ xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*>*& pRadioButtonGroupsByName)
{
    if (!m_pRadioButtonGroupsByName && ensure)
    {
        m_pRadioButtonGroupsByName = new xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*>;
    }
    pRadioButtonGroupsByName = m_pRadioButtonGroupsByName;
    return S_OK;
}

void DXamlCore::RegisterInputPaneHandler(_In_ wuc::ICoreWindow * coreWindow)
{
    CContentRoot* contentRoot = DXamlCore::GetCurrent()->GetHandle()->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    contentRoot->GetInputManager().RegisterInputPaneHandler(coreWindow);
}

void DXamlCore::OnWindowDestroyed(_In_ HWND hwndDestroyed)
{
    if (auto inputServices = DXamlCore::GetCurrent()->GetHandle()->GetInputServices())
    {
        inputServices->NotifyWindowDestroyed(hwndDestroyed);
    }
}

void DXamlCore::PhysicalScreenToClient(_Inout_ POINT* physicalPoint)
{
    m_pControl->ScreenToClient(physicalPoint);
}

void DXamlCore::ScreenToClient(_Inout_ wf::Point* pDipPoint)
{
    POINT pixelPoint;

    const auto contentRootCoordinator = GetHandle()->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);

    DipsToPhysicalPixels(scale, pDipPoint, &pixelPoint);
    m_pControl->ScreenToClient(&pixelPoint);
    PhysicalPixelsToDips(scale, &pixelPoint, pDipPoint);
}

void DXamlCore::ClientToScreen(_Inout_ wf::Point* pDipPoint)
{
    POINT pixelPoint;

    const auto contentRootCoordinator = GetHandle()->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);

    DipsToPhysicalPixels(scale, pDipPoint, &pixelPoint);
    m_pControl->ClientToScreen(&pixelPoint);
    PhysicalPixelsToDips(scale, &pixelPoint, pDipPoint);
}

void DXamlCore::ScreenToClient(_Inout_ wf::Rect* pDipRect)
{
    XamlOneCoreTransforms::FailFastIfEnabled(); // Due to ScreenToClient call

    wf::Point topLeft = {pDipRect->X, pDipRect->Y};
    ScreenToClient(&topLeft);
    pDipRect->X = topLeft.X;
    pDipRect->Y = topLeft.Y;
}

void DXamlCore::ClientToScreen(_Inout_ wf::Rect* pDipRect)
{
    XamlOneCoreTransforms::FailFastIfEnabled(); // Due to ClientToScreen call

    wf::Point topLeft = {pDipRect->X, pDipRect->Y};
    ClientToScreen(&topLeft);
    pDipRect->X = topLeft.X;
    pDipRect->Y = topLeft.Y;
}

void DXamlCore::ScreenToClient(_Inout_ RECT* pPhysicalPixelsRect)
{
    m_pControl->ScreenToClient(reinterpret_cast<POINT*>(&pPhysicalPixelsRect->left));
    m_pControl->ScreenToClient(reinterpret_cast<POINT*>(&pPhysicalPixelsRect->right));
}

void DXamlCore::ClientToScreen(_Inout_ RECT* pPhysicalPixelsRect)
{
    m_pControl->ClientToScreen(reinterpret_cast<POINT*>(&pPhysicalPixelsRect->left));
    m_pControl->ClientToScreen(reinterpret_cast<POINT*>(&pPhysicalPixelsRect->right));
}

void DXamlCore::DipsToPhysicalPixels(_In_ float scale, _In_ wf::Point* pDipPoint, _Out_ POINT* pPhysicalPoint)
{
    pPhysicalPoint->x = static_cast<LONG>(pDipPoint->X * scale);
    pPhysicalPoint->y = static_cast<LONG>(pDipPoint->Y * scale);
}

void DXamlCore::PhysicalPixelsToDips(_In_ float scale, _In_ POINT* pPhysicalPoint, _Out_ wf::Point* pDipPoint)
{
    pDipPoint->X = pPhysicalPoint->x / scale;
    pDipPoint->Y = pPhysicalPoint->y / scale;
}

void DXamlCore::PhysicalPixelsToDips(_In_ float scale, _In_ SIZE* pSize, _Out_ wf::Rect* pRect)
{
    pRect->X = 0;
    pRect->Y = 0;
    pRect->Width = pSize->cx / scale;
    pRect->Height = pSize->cy / scale;
}

void DXamlCore::PhysicalPixelsToDips(_In_ float scale, _In_ RECT* pRectIn, _Out_ wf::Rect* pRectOut)
{
    pRectOut->X      = static_cast<FLOAT>(pRectIn->left) / scale;
    pRectOut->Y      = static_cast<FLOAT>(pRectIn->top) / scale;
    pRectOut->Width  = static_cast<FLOAT>(pRectIn->right - pRectIn->left) / scale;
    pRectOut->Height = static_cast<FLOAT>(pRectIn->bottom - pRectIn->top) / scale;
}

void DXamlCore::PhysicalPixelsToDips(_In_ float scale, _In_ wf::Point* pPhysicalPoint, _Out_ wf::Point* pDipPoint)
{
    pDipPoint->X = pPhysicalPoint->X / scale;
    pDipPoint->Y = pPhysicalPoint->Y / scale;
}

void DXamlCore::PhysicalPixelsToDips(_In_ float scale, _In_ wf::Rect* pPhysicalRect, _Out_ wf::Rect* pDipRect)
{
    pDipRect->X      = pPhysicalRect->X / scale;
    pDipRect->Y      = pPhysicalRect->Y / scale;
    pDipRect->Width  = pPhysicalRect->Width / scale;
    pDipRect->Height = pPhysicalRect->Height / scale;
}

void DXamlCore::DipsToPhysicalPixels(_In_ float scale, _In_ wf::Rect* pDipRect, _Out_ wf::Rect* pPhysicalRect)
{
    pPhysicalRect->X      = pDipRect->X * scale;
    pPhysicalRect->Y      = pDipRect->Y * scale;
    pPhysicalRect->Width  = pDipRect->Width * scale;
    pPhysicalRect->Height = pDipRect->Height * scale;
}

void DXamlCore::DipsToPhysicalPixels(_In_ float scale, _In_ wf::Point* pDipPoint, _Out_ wf::Point* pPhysicalPoint)
{
    pPhysicalPoint->X = pDipPoint->X * scale;
    pPhysicalPoint->Y = pDipPoint->Y * scale;
}

void DXamlCore::QueueObjectForUnreachableCleanup(_In_ ctl::WeakReferenceSourceNoThreadId *pItem)
{
    if (m_spReleaseQueue != NULL)
    {
        m_spReleaseQueue->QueueUnreachableCleanup(pItem);
    }
}

void DXamlCore::QueueObjectForFinalRelease(_In_ ctl::WeakReferenceSourceNoThreadId *pItem)
{
    if (m_spReleaseQueue != NULL)
    {
        m_spReleaseQueue->QueueFinalRelease(pItem);
    }
}

void DXamlCore::ReleaseQueuedObjects( BOOLEAN bSync )
{
#if DBG
    if (m_hCore)
    {
        XUINT32 nThreadID = m_hCore->GetThreadID();
        ASSERT(nThreadID == ::GetCurrentThreadId());
    }
#endif

    if (m_spReleaseQueue != NULL)
    {
        m_spReleaseQueue->Cleanup(bSync);
    }
}

void DXamlCore::ReferenceTrackerWalkOnCoreGCRoots(_In_ EReferenceTrackerWalkType walkType)
{
    GetHandle()->ReferenceTrackerWalkOnCoreGCRoots(walkType);
}

_Check_return_ HRESULT DXamlCore::OnBeforeAppSuspend()
{
    HRESULT hr = S_OK;

    if (m_pControl)
    {
        IFC(m_pControl->SetTicksEnabled(false));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT DXamlCore::OnAfterAppSuspend()
{
    HRESULT hr = S_OK;

    // We request additional suspend time only for managed apps because
    // GC is an asynchronous operation that can be slow.
    if (ReferenceTrackerManager::HaveHost())
    {
        // Do a GC and wait for finalizers.
        IGNOREHR(ReferenceTrackerManager::TriggerCollectionForSuspend());
        IGNOREHR(ReferenceTrackerManager::TriggerFinalization());
    }

    // Flush the release queue
    // We make this call for both native and managed apps because the queue
    // is not guaranteed to be empty for native apps (for example, if some
    // objects were recently released off thread).
    ReleaseQueuedObjects();

    if (m_pControl)
    {
        IFC(m_pControl->GetCoreServices()->OnSuspend(false /* isTriggeredByResourceTimer */, true /* allowOfferResources */));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT DXamlCore::OnBeforeAppResume()
{
    HRESULT hr = S_OK;

    // Enable the ticks, only if the window was not already destroyed.
    if (m_pControl && !m_pControl->IsWindowDestroyed())
    {
        IFC(m_pControl->SetTicksEnabled(true));
    }

    if (m_pControl)
    {
        IFC(m_pControl->GetCoreServices()->OnResume());
    }

Cleanup:
    return hr;
}

//-----------------------------------------------------------------------------
//
// Gets the XAML dispatcher associated with this core.
//
//-----------------------------------------------------------------------------
IDispatcher* DXamlCore::GetXamlDispatcherNoRef()
{
    return m_spDispatcherImpl.Get();
}

//-----------------------------------------------------------------------------
//
// Gets the XAML dispatcher associated with this core.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT DXamlCore::GetXamlDispatcher(_Out_ ctl::ComPtr<IDispatcher>* pspDispatcher)
{
    *pspDispatcher = m_spDispatcherImpl;
    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::GetDataContextChangedEventArgsFromPool(_In_ IInspectable* pNewDataContext, _Outptr_ xaml::IDataContextChangedEventArgs** ppValue)
{
    HRESULT hr = S_OK;

    if (static_cast<ctl::ComBase*>(m_spDataContextChangedEventArgs.Cast<DataContextChangedEventArgs>())->GetRefCount() > 1)
    {
        // The application is holding on to the object, likely because it's doing something
        // asynchronous. Create a new instance.
        ctl::ComPtr<DataContextChangedEventArgs> spDataContextChangedEventArgs;
        IFC(ctl::make<DataContextChangedEventArgs>(&spDataContextChangedEventArgs));
        m_spDataContextChangedEventArgs.Attach(spDataContextChangedEventArgs.Detach());
    }

    IFC(m_spDataContextChangedEventArgs.Cast<DataContextChangedEventArgs>()->put_NewValue(pNewDataContext));
    IFC(m_spDataContextChangedEventArgs->put_Handled(FALSE));

    IFC(m_spDataContextChangedEventArgs.CopyTo(ppValue));

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::ReleaseDataContextChangedEventArgsToPool(_In_ xaml::IDataContextChangedEventArgs* pValue)
{
    HRESULT hr = S_OK;

    // 1 ref from DXamlCore + 1 ref from the caller = 2 refs we expect the most to allow
    // this object back in the pool. If there are more refs, the application is holding on to
    // the object (maybe doing something async, or caching it in a field).
    if (static_cast<ctl::ComBase*>(static_cast<DataContextChangedEventArgs*>(pValue))->GetRefCount() <= 2)
    {
        // The object may stay in the pool. Make sure we clear out its state though to avoid
        // keeping random state around for too long.
        IFC(static_cast<DataContextChangedEventArgs*>(pValue)->put_NewValue(nullptr));
    }
    else
    {
        // The application is referencing the object... Remove the instance from the pool and
        // create a new instance in the pool.
        ctl::ComPtr<DataContextChangedEventArgs> spDataContextChangedEventArgs;
        IFC(ctl::make<DataContextChangedEventArgs>(&spDataContextChangedEventArgs));
        m_spDataContextChangedEventArgs.Attach(spDataContextChangedEventArgs.Detach());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::GetVectorChangedEventArgsFromPool(_Outptr_ VectorChangedEventArgs** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VectorChangedEventArgs> spVectorChangedEventArgs;

    if (static_cast<ctl::ComBase*>(m_spVectorChangedEventArgs.Cast<VectorChangedEventArgs>())->GetRefCount() > 1)
    {
        // The framework is holding on to the object, likely because it's re-raising collection changed event.
        // Create a new instance.
        IFC(ctl::make<VectorChangedEventArgs>(&spVectorChangedEventArgs));
        m_spVectorChangedEventArgs = spVectorChangedEventArgs;
    }
    else
    {
        // that's what we do in c~tor of VectorChangedEventArgs
        IFC(m_spVectorChangedEventArgs.Cast<VectorChangedEventArgs>()->put_CollectionChange(wfc::CollectionChange_Reset));
        IFC(m_spVectorChangedEventArgs.Cast<VectorChangedEventArgs>()->put_Index(0));
        spVectorChangedEventArgs = m_spVectorChangedEventArgs.Cast<VectorChangedEventArgs>();
    }

    IFC(spVectorChangedEventArgs.MoveTo(ppValue));

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DXamlCore::ReleaseVectorChangedEventArgsToPool(_In_ VectorChangedEventArgs* pValue)
{
    HRESULT hr = S_OK;

    // 1 ref from DXamlCore + 1 ref from the caller = 2 refs we expect the most to allow
    // this object back in the pool. If there are more refs, the framework is holding on to
    // the object, likely because it's re-raising collection changed event.
    if (static_cast<ctl::ComBase*>(pValue)->GetRefCount() <= 2)
    {
        // The object may stay in the pool. Make sure we clear out its state though to avoid
        // keeping random state around for too long.
        IFC(pValue->put_CollectionChange(wfc::CollectionChange_Reset));
        IFC(pValue->put_Index(0));
    }
    else
    {
        // The framework is referencing the object... Remove the instance from the pool and
        // create a new instance in the pool.
        ctl::ComPtr<VectorChangedEventArgs> spVectorChangedEventArgs;
        IFC(ctl::make<VectorChangedEventArgs>(&spVectorChangedEventArgs));
        m_spVectorChangedEventArgs.Attach(spVectorChangedEventArgs.Detach());
    }

Cleanup:
    return hr;
}

void DXamlCore::TrySetGenericXamlFilePathFromMUX(const xstring_ptr_view& strUri)
{
    xref_ptr<IPALUri> uri;

    // Create appx scheme URI from string
    IGNOREHR(gps->UriCreate(strUri.GetCount(), strUri.GetBuffer(), uri.ReleaseAndGetAddressOf()));

    xref_ptr<IPALResourceManager> resourceManager;
    IGNOREHR(DXamlCore::GetCurrent()->GetHandle()->GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
    ASSERT(resourceManager != nullptr);

    xref_ptr<IPALResource> resource;
    IGNOREHR(resourceManager->TryGetLocalResource(uri, resource.ReleaseAndGetAddressOf()));

    if (resource)
    {
        IGNOREHR(resource->TryGetFilePath(&m_genericXamlFilePathFromMUX));
        m_hCore->SetIsUsingGenericXamlFilePath(true);
    }

    // Only check if the file exists once.
    m_shouldCheckGenericXamlFilePathFromMUX = false;
}

_Check_return_ HRESULT DXamlCore::ActivateWindow()
{
    IFC_RETURN(m_pControl->ActivateWindow());
    SignalWindowMutation(VisualMutationType::Add);
    return S_OK;
}

_Check_return_ HRESULT DXamlCore::NotifyFirstFramePending()
{
    return m_pControl->NotifyFirstFramePending();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes XamlPal, to be called after XAML has received a
//      CoreWindow.  Also opens some events for automation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
DXamlCore::XamlPalSetCoreWindow()
{
    HRESULT hr = S_OK;

    IFC(m_hCore->InitWaitForIdleEvents());

    // Open PageNavigation complete Named event created by TestAutomationHelper class.
    IFC(RetrievePageNavigationCompleteEvent());

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Retrieves the page navigation complete event,
//      which are created by TestAutomationHelper.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
DXamlCore::RetrievePageNavigationCompleteEvent()
{
    HRESULT hr = S_OK;

    IFC(gps->NamedEventCreate(
        &m_pPageNavigationCompleteEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"PageNavigationComplete"),
        FALSE /* bReturnFailureIfCreationFailed */));

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a value indicating whether or not the page navigation
//      complete event has been successfully retrieved.
//
//------------------------------------------------------------------------
bool
DXamlCore::HasPageNavigationCompleteEvent()
{
    return m_pPageNavigationCompleteEvent != NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the page navigation complete event, if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
DXamlCore::SetPageNavigationCompleteEvent()
{
    HRESULT hr = S_OK;

    if (m_pPageNavigationCompleteEvent != NULL)
    {
        IFC(m_pPageNavigationCompleteEvent->Set());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to set the atlas size hint
//      for this instance of framework.
//
//------------------------------------------------------------------------
void
DXamlCore::SetAtlasSizeHint(XUINT32 width, XUINT32 height)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices)
    {
        IXcpBrowserHost* pBrowserHost = pCoreServices->GetBrowserHost();
        if (pBrowserHost)
        {
            WindowsGraphicsDeviceManager* deviceManagerNoRef = pBrowserHost->GetGraphicsDeviceManager();
            if (deviceManagerNoRef)
            {
                deviceManagerNoRef->SetAtlasSizeHint(width, height);
            }
        }
    }
}

void
DXamlCore::ResetAtlasSizeHint()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices)
    {
        IXcpBrowserHost* pBrowserHost = pCoreServices->GetBrowserHost();
        if (pBrowserHost)
        {
            WindowsGraphicsDeviceManager* deviceManagerNoRef = pBrowserHost->GetGraphicsDeviceManager();
            if (deviceManagerNoRef)
            {
                deviceManagerNoRef->ResetAtlasSizeHint();
            }
        }
    }
}

// Returns nullptr if none available
WindowsPresentTarget* DXamlCore::GetCurrentPresentTargetNoRef()
{
    CCoreServices* pCoreServices = GetHandle();
    if (!pCoreServices) { return nullptr; }

    IXcpBrowserHost* pBrowserHost = pCoreServices->GetBrowserHost();
    if (!pBrowserHost) { return nullptr; }

    WindowsGraphicsDeviceManager* deviceManagerNoRef = pBrowserHost->GetGraphicsDeviceManager();
    if (!deviceManagerNoRef) { return nullptr; }

    CWindowRenderTarget* renderTargetNoRef = deviceManagerNoRef->GetRenderTarget();
    if (!renderTargetNoRef) { return nullptr; }

    return renderTargetNoRef->GetPresentTarget();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to disable the atlases
//      for this instance of framework.
//
//------------------------------------------------------------------------
void
DXamlCore::DisableAtlas()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices)
    {
        IXcpBrowserHost* pBrowserHost = pCoreServices->GetBrowserHost();
        if (pBrowserHost)
        {
            WindowsGraphicsDeviceManager* deviceManagerNoRef = pBrowserHost->GetGraphicsDeviceManager();
            if (deviceManagerNoRef)
            {
                deviceManagerNoRef->DisableAtlas();
            }
        }
    }
}

//static
_Check_return_ HRESULT
DXamlCore::ForwardWindowedPopupMessageToJupiterWindow(
    _In_ HWND window,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_opt_ CContentRoot* contentRoot,
    _Out_ LRESULT *pResult)
{
    if (DXamlCore::GetCurrent())
    {
        *pResult =  DXamlCore::GetCurrent()->ForwardWindowedPopupMessageToJupiterWindow(window, message, wParam, lParam, contentRoot);
    }
    else
    {
        *pResult = ::DefWindowProc(window, message, wParam, lParam);
    }

    return S_OK;
}

LRESULT DXamlCore::ForwardWindowedPopupMessageToJupiterWindow(
    _In_ HWND window,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_opt_ CContentRoot* contentRoot)
{
    if (m_pControl)
    {
        return m_pControl->ForwardWindowedPopupMessageToJupiterWindow(window, message, wParam, lParam, contentRoot);
    }
    else
    {
        return ::DefWindowProc(window, message, wParam, lParam);
    }
}

_Check_return_ HRESULT
DXamlCore::CalculateAvailableMonitorRect(
    _In_ UIElement* pTargetElement,
    _In_ wf::Point targetPointClientLogical,
    _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
    _Out_opt_ wf::Point* screenOffset,
    _Out_opt_ wf::Point* targetPointScreenPhysical,
    _Out_opt_ wf::Rect* inputPaneOccludeRectScreenLogical)
{
    return CalculateAvailableMonitorRect(
        pTargetElement->GetHandle(),
        targetPointClientLogical,
        availableMonitorRectClientLogicalResult,
        screenOffset,
        targetPointScreenPhysical,
        inputPaneOccludeRectScreenLogical);
}

// Calculate the available monitor bounds that is the screen Dips.
_Check_return_ HRESULT
DXamlCore::CalculateAvailableMonitorRect(
    _In_ CUIElement* pTargetElement,
    _In_ wf::Point targetPointClientLogical,
    _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
    _Out_opt_ wf::Point* screenOffset,
    _Out_opt_ wf::Point* targetPointScreenPhysical,
    _Out_opt_ wf::Rect* inputPaneOccludeRectScreenLogical)
{
    // This code does not work on WCOS, but theoretically we shouldn't ever get here on WCOS
    XamlOneCoreTransforms::FailFastIfEnabled();

    *availableMonitorRectClientLogicalResult = {};

    if (screenOffset)
    {
        *screenOffset = {};
    }

    if (targetPointScreenPhysical)
    {
        *targetPointScreenPhysical = {};
    }

    if (inputPaneOccludeRectScreenLogical)
    {
        *inputPaneOccludeRectScreenLogical = {};
    }

    VisualTree* visualTree = VisualTree::GetForElementNoRef(pTargetElement);
    IFCEXPECT_RETURN(visualTree);

    const POINT screenOffsetTemp = visualTree->GetScreenOffset();

    if (screenOffset)
    {
        *screenOffset =
        {
            static_cast<float>(screenOffsetTemp.x),
            static_cast<float>(screenOffsetTemp.y)
        };
    }

    float scale = static_cast<float>(visualTree->GetRasterizationScale());
    IFCEXPECT_RETURN(scale != 0.0f);

    const POINT targetPointScreenPhysicalTemp = visualTree->ClientLogicalToScreenPhysical(targetPointClientLogical);

    const RECT rectFrom = {
        targetPointScreenPhysicalTemp.x,    // left
        targetPointScreenPhysicalTemp.y,    // top
        targetPointScreenPhysicalTemp.x,    // right
        targetPointScreenPhysicalTemp.y     // bottom
    };

    if (targetPointScreenPhysical)
    {
        *targetPointScreenPhysical = {
            static_cast<float>(targetPointScreenPhysicalTemp.x),
            static_cast<float>(targetPointScreenPhysicalTemp.y)
        };
    }

    // Get the monitor from the nearest target rect
    HMONITOR hMonitor = MonitorFromRect(&rectFrom, MONITOR_DEFAULTTONEAREST);
    if (hMonitor)
    {
        MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
        if (GetMonitorInfo(hMonitor, &monitorInfo))
        {
            wf::Rect inputPaneOccludeRectScreenLogicalTemp = {};
            IFC_RETURN(GetInputPaneOccludeRect(pTargetElement, &inputPaneOccludeRectScreenLogicalTemp));

            if (inputPaneOccludeRectScreenLogical)
            {
                *inputPaneOccludeRectScreenLogical = inputPaneOccludeRectScreenLogicalTemp;
            }

            wf::Rect availableMonitorRectScreenLogical = {};
            if (visualTree->ShouldConstrainPopupsToWorkArea())
            {
                availableMonitorRectScreenLogical = {
                    monitorInfo.rcWork.left / scale,
                    monitorInfo.rcWork.top / scale,
                    (monitorInfo.rcWork.right - monitorInfo.rcWork.left) / scale,
                    (monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) / scale
                    };
            }
            else
            {
                availableMonitorRectScreenLogical = {
                    monitorInfo.rcMonitor.left / scale,
                    monitorInfo.rcMonitor.top / scale,
                    (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) / scale,
                    (monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) / scale
                    };
            }

            wf::Rect intersectionRectScreenLogical = availableMonitorRectScreenLogical;

            IFC_RETURN(RectUtil::Intersect(intersectionRectScreenLogical, inputPaneOccludeRectScreenLogicalTemp));

            if (intersectionRectScreenLogical.Height > 0)
            {
                availableMonitorRectScreenLogical.Height = availableMonitorRectScreenLogical.Height - intersectionRectScreenLogical.Height;
            }

            availableMonitorRectScreenLogical.Height = static_cast<float>(DoubleUtil::Max(0.0, availableMonitorRectScreenLogical.Height));

            *availableMonitorRectClientLogicalResult = {
                availableMonitorRectScreenLogical.X - (static_cast<float>(screenOffsetTemp.x) / scale),
                availableMonitorRectScreenLogical.Y - (static_cast<float>(screenOffsetTemp.y) / scale),
                availableMonitorRectScreenLogical.Width,
                availableMonitorRectScreenLogical.Height
            };
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::GetInputPaneOccludeRect(
    _In_ UIElement* element,
    _Out_ wf::Rect* inputPaneOccludeRectInDips)
{
    auto xamlRoot = XamlRoot::GetImplementationForElementStatic(element);
    return GetInputPaneOccludeRect(element->GetHandle(), xamlRoot.Get(), inputPaneOccludeRectInDips);
}

_Check_return_ HRESULT
DXamlCore::GetInputPaneOccludeRect(
    _In_ CUIElement* element,
    _Out_ wf::Rect* inputPaneOccludeRectInDips)
{
    ctl::ComPtr<DependencyObject> elementAsDO;
    IFC_RETURN(GetPeer(static_cast<CDependencyObject*>(element), &elementAsDO));
    auto xamlRoot = XamlRoot::GetImplementationForElementStatic(elementAsDO.Get());
    return GetInputPaneOccludeRect(element, xamlRoot.Get(), inputPaneOccludeRectInDips);
}

_Check_return_ HRESULT
DXamlCore::GetInputPaneOccludeRect(
    _In_ CUIElement* element,
    _In_ XamlRoot* xamlRoot,
    _Out_ wf::Rect* inputPaneOccludeRectInDips)
{
    IFCEXPECT_RETURN(element);
    IFCEXPECT_RETURN(xamlRoot);
    IFCPTR_RETURN(inputPaneOccludeRectInDips);

    // Return our simulated occluded rect if it's been set via our test hooks.
    wf::Rect simulatedOccludedRect = xamlRoot->GetSimulatedInputPaneOccludedRect();
    if (simulatedOccludedRect.Width != 0 && simulatedOccludedRect.Height != 0)
    {
        *inputPaneOccludeRectInDips = simulatedOccludedRect;
        return S_OK;
    }

    InputPaneState inputPaneState = InputPaneState::InputPaneHidden;
    const float zoomScale = RootScale::GetRasterizationScaleForElement(element);
    XRECTF inputPaneBounds = {};

    *inputPaneOccludeRectInDips = { 0 };

    IFC_RETURN(CoreImports::Application_GetInputPaneState(
        element,
        &inputPaneState,
        &inputPaneBounds));

    inputPaneOccludeRectInDips->X = inputPaneBounds.X / zoomScale;
    inputPaneOccludeRectInDips->Y = inputPaneBounds.Y / zoomScale;
    inputPaneOccludeRectInDips->Width = inputPaneBounds.Width / zoomScale;
    inputPaneOccludeRectInDips->Height = inputPaneBounds.Height / zoomScale;

    return S_OK;
}

bool DXamlCore::CompositionTarget_HasHandlers()
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>> eventSource;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetRenderingEventSource(&eventSource));
    return eventSource->HasHandlers();
}

AutomaticDragHelper*
DXamlCore::GetAutomaticDragHelper(_In_ UIElement* uielement)
{
    ASSERT(!IsShutdown());

    auto it = m_autoDragHelperMap.find(uielement);

    if (m_autoDragHelperMap.end() != it)
    {
        return it->second.get();
    }

    return nullptr;
}

// no-op if helper is already in the map
void DXamlCore::SetAutomaticDragHelper(_In_ UIElement* uielement, _In_ AutomaticDragHelper* helper)
{
    if (!IsShutdown())
    {
        auto it = m_autoDragHelperMap.find(uielement);

        if (m_autoDragHelperMap.end() == it)
        {
            if (helper != nullptr)
            {
                // the helper hasn't been put in the map yet
                m_autoDragHelperMap.insert(std::make_pair(uielement, std::unique_ptr<AutomaticDragHelper>(helper)));
            }
        }
        else
        {
            if (helper == nullptr)
            {
                // erase the key value pair
                m_autoDragHelperMap.erase(it);
            }
        }
    }
}

TextControlFlyout* DXamlCore::GetTextControlFlyout(_In_opt_ CFlyoutBase* flyout) const
{
    if (!flyout) { return nullptr; }

    ctl::ComPtr<FlyoutBase> flyoutBase;
    VERIFYHR(DXamlCore::GetCurrent()->GetPeer<FlyoutBase>(flyout, &flyoutBase));

    auto it = m_textControlFlyoutHelperMap.find(flyoutBase.Get());

    if (m_textControlFlyoutHelperMap.end() != it)
    {
        return it->second.get();
    }

    return nullptr;
}

// no-op if helper is already in the map
void DXamlCore::SetTextControlFlyout(_In_ FlyoutBase* flyout, _In_ TextControlFlyout* helper)
{
    if (!IsShutdown())
    {
        auto it = m_textControlFlyoutHelperMap.find(flyout);

        if (m_textControlFlyoutHelperMap.end() == it)
        {
            if (helper != nullptr)
            {
                // the helper hasn't been put in the map yet
                m_textControlFlyoutHelperMap.insert(std::make_pair(flyout, std::unique_ptr<TextControlFlyout>(helper)));
            }
        }
        else if (helper == nullptr)
        {
            // erase the key value pair
            m_textControlFlyoutHelperMap.erase(it);
        }
    }
}

bool
DXamlCore::GetIsKeyboardPresent()
{
    INT32 isKeyboardPresent = 0;

    if (!m_keyboardCapabilities)
    {
        IFCFAILFAST(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_Devices_Input_KeyboardCapabilities).Get(), m_keyboardCapabilities.ReleaseAndGetAddressOf()));
    }

    IFCFAILFAST(m_keyboardCapabilities->get_KeyboardPresent(&isKeyboardPresent));
    return !!isKeyboardPresent;
}

#pragma region IXamlTestHooks

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Simulate a device-lost error to trigger re-creation of DComp device(s).
//      Called by the test framework.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DXamlCore::SimulateDeviceLost(bool resetVisuals, bool resetDManip)
{
    HRESULT hr = S_OK;
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        IFC(pCoreServices->SimulateDeviceLost(resetVisuals, resetDManip));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the DComp device. Called by the test framework.
//
//------------------------------------------------------------------------------
void
DXamlCore::GetDCompDevice(
    _Outptr_ IDCompositionDesktopDevicePartner **ppDCompDevice
    ) const
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->GetDCompDevice(ppDCompDevice);
    }
}

// Forces the window to a certain size. Called by the test framework.
// Note: This should only be used for test purposes.
// TODO: This function needs to improve to support multiple desktop top level windows (Win32) Task# 29571334
_Check_return_ HRESULT DXamlCore::SetWindowSizeOverride(
    _In_ const XSIZE *pWindowSize,
    XFLOAT testOverrideScale
    )
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        IFC_RETURN(pCoreServices->SetWindowSizeOverride(pWindowSize,
            m_pControl->GetJupiterWindow()->GetWindowHandle(),
            testOverrideScale));

        IFC_RETURN(OnCompositionContentStateChangedForUWP());
    }

    return S_OK;
}

// A scale change is effectively a window size change. Even though the physical dimensions don't change,
// the logical size of the window is now different. Called from CompositionContent StateChanged handler to trigger OnWindowSizeChanged.
_Check_return_ HRESULT DXamlCore::OnUWPWindowSizeChanged()
{
    if (m_uwpWindowNoRef)
    {
        // In strict mode we need to resize the WindowsRenderTarget when changing the scale
        // There is an existing problem in strict mode where CoreWindows::Bounnds and CompositionIsland::Scale
        // and not in-sync, meaning the value of CoreWindows::Bounds is not up to date with CompositionIsland::Scale
        // This happens at startup as well when shell changes the scale via the Settings app.
        if(XamlOneCoreTransforms::IsEnabled())
        {
            const auto pJupiterWindow = m_pControl->GetJupiterWindow();
            if (pJupiterWindow)
            {
                // Force a call to CXcpBrowserHost::EnsureCorrectWindowSize
                // This call will only redraw if the height/width
                // of the CoreWindow has changed
                IFC_RETURN(m_pControl->OnJupiterWindowSizeChanged(*pJupiterWindow, nullptr));
            }
        }

        IFC_RETURN(m_uwpWindowNoRef->OnWindowSizeChanged());
    }

    return S_OK;
}

// CompositionContent StateChanged event handler for UWP.
_Check_return_ HRESULT DXamlCore::OnCompositionContentStateChangedForUWP()
{
    const auto contentRootCoordinator = GetHandle()->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    const auto rootScale = RootScale::GetRootScaleForContentRoot(root);
    if (!rootScale) // Check that we still have an active tree
    {
        return S_OK;
    }
    IFC_RETURN(rootScale->UpdateSystemScale());

    IFC_RETURN(OnUWPWindowSizeChanged());

    return S_OK;
}

void DXamlCore::SetHdrOutputOverride(bool isHdrOutputOverride)
{
    m_pControl->SetHdrOutputOverride(isHdrOutputOverride);
}

BOOLEAN DXamlCore::GetWantsRenderingEvent()
{
    BOOLEAN result = FALSE;
    CCoreServices* core = GetHandle();
    if (core != nullptr)
    {
        result = !!core->GetWantsRendering();
    }

    return result;
}

bool DXamlCore::GetWantsCompositionTargetRenderedEvent()
{
    bool result = FALSE;
    CCoreServices* core = GetHandle();
    if (core != nullptr)
    {
        result = core->GetWantsCompositionTargetRenderedEvent();
    }

    return result;
}

void DXamlCore::RequestReplayPreviousPointerUpdate_TempTestHook()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->RequestReplayPreviousPointerUpdate();
    }
}

void DXamlCore::SimulateSuspendToPauseAnimations()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->PauseDCompAnimationsOnSuspend();
    }
}

void DXamlCore::SimulateResumeToResumeAnimations()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->ResumeDCompAnimationsOnResume();
    }
}

bool DXamlCore::IsRenderingFrames()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        return pCoreServices->IsRenderingFrames();
    }
    return false;
}

void DXamlCore::SetIsSuspended(bool isSuspended)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->SetIsSuspended(isSuspended);
    }
}

void DXamlCore::SetIsRenderEnabled(bool value)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->SetIsRenderEnabled(value);
    }
}

void DXamlCore::SetIsHolographic(bool value)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->SetIsHolographicOverride(value);
    }
}

void DXamlCore::SetTimeManagerClockOverrideConstant(double newTime)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->SetTimeManagerClockOverrideConstant(newTime);
    }
}

HRESULT DXamlCore::CleanUpAfterTest()
{
    CCoreServices* coreServices = GetHandle();

    if (coreServices != nullptr)
    {
        IFC_RETURN(coreServices->CleanUpAfterTest());
    }

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::InitializeInstanceFromIdle()
{
    IFCEXPECT_ASSERT_RETURN(GetState() == State::Idle);

    if (m_pDefaultStyles == nullptr)
    {
        m_pDefaultStyles = new DefaultStyles();
    }

    m_staticStoreGuard = StaticStore::GetInstance();

    CCoreServices* coreServices = GetHandle();

    if (coreServices != nullptr && !coreServices->IsCoreReady())
    {
        IFC_RETURN(coreServices->InitializeFromIdle());

        // Call SetCurrentApplication and pass in null to create the m_pDeploymentTree
        IFC_RETURN(coreServices->SetCurrentApplication(NULL));

        // Our m_pDeploymentTree actually needs an application to do interesting stuff, so lets give it one!
        auto coreApp = do_pointer_cast<CApplication>(GetCoreAppHandle());
        IFC_RETURN(coreServices->SetCurrentApplication(coreApp));

        ctl::ComPtr<wuc::ICoreWindow> corewindow;
        IFC_RETURN(m_uwpWindowNoRef->get_CoreWindow(&corewindow));

        ctl::ComPtr<ICoreWindowInterop> spCoreWindowInterop;

        IFC_RETURN(corewindow.As(&spCoreWindowInterop));

        HWND hwndCoreWindow = NULL;
        IFC_RETURN(spCoreWindowInterop->get_WindowHandle(&hwndCoreWindow));

        coreServices->GetInputServices()->SetCoreWindow(corewindow.Get());
    }

    IFCEXPECT_RETURN(m_pControl);

    // Start processing ticks again
    m_pControl->SetTicksEnabled(true);

    IFC_RETURN(EnsureEventArgs());

    IFC_RETURN(DCompSurfaceMonitor::Initialize());

    DCompSurfaceFactoryManager::EnsureInitialized();

    IFC_RETURN(ctl::make<UIAffinityReleaseQueue>(&m_spReleaseQueue));

    m_state = State::Initialized;

    if (auto application = FrameworkApplication::GetCurrentNoRef())
    {
        // Make sure we have the dispatcher queue first to avoid any possible race condition
        ctl::ComPtr<xaml::IDebugSettings> debugSettings;
        IGNOREHR(application->get_DebugSettings(&debugSettings));
        if (debugSettings)
        {
            debugSettings.Cast<DebugSettings>()->OnThreadInitialized();
        }
    }

    if (m_uwpWindowNoRef)
    {
       SignalWindowMutation(VisualMutationType::Add);
    }

    return S_OK;
}

 void DXamlCore::SignalWindowMutation(VisualMutationType type)
 {
    if (auto interop = ::Diagnostics::GetDiagnosticsInterop(false))
    {
        interop->SignalRootMutation(ctl::iinspectable_cast(m_uwpWindowNoRef), type);
    }
 }

_Check_return_ HRESULT DXamlCore::EnsureEventArgs()
{
    if (!m_spDataContextChangedEventArgs)
    {
        ctl::ComPtr<DataContextChangedEventArgs> dataContextChangedEventArgs;
        IFC_RETURN(ctl::make<DataContextChangedEventArgs>(&dataContextChangedEventArgs));
        m_spDataContextChangedEventArgs.Attach(dataContextChangedEventArgs.Detach());
    }

    if (!m_spVectorChangedEventArgs)
    {
        ctl::ComPtr<VectorChangedEventArgs> vectorChangedEventArgs;
        IFC_RETURN(ctl::make<VectorChangedEventArgs>(&vectorChangedEventArgs));
        m_spVectorChangedEventArgs.Attach(vectorChangedEventArgs.Detach());
    }

    return S_OK;
}

void DXamlCore::ForceDisconnectRootOnSuspend(bool forceDisconnectRootOnSuspend)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->ForceDisconnectRootOnSuspend(forceDisconnectRootOnSuspend);
    }
}

void DXamlCore::TriggerSuspend(bool isTriggeredByResourceTimer, bool allowOfferResources)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->OnSuspend(isTriggeredByResourceTimer, allowOfferResources);
    }
}

void DXamlCore::TriggerResume()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->OnResume();
    }
}

void DXamlCore::TriggerLowMemory()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        IFCFAILFAST(pCoreServices->CheckMemoryUsage(true /* simulateLowMemory */));
    }
}

void DXamlCore::SimulateThemeChanged()
{
    m_pControl->GetJupiterWindow()->OnThemeChanged();
}

BOOLEAN DXamlCore::InjectWindowMessage(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _In_ xaml::IXamlRoot* xamlRoot)
{
    BOOLEAN result = FALSE;
    if (m_pControl != nullptr)
    {
        ctl::ComPtr<XamlRoot> spXamlRoot;
        IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

        if (msg == WM_POINTERROUTEDAWAY)
        {
            // Xaml now always requires a PointerPoint, and tests don't create one when injecting a pointer message.
            // Only one test injects any sort of pointer message - BasicPointerTests::PointerRoutedAway - so make
            // up a PointerPoint for it.

            wrl::ComPtr<ixp::IPointerPoint> currentPointerPoint;
            const auto jupiterWindow = m_pControl->GetJupiterWindow();
            if (jupiterWindow)
            {
                currentPointerPoint = jupiterWindow->GetInputSiteAdapterPointerPoint();
                ASSERT(currentPointerPoint);
                result = !!m_pControl->HandlePointerMessage(msg, wParam, lParam, spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef(), false /* isGeneratedMessage */, currentPointerPoint.Get());
            }
        }
        else
        {
            result = !!m_pControl->HandleWindowMessage(msg, wParam, lParam, spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef());
        }
    }
    return result;
}

HRESULT DXamlCore::WaitForCommitCompletion()
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        return pCoreServices->WaitForCommitCompletion();
    }
    return S_FALSE;
}

void DXamlCore::GetTransparentBackground(_Out_ bool* pIsTransparent)
{
    CCoreServices* pCoreServices = GetHandle();
    ASSERT(pCoreServices);
    *pIsTransparent = pCoreServices->IsTransparentBackground();
}

// Sets the background transparency for the window.  This has the effect of removing the opaque DComp primitive that
// is created by default for a window. Xaml product code is now toggling transparency at a per-island level. This
// global flag is kept around and used by tests.
void DXamlCore::SetTransparentBackground(bool isTransparent)
{
    CCoreServices* pCoreServices = GetHandle();
    ASSERT(pCoreServices);
    pCoreServices->SetTransparentBackground(isTransparent);
}

void DXamlCore::SetPostTickCallback(_In_opt_ std::function<void()> callback)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        pCoreServices->SetPostTickCallback(callback);
    }
}

void
DXamlCore::OverrideTrimImageResourceDelay(bool enabled)
{
    CCoreServices* pCoreServices = GetHandle();
    ASSERT(pCoreServices);
    pCoreServices->OverrideTrimImageResourceDelay(enabled);
}

_Check_return_ HRESULT
DXamlCore::CreateProviderForAP(_In_ CAutomationPeer* pAP, _Outptr_result_maybenull_ CUIAWrapper** ppRet)
{
    HRESULT hr = S_OK;

    *ppRet = nullptr;

    if (m_pControl)
    {
        IFC(m_pControl->CreateProviderForAP(pAP, ppRet));
    }

Cleanup:
    return hr;
}

UINT32 DXamlCore::GenerateRawElementProviderRuntimeId()
{
    if (m_hCore != nullptr)
    {
        return m_hCore->GetNextRuntimeId();
    }

    return 0;
}

_Check_return_ HRESULT
DXamlCore::SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        IFC_RETURN(pCoreServices->SetSystemFontCollectionOverride(pFontCollection));
    }
    return S_OK;
}

_Check_return_ HRESULT
DXamlCore::ShouldUseTypographicFontModel(_Out_ bool* useDWriteTypographicModel)
{
    CCoreServices* pCoreServices = GetHandle();
    if (pCoreServices != nullptr)
    {
        IFC_RETURN(pCoreServices->ShouldUseTypographicFontModel(useDWriteTypographicModel));
    }
    return S_OK;
}

void DXamlCore::SetMockUIAClientsListening(bool isEnabledMockUIAClientsListening)
{
    m_pControl->SetMockUIAClientsListening(isEnabledMockUIAClientsListening);
}

void DXamlCore::SetGenericXamlFilePathForMUX(const xstring_ptr_view& filePath)
{
    if (filePath.IsNullOrEmpty())
    {
        m_genericXamlFilePathFromMUX.Reset();
        m_pDefaultStyles->GetStyleCache()->Clear();
        m_pDefaultStyles->GetStyleCache()->LoadThemeResources();
    }
    else
    {
        TrySetGenericXamlFilePathFromMUX(filePath);
    }
}

void DXamlCore::SetThreadingAssertOverride(bool enabled)
{
#if DBG
    CCoreServices* pCoreServices = GetHandle();
    ASSERT(pCoreServices);
    pCoreServices->SetThreadingAssertOverride(enabled);
#endif
}

#pragma endregion

_Check_return_ HRESULT DXamlCore::GetContentBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue)
{
    if (dependencyObject && TryGetXamlIslandBoundsForElement(dependencyObject, pValue))
    {
        return S_OK;
    }

    if (m_uwpWindowNoRef && m_uwpWindowNoRef->HasBounds())
    {
        IFC_RETURN(m_uwpWindowNoRef->get_Bounds(pValue));
    }
    else
    {
        // Except Islands/ Win32, HasBounds should always be called before retrieving bounds. In win32/ Islands, it's okay to have no bounds
        ASSERT(GetHandle()->GetInitializationType() == InitializationType::IslandsOnly);
        *pValue = wf::Rect();
    }

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetContentLayoutBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue)
{
    if (TryGetXamlIslandBoundsForElement(dependencyObject, pValue))
    {
        return S_OK;
    }

    if (m_uwpWindowNoRef)
    {
        IFC_RETURN(m_uwpWindowNoRef->GetLayoutBounds(pValue));
    }
    else
    {
        ASSERT(false);  // HasBounds should always be called before retrieving bounds
    }

    return S_OK;
}

// ContentBounds are visible bounds or XamlIslandRoot bounds if Xaml is hosted in island mode
_Check_return_ HRESULT DXamlCore::GetVisibleContentBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue)
{
    IFC_RETURN(GetVisibleContentBoundsForElement(false /*ignoreIHM*/, false /*inDesktopCoordinates*/, dependencyObject, pValue));

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::GetVisibleContentBoundsForElement(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue)
{
    if (TryGetXamlIslandBoundsForElement(dependencyObject, pValue))
    {
        return S_OK;
    }

    if (m_uwpWindowNoRef)
    {
        IFC_RETURN(m_uwpWindowNoRef->GetVisibleBounds(ignoreIHM, inDesktopCoordinates, pValue));

        // Potentially shrink the visible bounds for testing purposes only.
        IFC_RETURN(Window::ShrinkApplicationViewVisibleBounds(m_uwpWindowNoRef, pValue));
    }
    else
    {
        ASSERT(false);  // HasBounds should always be called before retrieving bounds
    }

    return S_OK;
}

// Returns the XamlIslandRoot bounds if Xaml is in XamlIslandRoot host mode. Some elements such as connected animations
// will always have a non-XamlIslandRoot root and will never have XamlIslandRoot bounds.
bool DXamlCore::TryGetXamlIslandBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue)
{
    CXamlIslandRoot* xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(dependencyObject);
    if(xamlIslandRoot)
    {
        pValue->X = 0;
        pValue->Y = 0;

        auto size = xamlIslandRoot->GetSize();
        pValue->Width = size.Width;
        pValue->Height = size.Height;

        return true;
    }

    return false;
}

/* static */ _Check_return_ HRESULT DXamlCore::SetBinding(
    _In_ IInspectable* source,
    _In_ HSTRING pathString,
    _In_ xaml::IDependencyObject* target,
    KnownPropertyIndex targetPropertyIndex,
    _In_opt_ xaml_data::IValueConverter* converter)
{
    ctl::ComPtr<PropertyPath> path;
    IFC_RETURN(PropertyPath::CreateInstance(pathString, &path));

    ctl::ComPtr<Binding> binding;
    IFC_RETURN(ctl::make(&binding));

    IFC_RETURN(binding->put_Source(source));
    IFC_RETURN(binding->put_Path(path.Get()));
    IFC_RETURN(binding->put_Mode(xaml_data::BindingMode_OneWay));

    if (converter)
    {
        IFC_RETURN(binding->put_Converter(converter));
    }

    ctl::ComPtr<BindingOperations> bindingOperations;
    IFC_RETURN(ctl::make(&bindingOperations));

    ctl::ComPtr<IDependencyProperty> targetProperty;
    IFC_RETURN(MetadataAPI::GetIDependencyProperty(targetPropertyIndex, &targetProperty));

    IFC_RETURN(bindingOperations->SetBinding(target, targetProperty.Get(), binding.Get()));
    return S_OK;
}
/* static */ _Check_return_ HRESULT DXamlCore::SetBindingCore(
    _In_ CDependencyObject* source,
    _In_ HSTRING path,
    _In_ CDependencyObject* target,
    KnownPropertyIndex targetPropertyIndex)
{
    ctl::ComPtr<DependencyObject> sourceDO;
    ctl::ComPtr<DependencyObject> targetDO;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(source, &sourceDO));
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(target, &targetDO));

    IFC_RETURN(SetBinding(ctl::as_iinspectable(sourceDO.Get()), path, targetDO.Get(), targetPropertyIndex));
    return S_OK;
}

void DXamlCore::RegisterForChangeVisualStateOnDynamicScrollbarsRegistryKeySettingChanged()
{
    if (m_regKeyWatcher == nullptr)
    {
        // First time, hookup to registry notifications
        wil::unique_hkey hKeyAccessibility;
        // Open the key for read first instead of making make_registry_watcher doing it because
        // registry_watcher creates the key instead of opening it causing access issues on iot skus.
        const LONG result = RegOpenKeyEx(
            HKEY_CURRENT_USER,
            L"Control Panel\\Accessibility",
            0,
            KEY_NOTIFY,
            &hKeyAccessibility);

        if (ERROR_SUCCESS == result)
        {
            m_regKeyWatcher = wil::make_registry_watcher_nothrow(
                wistd::move(hKeyAccessibility),
                true /* isRecursive */,
                [this](wil::RegistryChangeKind kind)
                {
                    // This callback will be raised from a thread pool thread. Dispatch to UI Thread.
                    GetXamlDispatcherNoRef()->RunAsync(MakeCallback(this, &DXamlCore::RegistryUpdatedCallback));
                });
        }
    }

}

_Check_return_ HRESULT DXamlCore::UpdateVisualStateForConsciousScrollbar()
{
    for (const auto& control : m_registeredControlsForSettingsChanged)
    {
        IFC_RETURN(Control::UpdateVisualState(control, false /* useTransitions*/));
    }

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::RegistryUpdatedCallback()
{
    s_dynamicScrollbars = ShouldUseDynamicScrollbars_CheckRegistryKey();
    UpdateVisualStateForConsciousScrollbar();

    return S_OK;
}

_Check_return_ HRESULT DXamlCore::OnAutoHideScrollbarsChanged()
{
    DXamlCore::s_dynamicScrollbarsDirty = true;
    IFC_RETURN(UpdateVisualStateForConsciousScrollbar());
    return S_OK;
}

void DXamlCore::UnregisterFromDynamicScrollbarsSettingChanged(_In_ Control* control)
{
    // remove from m_registeredControlsForSettingsChanged
    const auto it =
        std::find(
            m_registeredControlsForSettingsChanged.begin(),
            m_registeredControlsForSettingsChanged.end(),
            control);
    if (it != m_registeredControlsForSettingsChanged.end())
    {
        m_registeredControlsForSettingsChanged.erase(it);
    }
}

/* static*/
bool DXamlCore::ShouldUseDynamicScrollbars_CheckRegistryKey()
{
    LPCWSTR accessibilityKeyName = L"Control Panel\\Accessibility";
    DWORD keyValue = 0;
    DWORD size = sizeof(DWORD);
    bool isDynamic = true; // Enabled by default.
    if (SUCCEEDED(HRESULT_FROM_WIN32(::RegGetValue(HKEY_CURRENT_USER, accessibilityKeyName, L"DynamicScrollbars", RRF_RT_REG_DWORD, NULL, &keyValue, &size))))
    {
        isDynamic = (keyValue != FALSE);
    }

    return isDynamic;
}

ctl::ComPtr<DxamlCoreTestHooks> DXamlCore::GetTestHooks()
{
    if (m_spTestHooks == nullptr)
    {
        IFCFAILFAST(ctl::make(&m_spTestHooks));
        m_spTestHooks->SetDXamlCore(this);
    }

    ctl::ComPtr<DxamlCoreTestHooks> testHooks = m_spTestHooks;
    return testHooks;
}

ctl::ComPtr<DirectUI::NullKeyedResource> DXamlCore::GetCachedNullKeyedResource()
{
    if (!m_spCachedNullKeyedResource)
    {
        IFCFAILFAST(ctl::make(&m_spCachedNullKeyedResource));
    }

    return m_spCachedNullKeyedResource;
}

bool DXamlCore::IsInBackgroundTask() const
{
    return m_hCore->IsInBackgroundTask();
}

_Check_return_ HRESULT DXamlCore::IsAnimationEnabled(_Out_ bool* result)
{
    *result = false;

    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    // Tests can override the UISettings value to false to ensure that the platform responds appropriately
    // without modifying system-wide settings.
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(
        RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations))
    {
        return S_OK;
    }

    // Tests can override the UISettings value to ensure animations run during tests
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(
        RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableGlobalAnimations))
    {
        *result = true;
        return S_OK;
    }

    //  Don't need to call UpdateAnimationsEnabled if we've successfully registered for AnimationsEnabledChanged event.
    if (m_animationsEnabledChangedToken.value == 0)
    {
        CCoreServices* coreServices = GetHandle();

        // We cache the value of UISettings.AnimationsEnabled to avoid the expensive call to get it all the time.
        if (!coreServices || coreServices->GetShouldReevaluateIsAnimationEnabled())
        {
            IFC_RETURN(UpdateAnimationsEnabled());

            if (coreServices)
            {
                coreServices->SetShouldReevaluateIsAnimationEnabled(false);
            }
        }
    }

    *result = m_isAnimationEnabled;

    return S_OK;
}
