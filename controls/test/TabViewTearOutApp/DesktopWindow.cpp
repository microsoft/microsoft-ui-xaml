#include "pch.h"

#include "DesktopWindow.h"
#include "resource.h"
#include <inspectable.h>

using namespace winrt;
using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Hosting;

const auto static invalidReason = static_cast<XamlSourceFocusNavigationReason>(-1);
static const WPARAM invalidKey = (WPARAM)-1;

static XamlSourceFocusNavigationReason GetReasonFromKey(WPARAM key)
{
    if (key == VK_TAB)
    {
        byte keyboardState[256] = {};
        WINRT_VERIFY(::GetKeyboardState(keyboardState));
        auto reason = (keyboardState[VK_SHIFT] & 0x80) ?
            XamlSourceFocusNavigationReason::Last :
            XamlSourceFocusNavigationReason::First;
        return reason;
    }
    if (key == VK_LEFT)
    {
        return XamlSourceFocusNavigationReason::Left;
    }
    if (key == VK_RIGHT)
    {
        return XamlSourceFocusNavigationReason::Right;
    }
    if (key == VK_UP)
    {
        return XamlSourceFocusNavigationReason::Up;
    }
    if (key == VK_DOWN)
    {
        return XamlSourceFocusNavigationReason::Down;
    }
    return invalidReason;
}

static WPARAM GetKeyFromReason(XamlSourceFocusNavigationReason reason)
{
    if (reason == XamlSourceFocusNavigationReason::Last || reason == XamlSourceFocusNavigationReason::First)
    {
        return VK_TAB;
    }
    if (reason == XamlSourceFocusNavigationReason::Left)
    {
        return VK_LEFT;
    }
    if (reason == XamlSourceFocusNavigationReason::Right)
    {
        return VK_RIGHT;
    }
    if (reason == XamlSourceFocusNavigationReason::Up)
    {
        return VK_UP;
    }
    if (reason == XamlSourceFocusNavigationReason::Down)
    {
        return VK_DOWN;
    }
    return invalidKey;
}

int DesktopWindow::s_windowCount{ 0 };

DesktopWindowXamlSource DesktopWindow::GetNextFocusedIsland(MSG* msg)
{
    if (msg->message == WM_KEYDOWN)
    {
        const auto key = msg->wParam;
        auto reason = GetReasonFromKey(key);
        if (reason != invalidReason)
        {
            const BOOL previous =
                (reason == XamlSourceFocusNavigationReason::First ||
                    reason == XamlSourceFocusNavigationReason::Down ||
                    reason == XamlSourceFocusNavigationReason::Right) ? false : true;

            const auto currentFocusedWindow = ::GetFocus();
            const auto nextElement = ::GetNextDlgTabItem(m_hMainWnd, currentFocusedWindow, previous);
            for (auto const& xamlSource : m_xamlSources)
            {
                HWND islandWnd = GetWindowFromWindowId(xamlSource.SiteBridge().WindowId());
                if (nextElement == islandWnd)
                {
                    return xamlSource;
                }
            }
        }
    }

    return nullptr;
}

DesktopWindowXamlSource DesktopWindow::GetFocusedIsland()
{
    for (auto const& xamlSource : m_xamlSources)
    {
        if (xamlSource.HasFocus())
        {
            return xamlSource;
        }
    }
    return nullptr;
}

bool DesktopWindow::NavigateFocus(MSG* msg)
{
    if (const auto nextFocusedIsland = GetNextFocusedIsland(msg))
    {
        WINRT_VERIFY(!nextFocusedIsland.HasFocus());
        const auto previousFocusedWindow = ::GetFocus();
        RECT rect = {};
        WINRT_VERIFY(::GetWindowRect(previousFocusedWindow, &rect));
        HWND islandWnd = GetWindowFromWindowId(nextFocusedIsland.SiteBridge().WindowId());
        POINT pt = { rect.left, rect.top };
        SIZE size = { rect.right - rect.left, rect.bottom - rect.top };
        ::ScreenToClient(islandWnd, &pt);
        const auto hintRect = winrt::Windows::Foundation::Rect({ static_cast<float>(pt.x), static_cast<float>(pt.y), static_cast<float>(size.cx), static_cast<float>(size.cy) });
        const auto reason = GetReasonFromKey(msg->wParam);
        const auto request = XamlSourceFocusNavigationRequest(reason, hintRect);
        lastFocusRequestId = request.CorrelationId();
        const auto result = nextFocusedIsland.NavigateFocus(request);
        return result.WasFocusMoved();
    }
    else
    {
        const bool islandIsFocused = GetFocusedIsland() != nullptr;
        byte keyboardState[256] = {};
        WINRT_VERIFY(::GetKeyboardState(keyboardState));
        const bool isMenuModifier = keyboardState[VK_MENU] & 0x80;
        if (islandIsFocused && !isMenuModifier)
        {
            return false;
        }
        const bool isDialogMessage = IsDialogMessage(m_hMainWnd, msg) != 0;
        return isDialogMessage;
    }
}

