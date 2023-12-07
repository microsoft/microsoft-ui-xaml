// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DesktopWindowXamlSource.g.h"
#include "WindowsXamlManager_Partial.h"
#include "Window_Partial.h"

#include <DependencyLocator.h>
#include <FrameworkTheming.h>

#include "FrameworkApplication_Partial.h"
#include "XAMLIslandRoot_Partial.h"
#include <windows.ui.composition.interop.h>
#include "IFocusController.h"
#include "DXamlServices.h"
#include "winpal.h"
#include "DiagnosticsInterop.h"
#include "focusmgr.h"
#include "KeyboardAcceleratorUtility.h"
#include "XamlTraceLogging.h"
#include "RuntimeProfiler.h"
#include "DCompTreeHost.h"
#include "SystemBackdrop.g.h"
#include <windowing.h>
#include <Windows.UI.Composition.h>
#include <WindowingCoreContentApi.h>
#include <WRLHelper.h>
#include <NavigateFocusResult.h>
#include <Microsoft.UI.Content.Private.h>

using namespace DirectUI;

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_WindowsXamlManager();
}

_Check_return_ HRESULT DesktopWindowXamlSource::InitializeImpl(_In_ mu::WindowId parentWnd)
{
    HWND parentHwnd;

    IFC_RETURN(Windowing_GetWindowFromWindowId(parentWnd, &parentHwnd));

    return AttachToWindow(parentHwnd);
}

_Check_return_ HRESULT DesktopWindowXamlSource::get_SiteBridgeImpl(_Outptr_result_maybenull_ ixp::IDesktopChildSiteBridge** ppValue)
{
    return m_contentBridgeDW.CopyTo(ppValue);
}

_Check_return_ HRESULT DesktopWindowXamlSource::AttachToWindow(_In_ HWND parentHwnd)
{
    IFC_RETURN(CheckThread());

    if (m_bClosed)
    {
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
            E_UNEXPECTED,
            wrl_wrappers::HStringReference(L"Cannot attach to a window when the DesktopWindowXamlSource instance has been closed").Get()));
    }

    const DWORD windowThreadId = ::GetWindowThreadProcessId(parentHwnd, nullptr /* lpProcessId*/ );
    if (::GetCurrentThreadId() != windowThreadId)
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(
            E_UNEXPECTED,
            ERROR_DESKTOPWINDOWXAMLSOURCE_WINDOW_IS_ON_DIFFERENT_THREAD));
    }

    //Initialize island will configure the core window when starting the framework application on the current thread.
    IFC_RETURN(ConnectToHwndIslandSite(parentHwnd));

    return S_OK;
}

DesktopWindowXamlSource::DesktopWindowXamlSource()
{
}

DesktopWindowXamlSource::~DesktopWindowXamlSource()
{
    VERIFYHR(Close());
}

_Check_return_ HRESULT DesktopWindowXamlSource::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    return __super::QueryInterfaceImpl(iid, ppObject);
}

_Check_return_ HRESULT DesktopWindowXamlSource::CheckWindowingModelPolicy()
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
        // We don't support this as there are issues with Xaml statics
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
            wrl_wrappers::HStringReference(L"Cannot activate DesktopWindowXamlSource. This type cannot be used in a UWP app. See: https://go.microsoft.com/fwlink/?linkid=875495").Get()));
    }

    return S_OK;
}

#define FIREFRAMEWORKTELEMETRY_UNINITIALIZED    0x80000000

