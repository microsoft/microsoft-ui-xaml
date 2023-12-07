// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Window.g.h"
#include <DependencyLocator.h>
#include <RuntimeEnabledFeatures.h>
#include "ScrollViewer.g.h"
#include "FrameworkApplication.g.h"
#include "WindowSizeChangedEventArgs.g.h"
#include "BackgroundTaskFrameworkContext.h"
#include "windows.ui.viewmanagement.h"
#include "CoreWindowWrapper.h"
#include <PixelFormat.h>
#include <windows.graphics.directx.h>
#include <windows.ui.input.h>
#include "IApplicationBarService.h"
#include <Grid.g.h>
#include <FeatureFlags.h>
#include <RootScale.h>
#include "SystemBackdrop.g.h"
#include <FeatureFlags.h>
#include <windows.ui.core.corewindow-defs.h>
#include <UWPWindowImpl.h>
#include <DesktopWindowImpl.h>
#include "host.h"
#include <XamlOneCoreTransforms.h>
#include <Windows.UI.Composition.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace RuntimeFeatureBehavior;

// Work around disruptive max/min macros
#undef max
#undef min

// ----------------------------------------------------------------------
//                             IWindowStatic
// ----------------------------------------------------------------------

_Check_return_ HRESULT WindowFactory::get_CurrentImpl(_Outptr_result_maybenull_ xaml::IWindow** pValue)
{
    *pValue = nullptr;

    ctl::ComPtr<xaml::IWindow> spRetVal;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    if (pCore)
    {
        // Determine XAML's runtime content: UWP or win32 Desktop
        AppPolicyWindowingModel policy = AppPolicyWindowingModel_None;
        LONG status = AppPolicyGetWindowingModel(GetCurrentThreadEffectiveToken(), &policy);
        if (status != ERROR_SUCCESS)
        {
            IFC_RETURN(E_FAIL);
        }

        // Window::Current calls on WinUI Desktop will always return NULL.
        // However, the test infrastructure makes heavy use of this function, restricting this here
        // on UWP mode results in catastrophic failures of the framework during init
        // this will likely need to be one of the last dummy window hooks that can be removed.
        // https://microsoft.visualstudio.com/OS/_workitems/edit/37016876
        if (policy != AppPolicyWindowingModel_ClassicDesktop)
        {
            spRetVal = pCore->GetDummyWindowNoRef();
        }
    }
    else
    {
        // We return NULL from Window.Current when the calling thread doesn't have a DXamlCore instance.
        // This can happen if Window.Current is called on a non-UI thread, or if it's called on a UI
        // thread before we've initialized DXamlCore or after we've deinitialized it. For example,
        // calling Window.Current in the Application object's constructor will return NULL.
    }

    RRETURN(spRetVal.CopyTo(pValue));
}

// ----------------------------------------------------------------------
//                               IWindow
// ----------------------------------------------------------------------
Window::Window()
{
    // The first window created internally by DXamlCore _must_ be a UWP Window.  DXamlCore
    // requires and controls the lifetime of a hidden UWP Microsoft.UI.Xaml.Window.
    // note that this Window instance will be the 'real' window for UWP instances, but
    // serves as a dummy for all other instances. dummy behavior is deprecated and being removed.
    auto dxamlCore = DXamlCore::GetCurrent();
    Window* window = dxamlCore->GetDummyWindowNoRef();

    if (!window)
    {
        // Do a runtime check to see if UWP should be enabled
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        auto UWPWindowEnabled = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnableUWPWindow);

        // WinUI UWP
        if (!UWPWindowEnabled && DXamlCore::GetCurrent()->GetHandle()->GetInitializationType() != InitializationType::IslandsOnly)
        {
            ::RoOriginateError(
                E_NOT_SUPPORTED,
                wrl_wrappers::HStringReference(
                L"WinUI: Error creating an UWP Window. Creating an UWP window is not allowed."
                ).Get());
            XAML_FAIL_FAST();
        }
        m_spWindowImpl = std::make_shared<UWPWindowImpl>(this);
    }
    else
    {
        // The XAML Application instance is required by UWP and always created in Desktop
        // by WindowsXamlManager.  There should be no case where Application::Current is NULL.
        AppPolicyWindowingModel policy = FrameworkApplication::GetCurrentNoRef()->GetAppPolicyWindowingModel();

        // WinUI UWP
        if (policy != AppPolicyWindowingModel_ClassicDesktop)
        {
           ::RoOriginateError(
                E_FAIL,
                wrl_wrappers::HStringReference(
                L"WinUI: Error creating second UWP Window on the current thread. No more than one UWP Window is allowed on a thread."
                ).Get());
            XAML_FAIL_FAST();
        }
        // WinUI Desktop
        else
        {
            m_spWindowImpl = std::make_shared<DesktopWindowImpl>(this);
        }
    }
}

