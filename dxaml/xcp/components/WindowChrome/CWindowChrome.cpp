// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "corep.h"
#include "style.h" 
#include "CWindowChrome.h"
#include "DXamlServices.h"
#include <WindowsX.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <ErrorHelper.h>
#include <XcpErrorResource.h>
#include "WindowChrome_Partial.h"
#include "comInstantiation.h"
#include <VisualStateManager.h>
#include "VisualTreeHelper.h"
#include "WindowHelpers.h"
#include "DiagnosticsInterop.h"
#include <FrameworkUdk/Theming.h>
#include "microsoft.ui.input.h"
#include "XamlRoot.g.h"
#include "Value.h"

using WindowChrome = DirectUI::WindowChrome;
using VisualTreeHelper = DirectUI::VisualTreeHelper;

namespace RectHelpers = WindowHelpers::RectHelpers;

_Check_return_ HRESULT CWindowChrome::Initialize(_In_ HWND parentWindow)
{
    m_topLevelWindow =  parentWindow;
    SetIsTabStop(false);

    return S_OK;
}

CWindowChrome::~CWindowChrome()
{
    m_topLevelWindow = NULL;
}

ctl::ComPtr<WindowChrome> CWindowChrome::GetPeer()
{
    ctl::ComPtr<DirectUI::DependencyObject> spPeer;
    IFCFAILFAST(DirectUI::DXamlServices::TryGetPeer(this, &spPeer));
    return spPeer.Cast<WindowChrome>();
}

// to apply min, max and close style definitions to custom titlebar,
// one needs to apply Content Control style with key WindowChromeStyle defined in generic.xaml
_Check_return_ HRESULT CWindowChrome::ApplyStyling()
{
    auto core = GetContext();
    xref_ptr<CStyle> windowChromeStyle;
    IFC_RETURN(core->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"WindowChromeStyle"),
                                                    reinterpret_cast<CDependencyObject **>(windowChromeStyle.ReleaseAndGetAddressOf())));
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Style, windowChromeStyle.get()));

    return S_OK;
}

_Check_return_ HRESULT CWindowChrome::ConfigureWindowChrome()
{
    const auto& windowChrome = GetPeer();
    auto appWindow = windowChrome->GetAppWindow();
    ctl::ComPtr<ixp::IAppWindowTitleBar> appWindowTitlebar;
    IFC_RETURN(appWindow->get_TitleBar(&appWindowTitlebar));
    IFC_RETURN(appWindowTitlebar->put_ExtendsContentIntoTitleBar(m_bIsActive)); // this will trigger a WM_MOVE and call OnTitleBarSizeChanged() immediately
    
    // Better default setting - Applying Transparent value as the default button bg color for caption buttons
    // This will be applied every time titlebar is enabled - whether first time or other times
    // this is the most common use of titlebar and this default setting makes it easier for customers to use it
    if (m_bIsActive && !m_isDefaultCaptionButtonStyleSet)
    {
        // only do it the first time in the lifetime of WindowChrome
        // so that if customer code changes caption button bg color to something
        // disable and re-enable titlebar then the configuration is not overwritten
        m_isDefaultCaptionButtonStyleSet = true; 
        wu::Color color = {0x0, 0xFF, 0xFF, 0xFF};
        ctl::ComPtr<wf::IReference<wu::Color>> box;
        IFC_RETURN(DirectUI::PropertyValue::CreateReference<wu::Color>(color, &box));
        IFC_RETURN(appWindowTitlebar->put_ButtonBackgroundColor(box.Get()));
        IFC_RETURN(appWindowTitlebar->put_ButtonInactiveBackgroundColor(box.Get()));

    }

    IFC_RETURN(SetFocusIfNeeded());
    return S_OK;
}

_Check_return_ HRESULT CWindowChrome::SetFocusIfNeeded()
{
    // WindowActivate for island/ win32 window has already had happened even before content of WindowChrome is loaded.
    // Due to this, SetWindowFocus which gets called on launch while handling WindowActivation has nothing to set focus on
    // and ends up in focusing nothing at all. This leads to launch an app without focus.
    // In order to fix this accessibility issue, here we are trying to set focus on first focusable element from the content of 
    // WindowChrome.

    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(this);
    if (m_bIsActive && focusManager->GetFocusedElementNoRef() == nullptr)
    {
        IFC_RETURN(focusManager->SetFocusOnNextFocusableElement(DirectUI::FocusState::Programmatic, true));
    }

    return S_OK;
}