void DesktopWindowXamlSource::FireFrameworkTelemetry()
{
    //  Getting a bitmask of the interesting loaded frameworks on this process

    struct
    {
        const wchar_t * pszLibraryName;
        DWORD fdwMask;
    } ScanList[] =
    {
        { L"system.windows.forms.dll",     0x1 },
        { L"system.windows.forms.ni.dll",  0x1 },
        { L"presentationframework.dll",    0x2 },
        { L"presentationframework.ni.dll", 0x2 },
    };

    //  Setting uninitialized bit to force sending event even on 'no frameworks' case
    static DWORD    fdwPreviouslyReportedMask = FIREFRAMEWORKTELEMETRY_UNINITIALIZED;
    DWORD           fdwLoadedFrameworkMask = (fdwPreviouslyReportedMask & (~FIREFRAMEWORKTELEMETRY_UNINITIALIZED));

    for (UINT ii = 0; ii < _countof(ScanList); ii++)
    {
        // Have we already reported this framework?
        if (0 == (fdwLoadedFrameworkMask & ScanList[ii].fdwMask))
        {
            // Nope.  Is this binary loaded in the process?
            if (NULL != GetModuleHandle(ScanList[ii].pszLibraryName))
            {
                fdwLoadedFrameworkMask |= ScanList[ii].fdwMask;
            }
        }
    }

    if (fdwPreviouslyReportedMask == fdwLoadedFrameworkMask)
    {
        //  Nothing new to report
        return;
    }

    //  Updating reported mask, filtering out MSB
    fdwPreviouslyReportedMask = fdwLoadedFrameworkMask;

    //  Fire event!
    TraceLoggingWrite(
        g_hTraceProvider,
        "DesktopWindowXamlSource-LoadedFrameworks",
        TraceLoggingDescription("Reporting loaded libraries upon DWXS activation."),
        TraceLoggingUInt32(((UINT32)fdwLoadedFrameworkMask), "LoadedFrameworkMask"),
        TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
}

void DesktopWindowXamlSource::InstrumentUsage(_In_ bool fRemove)
{
    static LONG     cActive = 0;
    static LONG     cMax = 0;

    FireFrameworkTelemetry();

    if (fRemove)
    {
        ::InterlockedDecrement(&cActive);
    }
    else
    {
        LONG    cCurrent = ::InterlockedIncrement(&cActive);

        if (cCurrent > cMax)
        {
            cMax = cCurrent;

            //  Fire event that says we've established a new max.
            TraceLoggingWrite(
                g_hTraceProvider,
                "DesktopWindowXamlSource-NewMaxActive",
                TraceLoggingDescription("Set new max active DesktopWindowXamlSource counts."),
                TraceLoggingUInt32(((UINT32)cCurrent), "NewMaxValue"),
                TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        //  Count marker for DesktopWindowXamlSource
        __RP_Marker_ClassByName("DesktopWindowXamlSource")
    }
}

_Check_return_ HRESULT DesktopWindowXamlSource::Initialize()
{
    IFC_RETURN(WeakReferenceSourceNoThreadId::Initialize());

    ctl::ComPtr<IActivationFactory> activationFactory(CreateActivationFactory_WindowsXamlManager());
    ctl::ComPtr<xaml_hosting::IWindowsXamlManagerStatics> coreFactory;
    IFC_RETURN(activationFactory.As(&coreFactory));
    ctl::ComPtr<xaml_hosting::IWindowsXamlManager> core;
    IFC_RETURN(coreFactory->InitializeForCurrentThread(&core));
    IFC_RETURN(core.As(&m_spXamlCore));

    IFC_RETURN(CheckThread());

    // In a C# desktop app, the DesktopXamlIslandSource isn't exposed to the app. Peg this peer so it doesn't get released
    // when the GC does garbage collection. There's a m_owner WeakRef pointer on DirectUI::XamlIslandRoot that points to this
    // DesktopWindowXamlSource, which we need to preserve to keep VS live visual tree working.
    SetReferenceTrackerPeg();

    // This will make sure that it doesn't get cleared off thread in WeakReferenceSourceNoThreadId::OnFinalReleaseOffThread()
    // as it has thread-local variables and needs to be disposed off by the same thread
    AddToReferenceTrackingList();

    ctl::ComPtr<FrameworkApplication> frameworkApplication = FrameworkApplication::GetCurrentNoRef();
    IFC_RETURN(frameworkApplication->CreateIslandRootWithContentBridge(ctl::iinspectable_cast(this), nullptr, m_spXamlIsland.ReleaseAndGetAddressOf()));

    // Create and configure focus navigation controller
    ctl::ComPtr<IInspectable> spInsp;
    IFC_RETURN(m_spXamlIsland->get_FocusController(&spInsp));
    IFC_RETURN(spInsp.As(&m_spFocusController));
    IFC_RETURN(m_spFocusController->add_GotFocus(
        Microsoft::WRL::Callback<xaml_hosting::FocusNavigatedEventHandler>(
            this, &DesktopWindowXamlSource::OnFocusControllerGotFocus).Get(),
        &m_gotFocusEventCookie));
    IFC_RETURN(m_spFocusController->add_LosingFocus(
        Microsoft::WRL::Callback<xaml_hosting::FocusDepartingEventHandler>(
            this, &DesktopWindowXamlSource::OnFocusControllerLosingFocus).Get(),
        &m_losingFocusEventCookie));

    ctl::ComPtr<XamlIslandRoot> xamlIsland = m_spXamlIsland.Cast<XamlIslandRoot>();
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(xamlIsland->GetHandle());
    focusManager->SetCanTabOutOfPlugin(true);

    auto coreXamlIsland = static_cast<CXamlIslandRoot*>(xamlIsland->GetHandle());
    coreXamlIsland->SetHasTransparentBackground(true);

    InstrumentUsage(false); // false -> adding
    m_initializedCalled = true;
    return S_OK;
}

wsy::VirtualKeyModifiers GetVirtualKeyModifiers()
{
    wsy::VirtualKeyModifiers result = {};
    byte keyboardState[256] = {};
    if (!::GetKeyboardState(keyboardState))
    {
        return result;
    }
    if (keyboardState[VK_SHIFT] & 0x80)
    {
        result |= wsy::VirtualKeyModifiers::VirtualKeyModifiers_Shift;
    }
    if (keyboardState[VK_CONTROL] & 0x80)
    {
        result |= wsy::VirtualKeyModifiers::VirtualKeyModifiers_Control;
    }
    if (keyboardState[VK_MENU] & 0x80)
    {
        result |= wsy::VirtualKeyModifiers::VirtualKeyModifiers_Menu;
    }
    return result;
}

// Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop implementation
IFACEMETHODIMP DesktopWindowXamlSource::get_SystemBackdrop(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush)
{
    ARG_VALIDRETURNPOINTER(systemBackdropBrush);
    *systemBackdropBrush = nullptr;

    IFC_RETURN(CheckThread());

    IFCEXPECT_RETURN(m_contentBridge);

    ctl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop> compositionSupportsSystemBackdrop;

    IFC_RETURN(m_contentBridge.As(&compositionSupportsSystemBackdrop));

    IFC_RETURN(compositionSupportsSystemBackdrop->get_SystemBackdrop(systemBackdropBrush));

    return S_OK;
}

IFACEMETHODIMP DesktopWindowXamlSource::put_SystemBackdrop(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush)
{
    IFC_RETURN(CheckThread());

    IFCEXPECT_RETURN(m_contentBridge);

    ctl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop> compositionSupportsSystemBackdrop;

    IFC_RETURN(m_contentBridge.As(&compositionSupportsSystemBackdrop));

    IFC_RETURN(compositionSupportsSystemBackdrop->put_SystemBackdrop(systemBackdropBrush));

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowXamlSource::get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** iSystemBackdrop)
{
    return m_systemBackdrop.CopyTo(iSystemBackdrop);
}

_Check_return_ HRESULT DesktopWindowXamlSource::put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* iSystemBackdrop)
{
    // If nothing changed then do nothing. Otherwise we'd call OnTargetDisconnected and OnTargetConnected
    // back-to-back on the same SystemBackdrop.
    if (m_systemBackdrop.Get() != iSystemBackdrop)
    {
        if (m_systemBackdrop.Get() != nullptr)
        {
            ctl::ComPtr<DirectUI::SystemBackdrop> systemBackdrop;
            IFC_RETURN(m_systemBackdrop.As(&systemBackdrop));
            IFC_RETURN(systemBackdrop->InvokeOnTargetDisconnected(this));
        }

        m_systemBackdrop = iSystemBackdrop;
        if (iSystemBackdrop != nullptr)
        {
            ctl::ComPtr<xaml::IUIElement> content;
            IFC_RETURN(get_ContentImpl(&content));
            ctl::ComPtr<xaml::IXamlRoot> xamlRoot;
            IFC_RETURN(content->get_XamlRoot(&xamlRoot));

            ctl::ComPtr<DirectUI::SystemBackdrop> systemBackdrop;
            IFC_RETURN(m_systemBackdrop.As(&systemBackdrop));
            systemBackdrop->InvokeOnTargetConnected(this, xamlRoot.Get());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowXamlSource::get_ContentImpl(_Outptr_ xaml::IUIElement** ppValue)
{
    *ppValue = nullptr;
    IFC_RETURN(m_spXamlIsland->get_Content(ppValue));

    return S_OK;
}


_Check_return_ HRESULT DesktopWindowXamlSource::put_ContentImpl(_In_opt_ xaml::IUIElement* pValue)
{
    if (m_childHwnd && pValue)
    {
        ctl::ComPtr<xaml::IFrameworkElement> contentAsFE;
        IFC_RETURN(ctl::do_query_interface(contentAsFE, pValue));

        auto contentCoreDO = contentAsFE.Cast<FrameworkElement>()->GetHandle();

        if (contentCoreDO->IsPropertyDefault(contentCoreDO->GetPropertyByIndexInline(KnownPropertyIndex::FrameworkElement_FlowDirection)) &&
            (GetWindowLong(m_childHwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL))
        {
            IFC_RETURN(contentAsFE->put_FlowDirection(xaml::FlowDirection_RightToLeft));
        }
    }

    IFC_RETURN(m_spXamlIsland->put_Content(pValue));
    return S_OK;
}

_Check_return_ xaml_hosting::IXamlIslandRoot* DesktopWindowXamlSource::GetXamlIslandRootNoRef()
{
    return m_spXamlIsland.Get();
}

IFACEMETHODIMP DesktopWindowXamlSource::Close()
{
    if (m_bClosed)
    {
        return S_OK;
    }

    IFC_RETURN(CheckThread());

    m_bClosed = true;

    if (m_initializedCalled)
    {
        InstrumentUsage(true); // true -> removing
    }

    if (m_systemBackdrop.Get() != nullptr)
    {
        ctl::ComPtr<DirectUI::SystemBackdrop> systemBackdrop;
        IFC_RETURN(m_systemBackdrop.As(&systemBackdrop));
        IFC_RETURN(systemBackdrop->InvokeOnTargetDisconnected(this));
        systemBackdrop = nullptr;
        m_systemBackdrop = nullptr;
    }

    auto island = m_spXamlIsland;

    IFC_RETURN(ReleaseFocusController());

    if (m_spXamlIsland.Get() != nullptr)
    {
        // Remove the Xaml content before calling Dispose on the content bridge. Disposing the content bridge also
        // closes the entire visual tree under it. If we then go to unparent visuals we'll hit RO_E_CLOSED errors
        // everywhere.
        auto frameworkApplication = FrameworkApplication::GetCurrentNoRef();
        IFCFAILFAST(frameworkApplication->RemoveIsland(m_spXamlIsland.Get()));

        // Turn off any frame counters (if they are on) for the same reason we remove the content.  Also inform
        // the core that we have done this in case it needs to re-evaluate whether to display on a future frame
        // (and a different island).
        DirectUI::XamlIslandRoot* xamlIsland = m_spXamlIsland.Cast<XamlIslandRoot>();
        CXamlIslandRoot* pXamlIslandCore = static_cast<CXamlIslandRoot*>(xamlIsland->GetHandle());
        if (pXamlIslandCore->GetDCompTreeHost())
        {
            IFC_RETURN(pXamlIslandCore->GetDCompTreeHost()->UpdateDebugSettings(false /* isFrameRateCounterEnabled */));
        }
        IFC_RETURN(pXamlIslandCore->GetContext()->OnDebugSettingsChanged());
    }

    if (m_contentBridgeDW)
    {
        ctl::ComPtr<mu::IClosableNotifier> closableNotifier;
        IFCFAILFAST(m_contentBridgeDW.As(&closableNotifier));
        IGNOREHR(closableNotifier->remove_FrameworkClosed(m_bridgeClosedToken));
    }

    // Dispose of the content bridge
    if (m_contentBridge && !m_bridgeClosed)
    {
        ctl::ComPtr<wf::IClosable> spClosable;
        IFCFAILFAST(m_contentBridge.As(&spClosable));

        m_contentBridge = nullptr;
        IFCFAILFAST(spClosable->Close());
    }
    m_desktopBridge = nullptr;
    m_contentBridgeDW = nullptr;

    // Signal to the interop tool of the closure after the XamlIslandRoot has been removed. This way
    // the RuntimeObjectCache stays connected.
    if (auto interop = Diagnostics::GetDiagnosticsInterop(false))
    {
        interop->SignalRootMutation(ctl::iinspectable_cast(this), VisualMutationType::Remove);
    }

    if (m_spXamlIsland.Get() != nullptr)
    {
        ctl::ComPtr<XamlIslandRoot> xamlIsland;
        IFC_RETURN(m_spXamlIsland.As(&xamlIsland));
        CXamlIslandRoot* pXamlIslandCore = static_cast<CXamlIslandRoot*>(xamlIsland->GetHandle());
        pXamlIslandCore->Dispose();
        m_spXamlIsland = nullptr;
    }

    if (m_spXamlCore.Get()!=nullptr)
    {
        ctl::ComPtr<DesktopWindowXamlSource> spThis(this); // Avoid deleting this
        ctl::ComPtr<wf::IClosable> spClosable;

        IFC_RETURN(m_spXamlCore.As(&spClosable));
        IFC_RETURN(spClosable->Close());
        spClosable = nullptr;
        m_spXamlCore = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT DesktopWindowXamlSource::ReleaseFocusController()
{
    if (m_spFocusController)
    {
        if (m_gotFocusEventCookie.value != 0)
        {
            IFC_RETURN(m_spFocusController->remove_GotFocus(m_gotFocusEventCookie));
            m_gotFocusEventCookie = {};
        }
        if (m_losingFocusEventCookie.value != 0)
        {
            IFC_RETURN(m_spFocusController->remove_LosingFocus(m_losingFocusEventCookie));
            m_losingFocusEventCookie = {};
        }
        m_spFocusController = nullptr;
    }
    return S_OK;
}

// Pre-generate a nullable MUC.LayoutDirection type, to be used for ContentSiteBridge.LayoutDirectionOverride
REFERENCE_ELEMENT_NAME_IMPL(ixp::ContentLayoutDirection, L"Microsoft.UI.Content.ContentLayoutDirection");

_Check_return_ HRESULT DesktopWindowXamlSource::ConnectToHwndIslandSite(_In_ HWND parentHwnd)
{
    // Create / access composition island
    DirectUI::XamlIslandRoot* xamlIsland = m_spXamlIsland.Cast<XamlIslandRoot>();

    // Get the XamlIslandRoot
    CXamlIslandRoot* pXamlIslandCore = static_cast<CXamlIslandRoot*>(xamlIsland->GetHandle());

    ctl::ComPtr<ixp::IDesktopChildSiteBridgeStatics> bridgeStatics;
    IFC_RETURN(ActivationFactoryCache::GetActivationFactoryCache()->GetDesktopChildSiteBridgeStatics(&bridgeStatics));

    DCompTreeHost* dcompTreeHost = pXamlIslandCore->GetDCompTreeHost();
    WUComp::ICompositor* compositor = dcompTreeHost->GetCompositor();
    FAIL_FAST_ASSERT(compositor);

    // Create DesktopChildSiteBridgeFactory
    ABI::Microsoft::UI::WindowId parentWindowId;
    IFC_RETURN(Windowing_GetWindowIdFromWindow(parentHwnd, &parentWindowId));
    IFC_RETURN(bridgeStatics->Create(
        compositor,
        parentWindowId,
        m_contentBridgeDW.ReleaseAndGetAddressOf()));

    IFCFAILFAST(m_contentBridgeDW.As(&m_contentBridge));
    IFCFAILFAST(m_contentBridgeDW.As(&m_desktopBridge));

    wrl::ComPtr<ixp::IContentIsland> contentIsland = pXamlIslandCore->GetContentIsland();

    IFC_RETURN(m_desktopBridge->Connect(contentIsland.Get()));

    // Note: This is marking CXamlIslandRoot::SetContentRequested, which is required for the island
    // to get a present target and to start rendering. See xamlIslandRoot->GetContentRequested() check inside
    // DCompTreeHost::EnsureXamlIslandTargetRoots.
    pXamlIslandCore->SetContentRequested(true);

    ABI::Microsoft::UI::WindowId windowId;
    IFC_RETURN(m_desktopBridge->get_WindowId(&windowId));
    IFC_RETURN(Windowing_GetWindowFromWindowId(windowId, &m_childHwnd));

    // Show and resize the bridge to fill the main window.
    RECT rcClient;
    if (!::GetClientRect(parentHwnd, &rcClient))
    {
        IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
    }
    if (!::SetWindowPos(m_childHwnd, NULL, 0, 0, rcClient.right, rcClient.bottom,
        SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW))
    {
        IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Note: This is needed for ScrollViewer initialization.
    // CUIElement::CanDMContainerInitialize looks for a valid input window, and CanDMContainerInitialize
    // is needed before CInputServices::InitializeDirectManipulationContainers can activate DM for a
    // ScrollViewer.
    wrl::ComPtr<ixp::IContentIslandPartner> contentIslandPartner;
    IFCFAILFAST(contentIsland.As(&contentIslandPartner));
    wrl::ComPtr<ixp::IIslandInputSitePartner> islandInputSitePartner;
    IFCFAILFAST(contentIslandPartner->get_IslandInputSite(&islandInputSitePartner));
    ABI::Microsoft::UI::WindowId inputWindowId;
    IFCFAILFAST(islandInputSitePartner->get_WindowId(&inputWindowId));
    HWND inputHwnd;
    IFCFAILFAST(Windowing_GetWindowFromWindowId(inputWindowId, &inputHwnd));
    pXamlIslandCore->SetInputWindow(inputHwnd);

    // Now that we've initialized the DesktopWindowXamlBridge, it's safe to tell the XamlIslandRoot to
    // do the initialization it needs which depends on it being property setup (eg setting up WindowInformation).
    pXamlIslandCore->OnPostDesktopWindowContentBridgeInitialized(m_contentBridgeDW.Get());

    // Note: This can only happen after we've told the XamlIslandRoot about the bridge.
    pXamlIslandCore->ForceLTRLayoutDirection();

    if (auto interop = Diagnostics::GetDiagnosticsInterop(false))
    {
        interop->SignalRootMutation(ctl::iinspectable_cast(this), VisualMutationType::Add);
    }

    ctl::ComPtr<xaml::IUIElement> content;
    IFC_RETURN(get_Content(&content));

    if (m_childHwnd && content)
    {
        auto contentAsFE = content.AsOrNull<xaml::IFrameworkElement>();

        contentAsFE->put_FlowDirection(
            (GetWindowLong(m_childHwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) ?
            xaml::FlowDirection_RightToLeft :
            xaml::FlowDirection_LeftToRight);
    }

    {
        ctl::ComPtr<mu::IClosableNotifier> closableNotifier;
        IFCFAILFAST(m_contentBridgeDW.As(&closableNotifier));

        // It's safe to capture "this" because we remove the event subscription from the dtor.
        auto frameworkClosedCallback = [this]() mutable -> HRESULT
        {
            // http://task.ms/45244384 Simplify the shutdown process of DesktopWindowXamlSource and Popups...
            // TODO: Ideally we would do a full Close here, release all our content and input objects, etc.
            // But we made this change late in the 1.4 release cycle and wanted to avoid much code churn.
            this->m_bridgeClosed = true;
            IFCFAILFAST(this->ReleaseFocusController());
            return S_OK;
        };

        IFCFAILFAST(closableNotifier->add_FrameworkClosed(
            WRLHelper::MakeAgileCallback<mu::IClosableNotifierHandler>(frameworkClosedCallback).Get(),
            &m_bridgeClosedToken));
    }

    return S_OK;
}

_Check_return_
HRESULT DesktopWindowXamlSource::NavigateFocusImpl(
        _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* request,
        _Outptr_ xaml_hosting::IXamlSourceFocusNavigationResult** ppResult)
{
    IFC_RETURN(CheckThread());
    if (m_spFocusController)
    {
        ctl::ComPtr<XamlIslandRoot> xamlIsland = m_spXamlIsland.Cast<XamlIslandRoot>();
        CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(xamlIsland->GetHandle());

        IFC_RETURN(m_spFocusController->NavigateFocus(request, focusManager->GetFocusObserverNoRef(), ppResult));
    }
    else
    {
        // In this case, the DWXS is not attached to an active bridge, so we can't process a focus navigation.
        // Return a valid result object with "FocusMoved" set to "false".
        wrl::ComPtr<xaml_hosting::NavigateFocusResult> result = wrl::Make<xaml_hosting::NavigateFocusResult>(false /*focusMoved*/);
        *ppResult = result.Detach();
    }
    return S_OK;

}

_Check_return_
HRESULT DesktopWindowXamlSource::get_HasFocusImpl(_Out_ boolean* pValue)
{
    IFC_RETURN(CheckThread());
    if (m_spFocusController)
    {
        IFC_RETURN(m_spFocusController->get_HasFocus(pValue));
    }
    else
    {
        // In this case, the DWXS is not attached to an active bridge, so we can't have focus.
        *pValue = FALSE;
    }
    return S_OK;
}

_Check_return_
HRESULT DesktopWindowXamlSource::GetGotFocusEventSourceNoRef(_Outptr_ GotFocusEventSourceType** ppEventSource)
{
    if (!m_spGotFocusEventSource)
    {
        IFC_RETURN(ctl::make(&m_spGotFocusEventSource));
        m_spGotFocusEventSource->Initialize(KnownEventIndex::DesktopWindowXamlSource_GotFocus, this, FALSE);
    }
    *ppEventSource = m_spGotFocusEventSource.Get();
    return S_OK;
}

_Check_return_
HRESULT DesktopWindowXamlSource::OnFocusControllerGotFocus(_In_ IInspectable*, _In_ IInspectable* args)
{
    if (m_spGotFocusEventSource)
    {
        ctl::ComPtr<DesktopWindowXamlSource> spThis(this);
        ctl::ComPtr<xaml_hosting::IDesktopWindowXamlSource> spThat;
        IFC_RETURN(spThis.As(&spThat));
        ctl::ComPtr<xaml_hosting::IDesktopWindowXamlSourceGotFocusEventArgs> spArgs;
        IFC_RETURN(ctl::ComPtr<IInspectable>(args).As(&spArgs));
        IFC_RETURN(m_spGotFocusEventSource->Raise(spThat.Get(), spArgs.Get()));
    }
    return S_OK;
}

_Check_return_
HRESULT DesktopWindowXamlSource::GetTakeFocusRequestedEventSourceNoRef(_Outptr_ TakeFocusRequestedEventSourceType** ppEventSource)
{
    if (!m_spLosingFocusEventSource)
    {
        IFC_RETURN(ctl::make(&m_spLosingFocusEventSource));
        m_spLosingFocusEventSource->Initialize(KnownEventIndex::DesktopWindowXamlSource_TakeFocusRequested, this, FALSE);
    }
    *ppEventSource = m_spLosingFocusEventSource.Get();
    return S_OK;
}

_Check_return_
HRESULT DesktopWindowXamlSource::OnFocusControllerLosingFocus(_In_ IInspectable*, _In_ IInspectable* args)
{
    if (m_spLosingFocusEventSource)
    {
        ctl::ComPtr<DesktopWindowXamlSource> spThis(this);
        ctl::ComPtr<xaml_hosting::IDesktopWindowXamlSource> spThat;
        IFC_RETURN(spThis.As(&spThat));
        ctl::ComPtr<xaml_hosting::IDesktopWindowXamlSourceTakeFocusRequestedEventArgs> spArgs;
        IFC_RETURN(ctl::ComPtr<IInspectable>(args).As(&spArgs));
        IFC_RETURN(m_spLosingFocusEventSource->Raise(spThat.Get(), spArgs.Get()));
    }
    return S_OK;
}

void DesktopWindowXamlSource::PrepareToClose()
{
    DirectUI::XamlIslandRoot* xamlIsland = m_spXamlIsland.Cast<XamlIslandRoot>();
    CXamlIslandRoot* xamlIslandCore = static_cast<CXamlIslandRoot*>(xamlIsland->GetHandle());
    CContentRoot* contentRoot = xamlIslandCore->GetContentRootNoRef();
    contentRoot->PrepareToClose();
}