_Check_return_ HRESULT Window::get_BoundsImpl(_Out_ wf::Rect* pValue)
{
    IFC_RETURN(m_spWindowImpl->get_BoundsImpl(pValue));

    return S_OK;
}

_Check_return_ HRESULT Window::get_VisibleImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(m_spWindowImpl->get_VisibleImpl(pValue));

    return S_OK;
}

_Check_return_ HRESULT Window::get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** ppValue)
{
    IFC_RETURN(m_spWindowImpl->get_ContentImpl(ppValue));

    return S_OK;
}

_Check_return_ HRESULT Window::put_ContentImpl(_In_opt_ xaml::IUIElement* pContent)
{

    IFC_RETURN(m_spWindowImpl->put_ContentImpl(pContent));

    return S_OK;
}

_Check_return_ HRESULT Window::get_CoreWindowImpl(_Outptr_result_maybenull_ wuc::ICoreWindow** ppValue)
{
    IFC_RETURN(m_spWindowImpl->get_CoreWindowImpl(ppValue));

    return S_OK;
}

//
// IMPORTANT: Don't use CoreDispatcher in XAML framework code.
//
// CoreDispatcher is exposed through our API for apps to use,
// but we need to avoid taking any internal dependencies on it,
// because it's not available in all environments we support.
//
// Instead, use DirectUI::IDispatcher. This is available through:
//     DXamlCore::GetXamlDispatcher()
//     DependencyObject::GetXamlDispatcher()
//
_Check_return_ HRESULT Window::get_DispatcherImpl(_Outptr_result_maybenull_ wuc::ICoreDispatcher** ppValue)
{
    IFC_RETURN(m_spWindowImpl->get_DispatcherImpl(ppValue));

    return S_OK;
}

_Check_return_ HRESULT Window::get_TitleImpl(_Out_ HSTRING* pValue)
{
    IFC_RETURN(m_spWindowImpl->get_TitleImpl(pValue));

    return S_OK;
}

_Check_return_ HRESULT Window::put_TitleImpl(_In_opt_ HSTRING value)
{
    IFC_RETURN(m_spWindowImpl->put_TitleImpl(value));

    return S_OK;
}

IFACEMETHODIMP Window::add_Activated(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_spWindowImpl->add_Activated(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP Window::remove_Activated(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spWindowImpl->remove_Activated(token));

    return S_OK;
}

IFACEMETHODIMP Window::add_Closed(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_spWindowImpl->add_Closed(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP Window::remove_Closed(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spWindowImpl->remove_Closed(token));

    return S_OK;
}

IFACEMETHODIMP Window::add_SizeChanged(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> * pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_spWindowImpl->add_SizeChanged(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP Window::remove_SizeChanged(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spWindowImpl->remove_SizeChanged(token));

    return S_OK;
}

IFACEMETHODIMP Window::add_VisibilityChanged(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(m_spWindowImpl->add_VisibilityChanged(pHandler, pToken));

    return S_OK;
}

IFACEMETHODIMP Window::remove_VisibilityChanged(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_spWindowImpl->remove_VisibilityChanged(token));

    return S_OK;
}

void Window::UnPeg()
{

    ASSERT(FrameworkApplication::GetCurrentNoRef()->GetAppPolicyWindowingModel() == AppPolicyWindowingModel_ClassicDesktop);
    // Desktop Window shall be unpegged when pegged window is being closed.
    if (m_peggedForHWNDLifetime)
    {
        this->UpdatePeg(false);
        m_peggedForHWNDLifetime = false;
    }
}

_Check_return_ HRESULT Window::ActivateImpl()
{
    // We peg only DesktopWindow, that's also only once on first Activate call. Pegging will keep window alive.
    // DesktopWindow will be unpegged when it's closing by calling UnPeg.
    if (!m_peggedForHWNDLifetime &&
        FrameworkApplication::GetCurrentNoRef()->GetAppPolicyWindowingModel() == AppPolicyWindowingModel_ClassicDesktop)
    {
        this->UpdatePeg(true);
        m_peggedForHWNDLifetime = true;
    }
    IFC_RETURN(m_spWindowImpl->ActivateImpl());

    return S_OK;
}

_Check_return_ HRESULT Window::CloseImpl()
{
    IFC_RETURN(m_spWindowImpl->CloseImpl());

    return S_OK;
}

// ----------------------------------------------------------------------
//                              IWindow3
// ----------------------------------------------------------------------

_Check_return_ HRESULT Window::get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor)
{
    IFC_RETURN(m_spWindowImpl->get_CompositorImpl(compositor));

    return S_OK;
}

_Check_return_ HRESULT Window::get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** iSystemBackdrop)
{
    IFC_RETURN(m_spWindowImpl->get_SystemBackdropImpl(iSystemBackdrop));

    return S_OK;
}