_Check_return_ HRESULT CWindowChrome::SetIsChromeActive(bool isActive)
{
    auto guard = wil::scope_exit([&, this]()
    {
        m_bIsActive = !m_bIsActive; // reset active state in case of failure
    });

    if (m_bIsActive != isActive)
    {

        m_bIsActive = isActive;
        IFC_RETURN(ConfigureWindowChrome());

        if (!OnCreate())
        {
            return E_FAIL;
        }

        IFC_RETURN(RefreshToolbarOffset());
    }
    
    guard.release(); // success, no need to reset active state
    return S_OK;
}

bool CWindowChrome::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, _Out_ LRESULT* pResult)
{
    if (uMsg == WM_STYLECHANGED && wParam == static_cast<WPARAM>(GWL_STYLE) &&
        WindowHelpers::ShouldApplyDwmTopBorderWorkaround(m_topLevelWindow))
    {
        // A presenter can remove the frame without changing the client size.
        TRACE_HR_NORETURN(UpdateDwmFrameMargins());
    }

    if (!IsChromeActive())
    {
        return false;
    }
    
    switch(uMsg)
    {
        case WM_CREATE:
            return !!OnCreate();
    }

    return false;
}

LRESULT CWindowChrome::OnCreate()
{
    RECT rcClient = {};
    ::GetWindowRect(m_topLevelWindow, &rcClient);

    // Inform application of the frame change.
    return ::SetWindowPos(m_topLevelWindow, 
        NULL, 
        rcClient.left, rcClient.top,
        RectHelpers::rectWidth(rcClient), RectHelpers::rectHeight(rcClient),
        SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

// Method Description:
// - Computes the legacy spacing above XAML within the client area.
//   This is not a measurement of the native border or current XAML position.
// - Uses only cached Window.ExtendsContentIntoTitleBar and maximization state;
//   fullscreen and borderless presenters are not checked.
// Return Value:
// - 1 physical pixel when Window.ExtendsContentIntoTitleBar is enabled and the
//   window is not maximized; 0 otherwise.
int CWindowChrome::GetTopBorderHeight() const noexcept
{
    if (!IsTitlebarVisible() || IsMaximized(m_topLevelWindow))
    {
        return 0;
    }

    return topBorderVisibleHeight;
}

// True when the client area's top row needs special painting.  This can happen when ExtendsContentIntoTitleBar
// is enabled, in this case the WinUI content starts at y=1 because the DWM border is at y=0.
// False means the normal background erase handles that row.
// Query failures are logged and also return false.
bool CWindowChrome::ShouldPaintTopRowOfClientArea()
{
    const int topBorderHeight = GetTopBorderHeight();
    const auto style = ::GetWindowLongPtrW(m_topLevelWindow, GWL_STYLE);
    // Fullscreen and borderless windows can still leave a one-pixel gap above XAML.
    // That gap alone does not mean Windows has a native border to show through it.
    // WS_BORDER requests a simple border; WS_THICKFRAME requests a resizable frame.
    // If neither bit is set, use normal background painting and clear any WinUI-owned DWM margins.
    // HWND styles can change before AppWindow reports the new presenter, so check them now.
    // We also check OverlappedPresenter.HasBorder below: a resize style alone is not enough.
    if (topBorderHeight == 0 || (style & (WS_BORDER | WS_THICKFRAME)) == 0)
    {
        return false;
    }

    const auto windowChrome = GetPeer();
    // Close detaches the DesktopWindow before VisibilityChanged(false). Reentrant
    // HWND messages must not query AppWindow through the detached chrome.
    if (!windowChrome->GetDesktopWindowNoRef())
    {
        return false;
    }

    ctl::ComPtr<ixp::IAppWindow> appWindow;
    ctl::ComPtr<ixp::IAppWindowPresenter> presenter;
    ixp::AppWindowPresenterKind kind;
    if (FAILED_LOG(windowChrome->GetAppWindow(&appWindow)) || !appWindow)
    {
        return false;
    }
    if (FAILED_LOG(appWindow->get_Presenter(&presenter)))
    {
        return false;
    }
    if (!presenter)
    {
        LOG_HR(E_UNEXPECTED);
        return false;
    }
    if (FAILED_LOG(presenter->get_Kind(&kind)))
    {
        return false;
    }
    if (kind == ixp::AppWindowPresenterKind_FullScreen)
    {
        return false;
    }
    if (kind == ixp::AppWindowPresenterKind_Overlapped)
    {
        ctl::ComPtr<ixp::IOverlappedPresenter> overlappedPresenter;
        if (FAILED_LOG(presenter.As(&overlappedPresenter)))
        {
            return false;
        }
        boolean hasBorder = false;
        if (FAILED_LOG(overlappedPresenter->get_HasBorder(&hasBorder)))
        {
            return false;
        }
        if (!hasBorder)
        {
            return false;
        }
    }

    return true;
}

// Method Description:
// - Returns whether custom-title-bar mode is enabled by the cached
//   Window.ExtendsContentIntoTitleBar state (chrome active).
// - Does not check whether an app-supplied title-bar element is set or visible,
//   or whether the current presenter displays a title bar.
// - Setting only AppWindow.TitleBar.ExtendsContentIntoTitleBar does not enable it.
// Arguments:
// - <none>
// Return Value:
// - true when Window.ExtendsContentIntoTitleBar is enabled; false otherwise.
bool CWindowChrome::IsTitlebarVisible() const
{
    return IsChromeActive();
}

// Work around the missing Windows 10 top border by calling DwmExtendFrameIntoClientArea.
// Clear WinUI-owned margins when the native frame or reserved top row is removed.
_Check_return_ HRESULT CWindowChrome::UpdateDwmFrameMargins()
{
    ASSERT(WindowHelpers::ShouldApplyDwmTopBorderWorkaround(m_topLevelWindow));

    int topFrameMargin = 0;
    if (ShouldPaintTopRowOfClientArea())
    {
        RECT frame = {};
        const UINT dpi = ::GetDpiForWindow(m_topLevelWindow);
        const DWORD style = static_cast<DWORD>(::GetWindowLongPtrW(m_topLevelWindow, GWL_STYLE));
        const DWORD exStyle = static_cast<DWORD>(::GetWindowLongPtrW(m_topLevelWindow, GWL_EXSTYLE));

        IFCW32_RETURN(::AdjustWindowRectExForDpi(
            &frame,
            style,
            ::GetMenu(m_topLevelWindow) != nullptr,
            exStyle,
            dpi));

        // Follow Terminal's _UpdateFrameMargins workaround: extend the standard
        // caption/resize-frame height, not just the visible row. On Windows 10
        // 1809, a one-pixel extension leaves the inactive row untinted. XAML
        // still starts at y=1 and covers the rest of this extended frame.
        topFrameMargin = static_cast<int>(std::max<LONG>(-frame.top, topBorderVisibleHeight));
    }

    if ((!m_appliedDwmTopFrameMargin && topFrameMargin == 0) ||
        (m_appliedDwmTopFrameMargin && *m_appliedDwmTopFrameMargin == topFrameMargin))
    {
        // WinUI has not changed the margins for this window, or the WinUI-owned
        // value is already current. Do not issue an unnecessary all-margin write.
        return S_OK;
    }

    // DwmExtendFrameIntoClientArea writes all four margins and has no getter.
    // Once WinUI reserves this row, it owns the complete margin set for the
    // window. A zero value clears that WinUI-owned set when the row is removed.
    MARGINS margins = {};
    margins.cyTopHeight = topFrameMargin;
    IFC_RETURN(::DwmExtendFrameIntoClientArea(m_topLevelWindow, &margins));

    if (topFrameMargin > 0)
    {
        m_appliedDwmTopFrameMargin = topFrameMargin;
    }
    else
    {
        m_appliedDwmTopFrameMargin.reset();
    }

    return S_OK;
}

void CWindowChrome::UpdateContainerSize(WPARAM wParam, LPARAM lParam)
{
    
    UpdateBridgeWindowSizePosition();
    VERIFYHR(OnTitleBarSizeChanged());
}

void CWindowChrome::UpdateBridgeWindowSizePosition()
{
    HWND bridgeWindow = GetPeer()->GetPositioningBridgeWindowHandle();
    ASSERT(bridgeWindow);

    RECT clientRect = {};
    if (::GetClientRect(m_topLevelWindow, &clientRect) == FALSE)
    {
        IFCFAILFAST(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(
                                                                        HRESULT_FROM_WIN32(::GetLastError()),
                                                                        ERROR_WINDOW_DESKTOP_SIZE_OR_POSITION_FAILED));
    }
    
    RECT bridgeWindowRect = {};
    if (::GetClientRect(bridgeWindow, &bridgeWindowRect) == FALSE)
    {
        IFCFAILFAST(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(
                                                                        HRESULT_FROM_WIN32(::GetLastError()),
                                                                        ERROR_WINDOW_DESKTOP_SIZE_OR_POSITION_FAILED));
    }

    const auto windowWidth = RectHelpers::rectWidth(clientRect);
    const auto windowHeight = RectHelpers::rectHeight(clientRect);
    const auto topBorderHeight = WindowHelpers::ClampToShortMax(GetTopBorderHeight(), 0);

    if (WindowHelpers::ShouldApplyDwmTopBorderWorkaround(m_topLevelWindow))
    {
        TRACE_HR_NORETURN(UpdateDwmFrameMargins());
    }

    const COORD newIslandPos = { 0, topBorderHeight };
    

    const RECT newBridgeWindowRect = {newIslandPos.X, newIslandPos.Y, newIslandPos.X + windowWidth, newIslandPos.Y + windowHeight - topBorderHeight };
    // if top-level window is getting minimized then no need to resize composition window
    // if there is no change between old and new values, don't update comp window
    if( ::IsIconic(m_topLevelWindow) ||
        (windowHeight - topBorderHeight) == 0 ||
        ::EqualRect(&bridgeWindowRect, &newBridgeWindowRect))
    {
        return;
    }

    if (::SetWindowPos(bridgeWindow,
            HWND_BOTTOM,
            newIslandPos.X,
            newIslandPos.Y,
            windowWidth,
            windowHeight - topBorderHeight,
            SWP_SHOWWINDOW) == 0)
    {
        IFCFAILFAST(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(
                                                                        HRESULT_FROM_WIN32(::GetLastError()),
                                                                        ERROR_WINDOW_DESKTOP_SIZE_OR_POSITION_FAILED));
    }
}


// When the user provided titleBar changes size,
// then update the size and margin of the TitleBarMinMaxCloseContainer to match it
_Check_return_ HRESULT CWindowChrome::OnTitleBarSizeChanged()
{

    // if top-level window is getting minimized then no need to resize titlebar and dragbar window as they get minimized
    if(::IsIconic(m_topLevelWindow))
    {
        return S_OK;
    }

    bool didRectChange = false;
    if (!IsChromeActive())
    {
        RECT empty = {0, 0, 0, 0};
        if (m_enabledDrag != m_enabledDragCached || !::EqualRect(&m_dragRegionCached, &empty))
        {
            didRectChange = true;
            IFC_RETURN(SetDragRegion(empty));
        }
    }
    else 
    {
        auto userTitlebar = GetPeer()->GetUserTitleBarNoRef();
        const wf::Rect logicalWindowRect = WindowHelpers::GetLogicalWindowCoordinates(m_topLevelWindow);
        if (userTitlebar)
        {
            // this function gets called multiple times, in some times layout has not finished 
            // so width or height can be 0 in those case. skip setting the values in those times
            if (userTitlebar->GetActualWidth() == 0 || userTitlebar->GetActualHeight() == 0)
            {
                return S_OK;
            }
            
            RECT dragBarRect = WindowHelpers::GetClientAreaLogicalRectForUIElement(userTitlebar);
            if (m_enabledDrag != m_enabledDragCached || !::EqualRect(&m_dragRegionCached, &dragBarRect))
            {
                didRectChange = true;
                IFC_RETURN(SetDragRegion(dragBarRect));
            }
        }
    }
    
    if (didRectChange)
    {
        IFC_RETURN(RefreshToolbarOffset());
    }

    return S_OK;
}



HRESULT CWindowChrome::RefreshToolbarOffset()
{
    ctl::ComPtr<DirectUI::DependencyObject> peer;
    ctl::ComPtr<DirectUI::UIElement> element;
    ctl::ComPtr<xaml::IUIElement> titlebarPeer;
    auto xamlRoot = DirectUI::XamlRoot::GetForElementStatic(GetPeer().Get());

    if (m_bIsActive)
    {
        auto userTitlebar = GetPeer()->GetUserTitleBarNoRef();

        if (userTitlebar)
        {
            IFC_RETURN(DirectUI::DXamlServices::TryGetPeer(userTitlebar, &peer));
            IFC_RETURN(peer.As(&titlebarPeer));
            // If the titlebar is active, update the toolbar offset to account for it
            IFC_RETURN(Diagnostics::DiagnosticsInterop::UpdateToolbarOffset(titlebarPeer.Get()));
        }
        else // default case with no titlebar uielement assigned
        {
            IFC_RETURN(Diagnostics::DiagnosticsInterop::SetToolbarOffset(xamlRoot.Get(), defaultTitlebarHeight));
        }
    }
    else
    {
        // If the titlebar isn't active, there's no custom titlebar and no offset is needed
        IFC_RETURN(Diagnostics::DiagnosticsInterop::ClearToolbarOffset(xamlRoot.Get()));
    }

    return S_OK;
}


void CWindowChrome::UpdateCanDragStatus(bool enabled)
{
    m_enabledDrag = enabled;
    VERIFYHR(OnTitleBarSizeChanged());
}

_Check_return_ HRESULT CWindowChrome::SetDragRegion(RECT rf)
{
    UINT32 captionRectLength;
    wil::unique_cotaskmem_ptr<wgr::RectInt32[]> captionRects;

    IFC_RETURN(GetPeer()->GetNonClientInputPtrSrc()->GetRegionRects(mui::NonClientRegionKind_Caption, &captionRectLength, wil::out_param(captionRects)));

    std::vector<wgr::RectInt32> captionRegions{ captionRects.get(), captionRects.get() + captionRectLength };

    // We'll remove the region rect we previously added. We'll add the new one later, if we need to.
    captionRegions.erase(
        std::remove_if(
            captionRegions.begin(),
            captionRegions.end(),
            [&](wgr::RectInt32 const& rect)
            {
                return rect.X == m_scaledDragRegionCached.X &&
                    rect.Y == m_scaledDragRegionCached.Y &&
                    rect.Width == m_scaledDragRegionCached.Width &&
                    rect.Height == m_scaledDragRegionCached.Height;
            }),
        captionRegions.end());

    // sometimes dragging needs to be disabled temporarily
    // for example : when content dialog's smoke screen is displayed
    if (m_enabledDrag && !IsRectEmpty(&rf))
    {
        // Xaml works with logical (dpi-applied) client coordinates
        // InputNonClientPointerSource apis take non-dpi client coordinates
        // physical client coordinates = dpi applied coordinates * dpi scale
        float scale = WindowHelpers::GetCurrentDpiScale(m_topLevelWindow);
        m_scaledDragRegionCached = { 
            static_cast<int>(std::round(rf.left * scale)),
            static_cast<int>(std::round(rf.top * scale)),
            static_cast<int>(std::round(RectHelpers::rectWidth(rf) * scale)),
            static_cast<int>(std::round(RectHelpers::rectHeight(rf)* scale))
        };
    }
    else
    {
        m_scaledDragRegionCached = { 0, 0, 0, 0 };
    }

    captionRegions.push_back(m_scaledDragRegionCached);

    IFC_RETURN(GetPeer()->GetNonClientInputPtrSrc()->SetRegionRects(mui::NonClientRegionKind_Caption, captionRegions.size(), captionRegions.data()));
    
    m_dragRegionCached = rf;
    m_enabledDragCached = m_enabledDrag;
    return S_OK;
}
