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
    if (!m_isDefaultCaptionButtonStyleSet)
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
    if (!IsChromeActive())
    {
        return false;
    }
    
    switch(uMsg)
    {
        case WM_CREATE:
            return !!OnCreate();
        case WM_PAINT :
            return !!OnPaint();
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
// - This method computes the height of the little border above the title bar
//   and returns it. If the border is disabled, then this method will return 0.
// Return Value:
// - the height of the border above the title bar or 0 if it's disabled
int CWindowChrome::GetTopBorderHeight() const noexcept
{
    // No border when maximized, or when the titlebar is invisible (by being in
    // fullscreen or focus mode).
    if (!IsTitlebarVisible() || IsMaximized(m_topLevelWindow))
    {
        return 0;
    }

    return topBorderVisibleHeight;
}

// Method Description:
// - Returns true if the titlebar is visible. For things like fullscreen mode,
//   borderless mode (aka "focus mode"), this will return false.
// Arguments:
// - <none>
// Return Value:
// - true iff the titlebar is visible
bool CWindowChrome::IsTitlebarVisible() const
{
    return IsChromeActive();
}


LRESULT CWindowChrome::OnPaint()
{
    PAINTSTRUCT ps{ 0 };
    const auto hdc = wil::BeginPaint(m_topLevelWindow, &ps);
    if (!hdc)
    {
        return 0;
    }

    const auto topBorderHeight = GetTopBorderHeight();

    if (ps.rcPaint.top < topBorderHeight)
    {
        RECT rcTopBorder = ps.rcPaint;
        rcTopBorder.bottom = topBorderHeight;

        // To show the original top border, we have to paint on top of it with
        // the alpha component set to 0. This page recommends to paint the area
        // in black using the stock BLACK_BRUSH to do this:
        // https://docs.microsoft.com/en-us/windows/win32/dwm/customframe#extending-the-client-frame
        ::FillRect(hdc.get(), &rcTopBorder, GetStockBrush(BLACK_BRUSH));
    }

    return 0;
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
        if (!::EqualRect(&m_dragRegionCached, &empty))
        {
            didRectChange = true;
            m_dragRegionCached = empty;
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
            if (!::EqualRect(&m_dragRegionCached, &dragBarRect))
            {
                didRectChange = true;
                m_dragRegionCached = dragBarRect;
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
    OnTitleBarSizeChanged();
}

_Check_return_ HRESULT CWindowChrome::SetDragRegion(RECT rf)
{
    // sometimes dragging needs to be disabled temporarily
    // for example : when content dialog's smoke screen is displayed
    wgr::RectInt32 rect = { 0, 0, 0, 0 };
    if (m_enabledDrag)
    {
        rect = { rf.left, rf.top, RectHelpers::rectWidth(rf), RectHelpers::rectHeight(rf) };
    }

    
    if (rect.Width == 0 || rect.Height == 0)
    {
        // clear out every drag region if user titlebar is defined as null
        // it will result in default case of AppWindowTitlebar being applied where a 
        // default drag region is covers the entire non client region
        IFC_RETURN(GetPeer()->GetNonClientInputPtrSrc()->ClearRegionRects(mui::NonClientRegionKind_Caption));
    }
    else
    {
        // Xaml works with logical (dpi-applied) client coordinates
        // InputNonClientPointerSource apis take non-dpi client coordinates
        // physical client coordinates = dpi applied coordinates * dpi scale
        float scale = WindowHelpers::GetCurrentDpiScale(m_topLevelWindow);
        rect = { 
            static_cast<int>(std::round(rect.X * scale)),
            static_cast<int>(std::round(rect.Y * scale)),
            static_cast<int>(std::round(rect.Width * scale)),
            static_cast<int>(std::round(rect.Height * scale))
        };
        wgr::RectInt32 rects[]{ rect };
        IFC_RETURN(GetPeer()->GetNonClientInputPtrSrc()->SetRegionRects(mui::NonClientRegionKind_Caption, 1, rects));
    }
    
    return S_OK;
}