_Check_return_ HRESULT Window::put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* iSystemBackdrop)
{
    IFC_RETURN(CheckThread());

    IFC_RETURN(m_spWindowImpl->put_SystemBackdropImpl(iSystemBackdrop));

    return S_OK;
}

// ----------------------------------------------------------------------
//                          IWindowNative
// ----------------------------------------------------------------------
_Check_return_ HRESULT Window::get_WindowHandle(_Out_ HWND* pValue)
{
    IFC_RETURN(m_spWindowImpl->get_WindowHandle(pValue));

    return S_OK;
}


// ----------------------------------------------------------------------
//          MUC::ICompositionSupportsSystemBackdrop
// ----------------------------------------------------------------------
IFACEMETHODIMP Window::get_SystemBackdrop(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush)
{
    ARG_VALIDRETURNPOINTER(systemBackdropBrush);
    *systemBackdropBrush = nullptr;

    IFC_RETURN(CheckThread());

    IFC_RETURN(m_spWindowImpl->get_SystemBackdrop(systemBackdropBrush));

    return S_OK;
}

IFACEMETHODIMP Window::put_SystemBackdrop(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush)
{
    IFC_RETURN(CheckThread());

    IFC_RETURN(m_spWindowImpl->put_SystemBackdrop(systemBackdropBrush));

    return S_OK;
}

// ----------------------------------------------------------------------
//                          IWindowPrivate
// ----------------------------------------------------------------------

_Check_return_ HRESULT Window::get_TransparentBackgroundImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(m_spWindowImpl->get_TransparentBackgroundImpl(pValue));

    return S_OK;
}

_Check_return_ HRESULT Window::put_TransparentBackgroundImpl(_In_ BOOLEAN value)
{
    IFC_RETURN(m_spWindowImpl->put_TransparentBackgroundImpl(value));

    return S_OK;
}

_Check_return_ HRESULT Window::ShowImpl()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT Window::HideImpl()
{
    IFC_RETURN(m_spWindowImpl->HideImpl());

    return S_OK;
}

_Check_return_ HRESULT Window::MoveWindowImpl(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height)
{
    IFC_RETURN(m_spWindowImpl->MoveWindowImpl(x, y, width, height));

    return S_OK;
}

_Check_return_ HRESULT Window::SetAtlasSizeHintImpl(UINT width, UINT height)
{
    IFC_RETURN(m_spWindowImpl->SetAtlasSizeHintImpl(width, height));

    return S_OK;
}

_Check_return_ HRESULT Window::ReleaseGraphicsDeviceOnSuspendImpl(BOOLEAN enable)
{
    IFC_RETURN(m_spWindowImpl->ReleaseGraphicsDeviceOnSuspendImpl(enable));

    return S_OK;
}