// ContentPreTranslateMessage is currently in the private/internal IXP package.  For now we just LoadLibrary/GetProcAddress for it,
// but let's call it directly when it's available.
typedef BOOL (__stdcall *ContentPreTranslateMessageFuncPtr)(const MSG *pmsg);

int DesktopWindow::MessageLoop(HACCEL hAccelTable)
{
    HMODULE windowingDll = ::LoadLibrary(L"Microsoft.UI.Windowing.Core.dll");
    WINRT_VERIFY(windowingDll != nullptr);

    auto contentPreTranslateMessagePtr = reinterpret_cast<ContentPreTranslateMessageFuncPtr>(
            ::GetProcAddress(windowingDll, "ContentPreTranslateMessage"));
    
    MSG msg = {};
    
    while (GetMessage(&msg, nullptr, 0, 0))
    {        
        if (!contentPreTranslateMessagePtr(&msg) && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            if (!NavigateFocus(&msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    FreeLibrary(windowingDll);

    return (int)msg.wParam;
}

void DesktopWindow::OnNCCreate(HWND const window, LPARAM const lparam) noexcept
{
    auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
    auto that = static_cast<DesktopWindow*>(cs->lpCreateParams);
    WINRT_ASSERT(that);
    WINRT_ASSERT(!that->GetHandle());
    that->m_hMainWnd = window;
    SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
}

void DesktopWindow::OnTakeFocusRequested(DesktopWindowXamlSource const& sender, DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args)
{
    if (args.Request().CorrelationId() != lastFocusRequestId)
    {
        const auto reason = args.Request().Reason();
        const BOOL previous =
            (reason == XamlSourceFocusNavigationReason::First ||
                reason == XamlSourceFocusNavigationReason::Down ||
                reason == XamlSourceFocusNavigationReason::Right) ? false : true;

        HWND senderHwnd = GetWindowFromWindowId(sender.SiteBridge().WindowId());

        MSG msg = {};
        msg.hwnd = senderHwnd;
        msg.message = WM_KEYDOWN;
        msg.wParam = GetKeyFromReason(reason);
        if (!NavigateFocus(&msg))
        {
            const auto nextElement = ::GetNextDlgTabItem(m_hMainWnd, senderHwnd, previous);
            ::SetFocus(nextElement);
        }
    }
    else
    {
        const auto request = XamlSourceFocusNavigationRequest(XamlSourceFocusNavigationReason::Restore);
        lastFocusRequestId = request.CorrelationId();
        sender.NavigateFocus(request);
    }
}

DesktopWindowXamlSource DesktopWindow::CreateDesktopWindowsXamlSource(DWORD dwStyle)
{
    DesktopWindowXamlSource desktopSource;

    // Parent the DesktopWindowXamlSource object to current window
    WindowId mainWndId = GetWindowIdFromWindow(m_hMainWnd);
    desktopSource.Initialize(mainWndId);

    // Get the new child window's hwnd 
    WindowId childWndId = desktopSource.SiteBridge().WindowId();
    HWND hWndXamlIsland = GetWindowFromWindowId(childWndId);
    DWORD dwNewStyle = GetWindowLong(hWndXamlIsland, GWL_STYLE);
    dwNewStyle |= dwStyle;
    SetWindowLong(hWndXamlIsland, GWL_STYLE, dwNewStyle);

    m_takeFocusEventRevokers.push_back(desktopSource.TakeFocusRequested(winrt::auto_revoke, { this, &DesktopWindow::OnTakeFocusRequested }));

    m_xamlSources.push_back(desktopSource);

    return desktopSource;
}

void DesktopWindow::RemoveDesktopWindowXamlSource(winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource dwxs)
{
    for (int i = 0; i < static_cast<int>(m_xamlSources.size()); ++i)
    {
        
        if (m_xamlSources[i].as<IInspectable>() == dwxs.as<IInspectable>())
        {
            m_xamlSources[i].Close();
            m_xamlSources.erase(m_xamlSources.begin() + i);
            m_takeFocusEventRevokers.erase(m_takeFocusEventRevokers.begin() + i);
            
            return;
        }
    }
}

void DesktopWindow::ClearXamlIslands()
{
    while (!m_xamlSources.empty())
    {
        RemoveDesktopWindowXamlSource(m_xamlSources.back());
    }
}