_Check_return_ HRESULT Window::SetAtlasRequestCallbackImpl(_In_opt_ xaml::IAtlasRequestCallback* callback)
{
    IFC_RETURN(m_spWindowImpl->SetAtlasRequestCallbackImpl(callback));

    return S_OK;
}

_Check_return_ HRESULT Window::GetWindowContentBoundsForElementImpl(
    _In_ xaml::IDependencyObject* element,
    _Out_ wf::Rect* rect)
{
    IFC_RETURN(m_spWindowImpl->GetWindowContentBoundsForElementImpl(element, rect));

    return S_OK;
}

// ----------------------------------------------------------------------
//                          instance methods
// ----------------------------------------------------------------------

_Check_return_ HRESULT Window::EnsureInitializedForIslands()
{
    IFC_RETURN(m_spWindowImpl->EnsureInitializedForIslands());

    return S_OK;
}

_Check_return_ HRESULT Window::GetLayoutBounds(_Out_  wf::Rect* pLayoutBounds)
{
    IFC_RETURN(m_spWindowImpl->GetLayoutBounds(pLayoutBounds));

    return S_OK;
}

_Check_return_ HRESULT Window::GetLayoutBounds(_Out_  XRECTF* pLayoutBounds)
{
    IFC_RETURN(m_spWindowImpl->GetLayoutBounds(pLayoutBounds));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// HasBounds() checks whether it's currently valid to call get_Bounds().
//
// A XAML Window object can represent several OS-level window constructs
// (i.e. CoreWindow, HWND). It can also represent a state in which there is no
// backing window at all - e.g. when we've just started up and don't have
// a window yet, or when we're shutting down and have de-associated from a
// window.
//
// For these reasons, it's not always possible to obtain the size (bounds)
// of a Window object. If you want to handle these cases, you can call HasBounds()
// to check whether it's safe to call get_Bounds().
//
//-----------------------------------------------------------------------------
bool Window::HasBounds()
{
    return m_spWindowImpl->HasBounds();
}

_Check_return_ HRESULT Window::SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow)
{
    IFC_RETURN(m_spWindowImpl->SetCoreWindow(pCoreWindow));

    return S_OK;
}

_Check_return_ HRESULT Window::get_DispatcherQueueImpl(_Outptr_result_maybenull_ msy::IDispatcherQueue** ppValue)
{
    if (nullptr == ppValue) { IFC_RETURN(E_INVALIDARG); }
    *ppValue = nullptr;

    auto xamlCore = GetDXamlCore();
    if (xamlCore)
    {
        auto coreServices = xamlCore->GetHandle();
        if (coreServices)
        {
            auto hostSite = coreServices->GetHostSite();
            if (hostSite)
            {
                auto xcpDispatcher = hostSite->GetXcpDispatcher();
                if (xcpDispatcher)
                {
                    *ppValue = xcpDispatcher->GetDispatcherQueueNoRef();
                    AddRefInterface(*ppValue);
                }
            }
        }
    }

    return S_OK;
}

// Gets the client area rect of the window in window coordinates.  This method uses
// ApplicationView.VisibleBounds to account for OS chrome occlusion and RootScrollViewer
// height to detect an open SIP when ignoreIHM is False.
_Check_return_ HRESULT Window::GetVisibleBounds(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _Out_ wf::Rect* pValue)
{
    IFC_RETURN(m_spWindowImpl->GetVisibleBounds(ignoreIHM, inDesktopCoordinates, pValue));

    return S_OK;
}

_Check_return_ HRESULT Window::GetRootScrollViewer(_Outptr_result_maybenull_ IScrollViewer** ppRootScrollViewer)
{
    IFC_RETURN(m_spWindowImpl->GetRootScrollViewer(ppRootScrollViewer));

    return S_OK;
}

_Check_return_ HRESULT Window::OnWindowSizeChanged()
{
    IFC_RETURN(m_spWindowImpl->OnWindowSizeChanged());

    return S_OK;
}


DXamlCore* Window::GetDXamlCore()
{
    return m_spWindowImpl->GetDXamlCore();
}

void Window::SetDXamlCore(_In_opt_ DXamlCore* pDXamlCore)
{
    m_spWindowImpl->SetDXamlCore(pDXamlCore);
}

WindowType::Enum Window::GetWindowType()
{
    return m_spWindowImpl->GetWindowType();
}

// ----------------------------------------------------------------------------
//                              static methods
// ----------------------------------------------------------------------------

// Callback to app code to determine if the given surface allocation should be atlased.
/*static*/ bool Window::AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat)
{
    BOOLEAN result = true;

    // GetAtlasRequestCallback returns a member of WindowImpl of which the 'set' path is only implemented in UWPWindowImpl.
    // however, this window instance is relied upon by WPF (likely in test infrastructure), changing this from
    // dummy window results in catastrophic failure during init.
    // https://microsoft.visualstudio.com/OS/_workitems/edit/37016926
    Window* window = DXamlCore::GetCurrent()->GetDummyWindowNoRef();
    if (window != nullptr)
    {
        auto callback = window->m_spWindowImpl->GetAtlasRequestCallback();
        if (callback)
        {
            wgrdx::DirectXPixelFormat dxPixelFormat = wgrdx::DirectXPixelFormat_B8G8R8A8UIntNormalized;
            switch (pixelFormat)
            {
                case PixelFormat::pixelGray8bpp:
                    dxPixelFormat = wgrdx::DirectXPixelFormat_A8UIntNormalized;
                    break;
                case PixelFormat::pixelColor32bpp_A8R8G8B8:
                    dxPixelFormat = wgrdx::DirectXPixelFormat_B8G8R8A8UIntNormalized;
                    break;
                case PixelFormat::pixelColor64bpp_R16G16B16A16_Float:
                    dxPixelFormat = wgrdx::DirectXPixelFormat_R16G16B16A16Float;
                    break;
                default:
                    ASSERT(FALSE);
                    break;
            }

            IFCFAILFAST(callback->AtlasRequest(width, height, dxPixelFormat, &result));
        }
    }

    return result;
}

/*static*/ _Check_return_ HRESULT Window::Create(
    _In_ DXamlCore *pDXamlCore,
    _Outptr_ Window** ppWindow
    )
{
    // Note: We can't use ctl::make here, since the DXamlCore is not fully initialized.
    IFC_RETURN(ctl::ComObject<Window>::CreateInstance(ppWindow));
    (*ppWindow)->SetDXamlCore(pDXamlCore);

    return S_OK;
}

// For testing purposes only, guarantee that the visible bounds are at least shrunk by 100 logical px at the left and right edges of the screen,
// and 40 logical px at the top and bottom edges, when either the ShrinkApplicationViewVisibleBounds feature reg key is set, or the
// IXamlTestHooks::ShrinkApplicationViewVisibleBounds method was invoked.
/*static*/ _Check_return_ HRESULT Window::ShrinkApplicationViewVisibleBounds(_In_ Window* pCurrentWindow, _Inout_ wf::Rect* pApplicationViewVisibleBounds)
{
     bool shrinkApplicationViewVisibleBounds = pCurrentWindow->ShouldShrinkApplicationViewVisibleBounds();

#ifdef DBG
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    shrinkApplicationViewVisibleBounds |= runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::ShrinkApplicationViewVisibleBounds);
#endif

    if (shrinkApplicationViewVisibleBounds)
    {
        const float horizontalOverscan = 100.0f;
        const float verticalOverscan = 40.0f;
        const auto coreServices = DXamlCore::GetCurrent()->GetHandle();
        const auto rootCoordinator = coreServices->GetContentRootCoordinator();
        const auto mainRoot = rootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        const float zoomScale = RootScale::GetRasterizationScaleForContentRoot(mainRoot);
        float monitorHeight = 0.0f;
        float monitorWidth = 0.0f;

        if (XamlOneCoreTransforms::IsEnabled())
        {
            // While DisplayInformation Screen Width/Height does not account for any shell chrome (e.g. taskbar, window title
            // bar), that is OK because all we want to do is ensure that there is space around the "active" xaml surface where the
            // pointer can travel to.  It doesn't matter whether this is the desktop, chrome or artificial padding on the xaml
            // surface (which is what this method creates).
            ctl::ComPtr<wgrd::IDisplayInformation> displayInformation(DXamlCore::GetCurrent()->GetDisplayInformationNoRef());
            IFCPTR_RETURN(displayInformation);

            ctl::ComPtr<wgrd::IDisplayInformation4> displayInformation4;
            IFC_RETURN(displayInformation.As(&displayInformation4));

            UINT screenWidth;
            UINT screenHeight;
            IFC_RETURN(displayInformation4->get_ScreenWidthInRawPixels(&screenWidth));
            IFC_RETURN(displayInformation4->get_ScreenHeightInRawPixels(&screenHeight));

            monitorWidth = static_cast<float>(static_cast<double>(screenWidth) / zoomScale);
            monitorHeight = static_cast<float>(static_cast<double>(screenHeight) / zoomScale);
        }
        else
        {
            RECT rectFrom = {};
            HMONITOR hMonitor = MonitorFromRect(&rectFrom, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO monitorInfo = { sizeof(monitorInfo) };
            monitorInfo.cbSize = sizeof(monitorInfo);
            if (hMonitor == nullptr || !GetMonitorInfo(hMonitor, &monitorInfo))
            {
                // We have no way to get any monitor info, so just return.
                return S_OK;
            }
            monitorWidth = static_cast<float>(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) / zoomScale;
            monitorHeight = static_cast<float>(monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) / zoomScale;
        }

        const float left = horizontalOverscan - pApplicationViewVisibleBounds->X;
        if (left > 0.0f)
        {
            pApplicationViewVisibleBounds->X += left;
            pApplicationViewVisibleBounds->Width -= left;
        }

        const float top = verticalOverscan - pApplicationViewVisibleBounds->Y;
        if (top > 0.0f)
        {
            pApplicationViewVisibleBounds->Y += top;
            pApplicationViewVisibleBounds->Height -= top;
        }

        const float right = pApplicationViewVisibleBounds->Width - (monitorWidth - 2 * horizontalOverscan);
        if (right > 0.0f)
        {
            pApplicationViewVisibleBounds->Width -= right;
        }

        const float bottom = pApplicationViewVisibleBounds->Height - (monitorHeight - 2 * verticalOverscan);
        if (bottom > 0.0f)
        {
            pApplicationViewVisibleBounds->Height -= bottom;
        }
    }

    return S_OK;
}

/*static*/ _Check_return_ HRESULT Window::GetContentRootBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds)
{
    wf::Rect contentRootRect;

    pContentRootBounds->X = pContentRootBounds->Y = 0.0f;
    pContentRootBounds->Width = pContentRootBounds->Height = 0.0f;

    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(pObject, &contentRootRect));

    pContentRootBounds->X = contentRootRect.X;
    pContentRootBounds->Y = contentRootRect.Y;
    pContentRootBounds->Width = contentRootRect.Width;
    pContentRootBounds->Height = contentRootRect.Height;

    return S_OK;
}

/*static*/ _Check_return_ HRESULT Window::GetContentRootLayoutBounds(_In_ CDependencyObject* pObject, _Out_ XRECTF* pContentRootBounds)
{
    pContentRootBounds->X = pContentRootBounds->Y = 0.0f;
    pContentRootBounds->Width = pContentRootBounds->Height = 0.0f;

    wf::Rect contentRootRect{};
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(pObject, &contentRootRect));

    pContentRootBounds->X = contentRootRect.X;
    pContentRootBounds->Y = contentRootRect.Y;
    pContentRootBounds->Width = contentRootRect.Width;
    pContentRootBounds->Height = contentRootRect.Height;

    return S_OK;
}

/*static*/ _Check_return_ HRESULT Window::GetRootScrollViewer(
    _Outptr_result_maybenull_ CDependencyObject** ppRootScrollViewer)
{
    ctl::ComPtr<IScrollViewer> spRootScrollViewer;
    ctl::ComPtr<ScrollViewer> spRootScrollViewerAsSV;

    IFCPTR_RETURN(ppRootScrollViewer);
    *ppRootScrollViewer = NULL;

    // the window used here appears agnostic, but (non-static) Window::GetRootScrollViewer is only implemented in UWPWindowImpl
    // and this (static) function always uses the UWP Window as the instance (dummy) to retrieve this in a non-static way.
    // https://microsoft.visualstudio.com/OS/_workitems/edit/37012650
    if (const auto window = DXamlCore::GetCurrent()->GetDummyWindowNoRef())
    {
        IFC_RETURN(window->GetRootScrollViewer(&spRootScrollViewer));
        if (spRootScrollViewer)
        {
            spRootScrollViewerAsSV = spRootScrollViewer.Cast<ScrollViewer>();
            *ppRootScrollViewer = spRootScrollViewerAsSV.Cast<DependencyObject>()->GetHandleAddRef();
        }
    }

    return S_OK;
}

// -------------------------------------------------------
//              DXamlTestHooks
// -------------------------------------------------------
UINT32 Window::GetWidthOverride()
{
    return m_spWindowImpl->GetWidthOverride();
}

void Window::SetWidthOverride(UINT32 width)
{
    m_spWindowImpl->SetWidthOverride(width);
}

UINT32 Window::GetHeightOverride()
{
    return m_spWindowImpl->GetHeightOverride();
}

void Window::SetHeightOverride(UINT32 height)
{
    m_spWindowImpl->SetHeightOverride(height);
}

wf::Rect Window::GetLayoutBoundsOverrides()
{
    return m_spWindowImpl->GetLayoutBoundsOverrides();
}

void Window::SetLayoutBoundsOverrides(const wf::Rect& rect)
{
    m_spWindowImpl->SetLayoutBoundsOverrides(rect);
}

wf::Rect& Window::GetVisibleBoundsOverrides()
{
    return m_spWindowImpl->GetVisibleBoundsOverrides();
}

void Window::SetVisibleBoundsOverrides(const wf::Rect& rect)
{
    m_spWindowImpl->SetVisibleBoundsOverrides(rect);
}

void Window::SetHasSizeOverrides(bool hasSizeOverrides)
{
    m_spWindowImpl->SetHasSizeOverrides(hasSizeOverrides);
}

void Window::SetShrinkApplicationViewVisibleBounds(bool enabled)
{
    m_spWindowImpl->SetShrinkApplicationViewVisibleBounds(enabled);
}

// TODO: Move these
bool Window::ShouldShrinkApplicationViewVisibleBounds() const
{
    return m_spWindowImpl->ShouldShrinkApplicationViewVisibleBounds();
}

DirectUI::WindowImpl* Window::GetWindowImpl()
{
    return m_spWindowImpl.get();
}

_Check_return_ HRESULT Window::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IWindowNative)))
    {
        // The XAML Application instance is required by UWP and always created in Desktop
        // by WindowsXamlManager.  There should be no case where Application::Current is NULL.
        if (AppPolicyWindowingModel_ClassicDesktop != FrameworkApplication::GetCurrentNoRef()->GetAppPolicyWindowingModel())
        {
           ::RoOriginateError(
                E_NOINTERFACE,
                wrl_wrappers::HStringReference(
                L"The IWindowNative interface and support for obtaining the window handle (HWND) is only available on WinUI Desktop, not WinUI UWP."
                ).Get());

            return E_NOINTERFACE;
        }

        *ppObject = static_cast<IWindowNative*>(this);

        return AddRefOuter();
    }
    else
    {
        return __super::QueryInterfaceImpl(iid, ppObject);
    }
}

_Check_return_ HRESULT Window::get_ExtendsContentIntoTitleBarImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(m_spWindowImpl->get_ExtendsContentIntoTitleBarImpl(pValue));
    return S_OK;
}

_Check_return_ HRESULT Window::put_ExtendsContentIntoTitleBarImpl(_In_ BOOLEAN value)
{
    IFC_RETURN(m_spWindowImpl->put_ExtendsContentIntoTitleBarImpl(value));
    return S_OK;
}

_Check_return_ HRESULT Window::SetTitleBarImpl(_In_opt_ xaml::IUIElement* pTitleBar)
{
    IFC_RETURN(m_spWindowImpl->SetTitleBarImpl(pTitleBar));
    return S_OK;
}

_Check_return_ HRESULT Window::get_AppWindowImpl(_Outptr_result_maybenull_ ixp::IAppWindow** ppValue)
{
    IFC_RETURN(m_spWindowImpl->get_AppWindowImpl(ppValue));
    return S_OK;
}

