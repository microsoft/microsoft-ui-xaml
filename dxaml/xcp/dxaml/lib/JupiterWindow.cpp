// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <AKExport.h>
#include <CoreWindow.h>
#include "CoreWindowWrapper.h"
#include "DragDropInternal.h"
#include <DragDropInterop.h>
#include "DropOperationTarget.h"
#include <FeatureFlags.h>
#include "focusmgr.h"
#include "FrameworkApplication_Partial.h"
#include <FrameworkUdk/DebugTool.h>
#include <IHwndComponentHost.h>
#include "InternalDebugInteropModel.h"
#include <isapipresent.h>
#include "JupiterWindow.h"
#include "JupiterControl.h"
#include "TouchHitTestingHandler.h"
#include "Window.g.h"
#include <windows.ui.viewmanagement.h>
#include <winpal.h>


#include <XamlTraceLogging.h>

#include <InputServices.h>
#include <KeyboardUtility.h>
#include <TextBoxBase.h>
#include <DesktopUtility.h>

#include <XamlOneCoreTransforms.h>

#include "TextCommon.h"
#include "MUX-ETWEvents.h"
#include "KeyboardAcceleratorUtility.h"
#include <RootScale.h>

#include "FocusObserver.h"

#include <windows.ui.core.corewindow-defs.h>
#include "InputSiteAdapter.h"

#include <FrameworkUdk/CoreWindowIntegration.h>
#include <Microsoft.UI.Input.Partner.h>
#include <WindowingCoreContentApi.h>

#include "WrlHelper.h"

#pragma warning(disable:4996) // use of apis marked as [[deprecated("PrivateAPI")]]

#define VK_COPY     VK_F16
#define VK_PASTE    VK_F17

// copied from WindowServer.h
const UINT WNDPROC_STATUS_OFFSET = (3 * sizeof(LONG_PTR));   // ULONG size return status from Jupiter WndProc

Microsoft::WRL::ComPtr<wuv::IApplicationViewStatics> CJupiterWindow::s_spApplicationViewStatics;

// msinkaut.h (which is already included) and peninputpanel.h (which contains the
// definition of MICROSOFT_TIP_OPENING_MSG) are incompatible and so we explicitly
// define the string here and rely on an assert when it is used to catch if
// it ever changes.
const WCHAR MICROSOFT_TIP_OPENING_MSG[] = L"TabletInputPanelOpening";

using namespace Microsoft::WRL;
using namespace ::Windows::Internal;

extern HINSTANCE g_hInstance;

InputMessage BuildInputMessage(_In_ const wsy::VirtualKey virtualKey,
    _In_ const ixp::PhysicalKeyStatus keyStatus,
    _In_ const XUINT32 modifierKeys,
    _In_ const MessageMap messageId)
{
    InputMessage inputMsg = {};
    inputMsg.m_msgID = messageId;

    inputMsg.m_platformKeyCode = virtualKey;
    inputMsg.m_physicalKeyStatus.m_uiRepeatCount = keyStatus.RepeatCount;
    inputMsg.m_physicalKeyStatus.m_uiScanCode = keyStatus.ScanCode;
    inputMsg.m_physicalKeyStatus.m_bIsExtendedKey = !!keyStatus.IsExtendedKey;
    inputMsg.m_physicalKeyStatus.m_bIsMenuKeyDown = !!keyStatus.IsMenuKeyDown;
    inputMsg.m_physicalKeyStatus.m_bWasKeyDown = !!keyStatus.WasKeyDown;
    inputMsg.m_physicalKeyStatus.m_bIsKeyReleased = !!keyStatus.IsKeyReleased;

    return inputMsg;
}

_Check_return_ HRESULT CJupiterWindow::ConfigureJupiterWindow(
    _In_opt_ wuc::ICoreWindow* pCoreWindow,
    _In_ CJupiterControl* pControl,
    _Outptr_ CJupiterWindow** ppWindow)
{
    HRESULT hr = S_OK;
    ICoreWindowInterop* pCoreWindowInterop = nullptr;
    HWND hwnd = nullptr;
    CJupiterWindow* pJupiterWindow = nullptr;

    if (pCoreWindow)
    {
        IFC(pCoreWindow->QueryInterface(__uuidof(ICoreWindowInterop), reinterpret_cast<void**>(&pCoreWindowInterop)));

        IFC(pCoreWindowInterop->get_WindowHandle(&hwnd));

        //TODO: Task# 28854849 Update Create function from CJupiterWindow to always use ExpCompositionContent.
        IFC(Create(hwnd, WindowType::CoreWindow, pControl, &pJupiterWindow));

        IFC(pJupiterWindow->SetCoreWindow(pCoreWindow));

        //  Staging subclass WNDPROC to serve during refactoring from HWND to CoreWindow; subclassing to be deprecated entirely.
        pJupiterWindow->m_subclassedWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CJupiterWindow::StaticCoreWindowSubclassProc)));
        if (0 == pJupiterWindow->m_subclassedWndProc)
        {
            IFC(E_FAIL);
        }
    }
    else
    {
        IFC(Create(nullptr /*hwnd*/, WindowType::DesktopWindow, pControl, &pJupiterWindow));

        IFC(pJupiterWindow->SetCoreWindow(nullptr));
    }
    if (pCoreWindow && !IsXamlBehaviorEnabledForCurrentSku(AllHwndFeatures_NotSupported))
    {
        // Desktop/full-user32 only
        // The Win32 model for WM_CHANGEUISTATE/WM_UPDATEUISTATE is that focus indicators are visible by default,
        // and then a WM_UPDATEUISTATE message will arrive and indicate they should be hidden if they need to be.
        // Set last input device type to keyboard initially, and we'll handle that WM_UPDATEUISTATE message when it comes.
        CContentRoot* contentRoot = pJupiterWindow->GetCoreWindowContentRootNoRef();
        contentRoot->GetInputManager().SetLastInputDeviceType(DirectUI::InputDeviceType::Keyboard);

        // Tell Windows to propagate a WM_UPDATEUISTATE message that containing information about the last
        // input device used.  We use this to decide whether or not to show focus rectangles.  This SendMesssage results
        // in a synchronous call to the window proc with WM_UPDATEUISTATE if we should be hiding accelerators or
        // focus rectangles.
        ::SendMessage(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);
    }

    IFC(pJupiterWindow->UpdateFontScale());

    *ppWindow = pJupiterWindow;
    pJupiterWindow = nullptr;

Cleanup:
    ReleaseInterface(pCoreWindowInterop);
    ReleaseInterface(pJupiterWindow);

    return hr;
}

_Check_return_ HRESULT CJupiterWindow::SetCoreWindow(_In_opt_ wuc::ICoreWindow* coreWindow)
{
    ASSERT(m_pCoreWindow == nullptr);
    SetInterface(m_pCoreWindow, coreWindow);

    const auto pDxamlCore = DirectUI::DXamlCore::GetCurrent();
    if (!pDxamlCore->IsInBackgroundTask())
    {
        // CONTENT-TODO: OneCoreTransforms is not supported by lifted IXP at this time.
        if (XamlOneCoreTransforms::IsEnabled())
        {
            // If the compositor is not already created, this will ensure that it gets created now. Creating the compositor
            // will then create the composition island for the core window
            m_pControl->GetCoreServices()->EnsureCompositionIslandCreated(coreWindow);
        }

        IFC_RETURN(SetInitialCursor());
        if (m_pCoreWindow)
        {
            IFC_RETURN(RegisterCoreWindowEvents());
        }
        IFC_RETURN(RegisterUISettingsEvents());

        // Initial OnThemeChanged() because the theme may have changed between
        // when the DXamlCore was initialized and when we've actually registered for
        // the event notifications just now
        OnThemeChanged();

        // For Drag and Drop
        if (m_pCoreWindow)
        {
            IFC_RETURN(RegisterDropTargetRequested());
        }
    }

    // Initial OnSizeChanged() to set initial state.
    if (m_pCoreWindow)
    {
        IFC_RETURN(OnSizeChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::Create(_In_ HWND hwnd, WindowType::Enum windowType, _In_ CJupiterControl* pControl, _Outptr_ CJupiterWindow** ppWindow)
{
    *ppWindow = nullptr;

    ctl::ComPtr<CJupiterWindow> spWindow;
    ctl::make(hwnd, windowType, &spWindow);

    spWindow->SetControl(pControl);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(spWindow.Get()));

    *ppWindow = spWindow.Detach();

    return S_OK;
}

// Staging static subclass WNDPROC to serve during refactoring from HWND to CoreWindow; to be deprecated.
LRESULT CALLBACK CJupiterWindow::StaticCoreWindowSubclassProc(_In_ HWND hwnd, UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    CJupiterWindow* pWindow = reinterpret_cast<CJupiterWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // note: pWindow can be nullptr: we can get window messages before we've associated the CJupiterWindow pointer with the HWND

    if (pWindow)
    {
        return pWindow->CoreWindowSubclassProc(hwnd, uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

_Check_return_ HRESULT CJupiterWindow::ShowWindow()
{
    TraceShowWindowBegin();

    FAIL_FAST_ASSERT(m_hwnd != nullptr);
    ::ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);

    TraceShowWindowEnd();

    ::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INTERNALPAINT | RDW_UPDATENOW);

    return S_OK;
}

void CJupiterWindow::SetControl(_In_ CJupiterControl* pControl)
{
    m_pControl = pControl;
}

CJupiterWindow::CJupiterWindow() :
    m_hwnd(nullptr),
    m_windowType(WindowType::Enum::None),
    m_pControl(nullptr),
    m_pCoreWindow(nullptr),
    m_subclassedWndProc(nullptr),
    m_wasWindowEverActivated(false),
    m_windowActivationState(JupiterWindowActivationState::Activatable),
    m_fWindowDestroyed(false),
    m_fHandledAppsKeyUpForContextMenu(false),
    m_fHandledShiftF10KeyDownForContextMenu(false),
    m_visibilityChangedToken(),
    m_sizeChangedToken(),
    m_dropTargetRequestedToken(),
    m_activatedToken(),
    m_colorValuesChangedToken(),
    m_fontScaleChangedToken()
{
    XCP_WEAK(&m_pControl);

    // Get the windows message that will tell us if there is an on screen keyboard
    // present (and being used) so we know to register for open/close events.  If
    // for some reason this call fails, we will ignore the message (by pretending
    // that we already saw it) and let the legacy methods kick in (such as registering
    // on focus change)
    m_inputPanelMessage = ::RegisterWindowMessage(MICROSOFT_TIP_OPENING_MSG);
    ASSERT(m_inputPanelMessage != 0);
    m_registerInputPaneHandler = m_inputPanelMessage != 0;
}

_Check_return_ HRESULT CJupiterWindow::Initialize(_In_ HWND hwnd, WindowType::Enum windowType)
{
    m_hwnd = hwnd;
    m_windowType = windowType;

    return S_OK;
}

CJupiterWindow::~CJupiterWindow()
{
    if (m_subclassedWndProc)
    {
        // un-subclass
        SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_subclassedWndProc));
        m_subclassedWndProc = nullptr;
    }

    if (m_pCoreWindow)
    {
        IGNOREHR(UnregisterCoreWindowEvents());
    }
    IGNOREHR(UnregisterUISettingsEvents());

    IGNOREHR(UnregisterDropTargetRequested());

    ReleaseInterface(m_pCoreWindow);
}

FocusObserver* CJupiterWindow::GetFocusObserverNoRef() const
{
    const auto contentRoot = GetCoreWindowContentRootNoRef();
    if (contentRoot != nullptr)
    {
        return contentRoot->GetFocusManagerNoRef()->GetFocusObserverNoRef();
    }

    return nullptr;
}

bool IsWin32InputMessage(UINT msg)
{
    return
        (msg == WM_POINTERUPDATE
            || msg == WM_POINTERDOWN
            || msg == WM_POINTERUP
            || msg == WM_POINTERENTER
            || msg == WM_POINTERLEAVE
            || msg == WM_POINTERACTIVATE
            || msg == WM_POINTERCAPTURECHANGED
            || msg == WM_TOUCHHITTESTING
            || msg == WM_POINTERWHEEL
            || msg == WM_POINTERHWHEEL
            || msg == WM_KEYDOWN
            || msg == WM_KEYUP
            || msg == WM_SYSKEYDOWN
            || msg == WM_SYSKEYUP
            || msg == WM_CHAR
            || msg == WM_SETFOCUS
            || msg == WM_KILLFOCUS
            || msg == WM_POINTERROUTEDTO
            || msg == WM_POINTERROUTEDAWAY
            || msg == WM_POINTERROUTEDRELEASED
            || msg == DM_POINTERHITTEST
            );
}

// Staging instance subclass WNDPROC to serve during refactoring from HWND to CoreWindow; to be deprecated.
LRESULT CALLBACK CJupiterWindow::CoreWindowSubclassProc(_In_ HWND hwnd, UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    ASSERT(WindowType::CoreWindow == m_windowType);

    bool fHandled = false;
    bool fDefaultToControl = false;
    LRESULT lresult = 0;

    // See if we need to register for open/close events on the on screen keyboard.
    if (m_registerInputPaneHandler && uMsg == m_inputPanelMessage && m_pCoreWindow != nullptr)
    {
        DirectUI::DXamlCore* pCore = DirectUI::DXamlCore::GetCurrent();
        if (pCore)
        {
            m_registerInputPaneHandler = false;
            pCore->RegisterInputPaneHandler(m_pCoreWindow);
        }
    }

    // We no longer handle input through window messages, just call the next window proc
    if (IsWin32InputMessage(uMsg))
    {
        // Just do nothing and return
        if (m_subclassedWndProc)
        {
            return CallWindowProc(m_subclassedWndProc, hwnd, uMsg, wParam, lParam);
        }
        else
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    switch (uMsg)
    {
        case WM_GETOBJECT:  // Todo: replace with ICoreWindow::AutomationProviderRequested, followed by deep refactoring
            {
                // Avoiding processing WM_GETOBJECT while we are laready processing WM_DESTROY. While we are still destroying
                // the windows some UIA Client could send WM_GETOBJECT as HWND is still alive and that will lead to unexpected situations.
                if (m_fWindowDestroyed)
                {
                    return 0;
                }

                if (m_pControl && m_pControl->GetCoreServices()->UseUiaOnMainWindow())
                {
                    // Handle the WM_GETOBJECT message only if the main window has content.
                    // (If it doesn't, we're only showing islands; don't hook up UIA to the main window.
                    // Each island's UIA tree will get hooked up separately.)

                    lresult = m_pControl->HandleGetObjectMessage(uMsg, wParam, lParam);
                    fHandled = true;
                }
            }
            fDefaultToControl = false;
            break;

        //  The following messages are handled by CoreWindow events, which delegate to the control as appropriate.
        //  They should not be delegated via subclassing
        case WM_DESTROY:
            OnCoreWindowDestroying();
            fDefaultToControl = false;
            break;
        case WM_POWERBROADCAST:
        case WM_SIZE:
        case WM_MOVE:
        case WM_TOUCHHITTESTING:
        case WM_CONTEXTMENU:
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_CHAR:
        case WM_SYSCHAR:
            fDefaultToControl = false;
            break;

        case WM_PAINT:
        case WM_UPDATEUISTATE:
            // Process these messages, even if the XAML Window has no content
            fDefaultToControl = true;
            break;

        case DM_POINTERHITTEST:
            fDefaultToControl = true;
            break;

        default:
            // Process all the remaining messages if Window.Content is set.  If this XAML core only has islands, and no
            // content bound to the CoreWindow, we ignore messages by default.
            fDefaultToControl = m_pControl->GetCoreServices()->HandleInputOnMainWindow();
            break;
    }

    //  Delegate unhandled messages to the control
    if (fDefaultToControl && m_pControl)
    {
        fHandled = m_pControl->HandleWindowMessage(uMsg, wParam, lParam, GetCoreWindowContentRootNoRef());
    }

    // pass handled status to CoreWindow wndproc
    // NOTE: This is NOT a bug on WOW or x64.
    // We are intentionally using SetWindowLong and not SetWindowLongPtr to match
    // a corresponding GetWindowLong call in CoreWindow code (see WindowServer.cpp).
    SetWindowLong(hwnd, WNDPROC_STATUS_OFFSET, fHandled);

    LRESULT savedLresult = lresult;

    if (m_subclassedWndProc)
    {
        lresult = CallWindowProc(m_subclassedWndProc, hwnd, uMsg, wParam, lParam);
    }
    else
    {
        lresult = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    // Need to return our saved lresult if this is a handled WM_GETOBJECT message.
    if (uMsg == WM_GETOBJECT && fHandled)
    {
        lresult = savedLresult;
    }
    return lresult;
}

_Check_return_ HRESULT CJupiterWindow::GetCoreWindow(_Outptr_result_maybenull_ wuc::ICoreWindow** ppWindow)
{
    *ppWindow = m_pCoreWindow;
    AddRefInterface(*ppWindow);
    return S_OK;
}

wuc::ICoreWindow* CJupiterWindow::GetCoreWindowNoRef()
{
    return m_pCoreWindow;
}

_Check_return_ HRESULT CJupiterWindow::RunCoreWindowMessageLoop()
{
    HRESULT hr = S_OK;
    wuc::ICoreDispatcher* pCoreDispatcher = nullptr;

    IFCEXPECT(m_pCoreWindow);
    IFC(m_pCoreWindow->get_Dispatcher(&pCoreDispatcher));

    IFC(pCoreDispatcher->ProcessEvents(wuc::CoreProcessEventsOption_ProcessUntilQuit));

Cleanup:
    ReleaseInterface(pCoreDispatcher);
    return hr;
}

_Check_return_ HRESULT CJupiterWindow::PreTranslateMessage(
    _In_opt_ CContentRoot* contentRoot,
    _In_ mui::IInputPreTranslateKeyboardSourceInterop* source,
    _In_ mui::IInputKeyboardSourceInterop* keyboardSource,
    _In_ const MSG* msg,
    _In_ UINT keyboardModifiers,
    _In_ bool focusPass,
    _Inout_ bool* handled)
{
    *handled = false;

    if (contentRoot == nullptr)
    {
        return S_OK;
    }

    // We ignore the normal tree pass of pre-translation when focused because we perform all of
    // our has-focus accelerator processing on the focus pass.
    //
    // For now we also ignore the normal (non-focused) tree pass when pre translating messages. We
    // eventually want to support raising accelerator events for Xaml islands that are not focused,
    // and the non-focus pass is where we would do that. However, the current behavior for
    // multi-window Xaml Desktop apps is for the focused window to perform accelerator processing
    // and the non-focused window(s) to not perform any accelerator processing. To keep this
    // behavior, we ignore all passes except for the focus pass.
    if (!focusPass)
    {
        return S_OK;
    }

    const auto virtualKey = static_cast<wsy::VirtualKey>(msg->wParam);
    const bool isFocusKey =
        virtualKey == wsy::VirtualKey::VirtualKey_Tab ||
        virtualKey == wsy::VirtualKey::VirtualKey_Left ||
        virtualKey == wsy::VirtualKey::VirtualKey_Right ||
        virtualKey == wsy::VirtualKey::VirtualKey_Up ||
        virtualKey == wsy::VirtualKey::VirtualKey_Down;
    const auto virtualKeyModifiers =
        ((keyboardModifiers & FCONTROL) ? wsy::VirtualKeyModifiers::VirtualKeyModifiers_Control : wsy::VirtualKeyModifiers::VirtualKeyModifiers_None) |
        ((keyboardModifiers & FALT) ? wsy::VirtualKeyModifiers::VirtualKeyModifiers_Menu : wsy::VirtualKeyModifiers::VirtualKeyModifiers_None) |
        ((keyboardModifiers & FSHIFT) ? wsy::VirtualKeyModifiers::VirtualKeyModifiers_Shift : wsy::VirtualKeyModifiers::VirtualKeyModifiers_None);
    const bool modifierIsShift = virtualKeyModifiers == wsy::VirtualKeyModifiers::VirtualKeyModifiers_Shift;
    const bool modifierIsNone = virtualKeyModifiers == wsy::VirtualKeyModifiers::VirtualKeyModifiers_None;
    const bool isFocusModifier = (modifierIsShift && isFocusKey) || modifierIsNone;
    const bool isFocusMessage = isFocusKey && isFocusModifier;

    if (isFocusMessage)
    {
        if (!focusPass)
        {
            return S_OK;
        }


        if (msg->wParam > UINT_MAX)
        {
            XAML_FAIL_FAST(); // process only when wParam fits in 32 bit as required by PreTranslateKeyboardMessage API
        }

        IFC_RETURN(AcceleratorKeyActivated(
            contentRoot,
            msg,
            keyboardModifiers,
            virtualKey,
            handled));

        if (*handled)
        {
            return S_OK;
        }


        // Accelerator key not handled, so send the message to the keyboard input source to raise the
        // normal keyboard input events instead.
        IFC_RETURN(keyboardSource->SendKeyboardMessage(msg, handled));
    }
    else
    {
        if (focusPass)
        {
            return S_OK; // If we have focus, then let the accelerator come normally over WM_KEYDOWN on CInputManager::ProcessKeyboardInput
        }

        if (msg->wParam > UINT_MAX)
        {
            XAML_FAIL_FAST(); // process only when wParam fits in 32 bit as required by PreTranslateKeyboardMessage API
        }

        IFC_RETURN(AcceleratorKeyActivated(
            contentRoot,
            msg,
            keyboardModifiers,
            virtualKey,
            handled));

        if (*handled)
        {
            return S_OK;
        }

        // Here we only care about the global accelerators as we are outside of Island. Note that
        // we also only process accelerators on WM_KEYDOWN or WM_SYSKEYDOWN, not "char" or "up".
        if ((msg->message == WM_KEYDOWN) ||
            (msg->message == WM_SYSKEYDOWN))
        {
                auto liveAccelerators = contentRoot->GetAllLiveKeyboardAccelerators();
                *handled = KeyboardAcceleratorUtility::ProcessGlobalAccelerators(
                    virtualKey,
                    virtualKeyModifiers,
                    liveAccelerators
                );
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::AcceleratorKeyActivated(
    _In_ CContentRoot* contentRoot,
    _In_ const MSG* msg,
    _In_ UINT keyboardModifiers,
    _In_ wsy::VirtualKey virtualKey,
    _Inout_ bool* handled)
{
    HRESULT hr = S_OK;
    bool fProcessTabKey = false;

    IXcpBrowserHost *pBrowerHost = m_pControl->GetBrowserHost();
    CCoreServices* pCoreService = pBrowerHost->GetContextInterface();

    const unsigned int repeatCount = (msg->lParam & 0x0000FFFF);
    TraceAcceleratorKeyActivatedInfo(virtualKey, repeatCount);

    // Process Tab key (Tab or Shift+Tab) only when the Ctrl and Alt keys are not pressed.
    // For example OneNote wants to process Ctrl+Tab or Ctrl+Shift+Tab for page navigation
    // instead of the default tab navigation.
    if (virtualKey == wsy::VirtualKey_Tab)
    {
        if (pBrowerHost)
        {
            if (pCoreService)
            {
                if (!(keyboardModifiers & FALT) && !(keyboardModifiers & FCONTROL))
                {
                    fProcessTabKey = true;
                }
            }
        }
    }

    // To handle tabbing out of XAML when it is in hosted activation (for example, in FilePicker),
    // temporarily turn off Tab cycling. This will cause Focus Manager to not handle the Tab
    // after the last focusable control is reached, so ICoreWindowEventArgs::put_Handled(FALSE)
    // will be called. In hosted activation, this will cause the Tab to be processed by the host,
    // which will tab out of XAML (see shell's CHostedWindowManager::PostTranslateAccelerator). When
    // not hosted, this will cause Tab to be received through the WindowProc input path, after
    // Tab cycling is turned back on, and Focus Manager will cycle then.
    if ((fProcessTabKey) && (msg->message == WM_KEYDOWN))
    {
        contentRoot->GetFocusManagerNoRef()->SetCanTabOutOfPlugin(TRUE);
    }

    if (WM_KEYDOWN != msg->message)
    {
        goto Cleanup;
    }

    if (virtualKey != wsy::VirtualKey_Tab)
    {
        goto Cleanup;
    }

    // In the case of a hosted hwnd, we may not want to handle or mark handled the TAB here,
    // but rather let it flow down into the hosted hwnd.
    // Currently, the only instance of this is WebView2, where we want the tab to be handled by
    // the CoreWebView2 rather than Xaml.
    CDependencyObject* focusedObject = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
    // WebView2 derives from Panel, scope to QI only on Panel elements
    if (focusedObject && focusedObject->GetTypeIndex() == KnownTypeIndex::Panel)
    {
        ctl::ComPtr<DirectUI::DependencyObject> peer;
        IFC(DirectUI::DXamlCore::GetCurrent()->TryGetPeer(focusedObject, &peer));
        if (peer)
        {
            ctl::ComPtr<IUnknown> host;
            HRESULT qiResult = ctl::iinspectable_cast(peer.Get())->QueryInterface(__uuidof(IHwndComponentHost), reinterpret_cast<void**>(host.ReleaseAndGetAddressOf()));
            if (SUCCEEDED(qiResult) && host != nullptr)
            {
                goto Cleanup;
            }
        }
    }

    // When the XAML window is hosted (for example, in FilePicker), it is responsible for setting focus
    // on its window when the user tabs into XAML from the host. So if TAB(or SHIFT+TAB) is received
    // when the XAML window or its child windows don't have focus, set focus on the XAML window.
    if (fProcessTabKey && !contentRoot->GetFocusManagerNoRef()->IsPluginFocused() &&
        !::IsChild(m_hwnd, ::GetFocus()))
    {
        IGNOREHR(m_inputSiteAdapter->SetFocus());

        // Calling SetFocus() will generate WM_SETFOCUS and InputManager will set focus on the first
        // focusable control in case of no focus on the content.
        // ClearFocus() is for clearing focus that is set by calling SetFocus(). We want to clear the focus
        // before calling HandleWindowMessage() that will set the focus properly on the first or last
        // focusable control by handling Tab or Shift+Tab key.
        // The focus will be the first(Tab) or last(Shift+Tab) control of Jupiter content.
        contentRoot->GetFocusManagerNoRef()->ClearFocus();
    }

    {
        *handled = m_pControl->HandleWindowMessage(WM_KEYDOWN, virtualKey, 0, contentRoot);
    }

    // Tab cycling was turned off, so Focus Manager will not handle Tab after the last focusable element
    // is reached. Clear focus on the last focusable control, so that focus can be set on the
    // the first focusable element on the next Tab.
    if (fProcessTabKey && !(*handled))
    {
        // DEBUG: Commenting this out should cause it to mimic the logonui behavior...
        //        This should ease debugging (and allow test hooks to mimic logonui behavior, especially if it also skips this entire function)
        contentRoot->GetFocusManagerNoRef()->ClearFocus();
    }

Cleanup:

    if ((virtualKey == wsy::VirtualKey_Tab) && (msg->message == WM_KEYDOWN))
    {
        contentRoot->GetFocusManagerNoRef()->SetCanTabOutOfPlugin(FALSE);
    }

    return hr;
}

_Check_return_ HRESULT CJupiterWindow::SetInitialCursor()
{
    HRESULT hr = S_OK;
    wrl_wrappers::HStringReference coreCursorAcid(RuntimeClass_Windows_UI_Core_CoreCursor);
    wuc::ICoreCursorFactory* pCoreCursorFactory = nullptr;
    wuc::ICoreCursor* pCursor = nullptr;

    if (!m_pCoreWindow)
    {
        return S_OK;
    }

    IFC(wf::GetActivationFactory(coreCursorAcid.Get(), &pCoreCursorFactory));
    IFC(pCoreCursorFactory->CreateCursor(
        wuc::CoreCursorType_Arrow,
        0,
        &pCursor));

    IFC(m_pCoreWindow->put_PointerCursor(pCursor));

Cleanup:
    ReleaseInterface(pCoreCursorFactory);
    ReleaseInterface(pCursor);
    return hr;
}

_Check_return_ HRESULT CJupiterWindow::SetLayoutCompletedNeeded(const LayoutCompletedNeededReason reason)
{
    IXcpBrowserHost* pBrowserHost = m_pControl->GetBrowserHost();
    if (!pBrowserHost)
    {
        return S_OK;
    }

    CCoreServices* pCoreServices = pBrowserHost->GetContextInterface();
    if (!pCoreServices)
    {
        return S_OK;
    }

    IFC_RETURN(pCoreServices->SetLayoutCompletedNeeded(reason));

    return S_OK;
}

XSIZE CJupiterWindow::GetJupiterWindowPhysicalSize(HWND hwndContext) const
{
    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In XamlOneCoreTransforms, use CoreWindow bounds rather than calling GetClientRect.
        // Note this returns a scaled size, in DIPs, whereas the other GetClientRect
        // codepath returns a physical pixel-based size.  For now, we match this.
        // 11236439 tracks rationalizing the scaling story in strict mode.
        wf::Rect coreWindowBounds = {};
        IFCFAILFAST(m_pCoreWindow->get_Bounds(&coreWindowBounds));

        const auto coreServices = m_pControl->GetCoreServices();
        const auto rootCoordinator = coreServices->GetContentRootCoordinator();
        const auto mainRoot = rootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        const float scale = RootScale::GetRasterizationScaleForContentRoot(mainRoot);

        const XSIZE strictSize = {
            static_cast<XINT32>(ceilf(coreWindowBounds.Width * scale)),
            static_cast<XINT32>(ceilf(coreWindowBounds.Height * scale))};
        return strictSize;
    }
    else
    {
        FAIL_FAST_ASSERT(hwndContext);
        XSIZE size = { 0, 0 };
        RECT clientRect;
        if (GetClientRect(hwndContext, &clientRect))
        {
            size.Width = clientRect.right;
            size.Height = clientRect.bottom;
        }

        return size;
    }
}

wrl::ComPtr<ixp::IInputPointerSource> CJupiterWindow::GetInputSiteAdapterInputPointerSource()
{
    return m_inputSiteAdapter->GetInputPointerSource();
}

wrl::ComPtr<ixp::IPointerPoint> CJupiterWindow::GetInputSiteAdapterPointerPoint()
{
    return m_inputSiteAdapter->GetPreviousPointerPoint();
}

_Check_return_ HRESULT CJupiterWindow::OnSizeChanged()
{
    if (WindowType::CoreWindow == m_windowType)
    {
        // In the CoreWindow case, we do some extra work to process the ApplicationViewState.
        wuv::ApplicationViewState applicationViewState {};

        if (!s_spApplicationViewStatics)
        {
            IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(), &s_spApplicationViewStatics));
        }

        IFC_RETURN(s_spApplicationViewStatics->get_Value(&applicationViewState));

        bool fIsFullScreen =    ((applicationViewState == wuv::ApplicationViewState_FullScreenLandscape)
                              ||  (applicationViewState == wuv::ApplicationViewState_FullScreenPortrait));

        IXcpBrowserHost* pBrowserHost = m_pControl->GetBrowserHost();
        pBrowserHost->SetFullScreen(fIsFullScreen);

        TraceCoreWindowResizeFiredInfo(static_cast<XUINT32>(applicationViewState));
    }

    IFC_RETURN(m_pControl->OnJupiterWindowSizeChanged(*this, m_hwnd));

    // Now that we opt in to the new WinBlue LayoutCompleted mechanism (see SetShouldWaitForLayoutCompletion),
    // we need to call LayoutCompleted following re-layout after a size change.
    IFC_RETURN(SetLayoutCompletedNeeded(LayoutCompletedNeededReason::WindowSizeChanged));

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::ProcessCharEvents(
    _Inout_ ixp::ICharacterReceivedEventArgs *args,
    _In_opt_ CContentRoot* contentRoot)
{
    if (!contentRoot)
    {
        return S_OK;
    }

    if (!m_pControl)
    {
        return S_OK;
    }

    bool keyHandled = false;

    const auto& akExport = contentRoot->GetAKExport();
    IFC_RETURN(akExport.TryProcessInputForCharacterReceived(args, &keyHandled));

    if (!keyHandled)
    {
        UINT32 wParam = 0;
        UINT32 lParam = 0;

        IFC_RETURN(PackIntoWin32StyleCharArgs(args, WM_CHAR, &wParam, &lParam));
        keyHandled = m_pControl->HandleWindowMessage(WM_CHAR, wParam, lParam, contentRoot);
    }

    IFC_RETURN(args->put_Handled(keyHandled));

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::ProcessCopyAndPaste(_In_ wsy::VirtualKey virtualKey)
{
    IXcpBrowserHost *pBrowserHost = m_pControl->GetBrowserHost();
    IFCPTR_RETURN(pBrowserHost);

    CCoreServices* pCoreServices = pBrowserHost->GetContextInterface();
    IFCPTR_RETURN(pCoreServices);

    CDependencyObject *pDOFocusElem = nullptr;
    const auto contentRoot = GetCoreWindowContentRootNoRef();
    if (contentRoot != nullptr)
    {
        pDOFocusElem = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
    }

    if (nullptr == pDOFocusElem)
    {
        return S_OK;
    }

    if (VK_COPY == virtualKey)
    {
        // These controls support copy: CTextBlock, CRichEditBox, CRichTextBlockOverflow, CRichTextBlock, CTextBox, CPasswordBox
        switch (pDOFocusElem->GetTypeIndex())
        {
            case KnownTypeIndex::TextBlock:
                IFC_RETURN(do_pointer_cast<CTextBlock>(pDOFocusElem)->CopySelectedText());
                break;

            case KnownTypeIndex::RichTextBlock:
                IFC_RETURN(do_pointer_cast<CRichTextBlock>(pDOFocusElem)->CopySelectedText());
                break;

            case KnownTypeIndex::RichTextBlockOverflow:
                IFC_RETURN(do_pointer_cast<CRichTextBlockOverflow>(pDOFocusElem)->CopySelectedText());
                break;

            case KnownTypeIndex::RichEditBox:
            case KnownTypeIndex::TextBox:
            case KnownTypeIndex::PasswordBox:
                IFC_RETURN(do_pointer_cast<CTextBoxBase>(pDOFocusElem)->Copy());
                break;
        }
    }
    else // VK_PASTE == wParam
    {
        // CTextBox, CRichEditBox and CPasswordBox can be pasted into.
        switch (pDOFocusElem->GetTypeIndex())
        {
            case KnownTypeIndex::RichEditBox:
            case KnownTypeIndex::TextBox:
            case KnownTypeIndex::PasswordBox:
                IFC_RETURN(do_pointer_cast<CTextBoxBase>(pDOFocusElem)->Paste());
                break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::ProcessKeyEvents(
    _Inout_ ixp::IKeyEventArgs* pArgs,
    _In_ UINT32 uMsg,
    _In_opt_ CContentRoot* contentRoot,
    _In_ bool processContextMenu)
{
    if (!contentRoot)
    {
        return S_OK;
    }

    bool handled = false;
    UINT32 wParam = 0;
    UINT32 lParam = 0;

    wsy::VirtualKey virtualKey;
    IFC_RETURN(pArgs->get_VirtualKey(&virtualKey));
    if (VK_COPY == virtualKey || VK_PASTE == virtualKey)
    {
        IFC_RETURN(ProcessCopyAndPaste(virtualKey));
        return S_OK;
    }

    if (!m_pControl || !m_pControl->GetCoreServices())
    {
        return S_OK;
    }

    IFC_RETURN(InputUtility::Keyboard::PackIntoWin32StyleKeyArgs(pArgs, uMsg, &wParam, &lParam));

    MsgPacket packet;
    packet.m_pCoreWindow = GetCoreWindowNoRef();
    packet.m_hwnd = GetWindowHandle();
    packet.m_wParam = wParam;
    packet.m_lParam = lParam;

    IFC_RETURN(contentRoot->GetInputManager().ProcessKeyEvent(pArgs, uMsg, static_cast<XHANDLE>(&packet), &handled));

    if (processContextMenu) { handled = handled || ProcessContextMenu(uMsg, wParam, lParam, handled, contentRoot); }

    // VK processing in certain apps (e.g.: Calc) depends on IsHandled to be false for KeyArgs.
    // We need to ensure that we will always reset IsHandled to the value that we calculate here.
    IFC_RETURN(pArgs->put_Handled(handled));

    return S_OK;
}

bool CJupiterWindow::ProcessFocusEvents(
    _In_ UINT32 uMsg,
    _In_opt_ CContentRoot* contentRoot)
{
    if (!contentRoot)
    {
        return false;
    }

    const bool handled = !!m_pControl->HandleWindowMessage(uMsg, 0, 0, contentRoot);
    return handled;
}

// Helper to populate Win32-style LPARAM and WPARAM args for character events
// from a WinRT CharReceivedEventArgs object
_Check_return_ HRESULT CJupiterWindow::PackIntoWin32StyleCharArgs(
    _In_ ixp::ICharacterReceivedEventArgs* pArgs,
    _In_ UINT32 uMsg,
    _Out_ UINT32* wParam,
    _Out_ UINT32* lParam)
{
    ixp::PhysicalKeyStatus keyStatus;

    IFC_RETURN(pArgs->get_KeyCode(wParam));

    IFC_RETURN(pArgs->get_KeyStatus(&keyStatus));

    *lParam |= (keyStatus.RepeatCount & 0x0000FFFF);      // bits 0-15
    *lParam |= ((keyStatus.ScanCode & 0x000000FF) << 16); // bits 16-23

    if (keyStatus.IsExtendedKey)
    {
        *lParam |= ((LPARAM)1 << 24);
    }
    if (keyStatus.IsMenuKeyDown)
    {
        *lParam |= ((LPARAM)1 << 29);
    }
    if (keyStatus.WasKeyDown)
    {
        *lParam |= ((LPARAM)1 << 30);
    }
    if (keyStatus.IsKeyReleased)
    {
        *lParam |= ((LPARAM)1 << 31);
    }

    return S_OK;
}

bool CJupiterWindow::ProcessContextMenu(
    _In_ UINT32 uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ bool wasKeyEventHandled,
    _In_opt_ CContentRoot* contentRoot)
{
    if (!contentRoot)
    {
        return false;
    }

    bool fHandled = wasKeyEventHandled;
    bool fProcessContextMenuMessage = false;

    if (m_pControl)
    {
        // Process WM_CONTEXTMENU window message to raise RightTapped event.
        //  This allows for processing the unhandled VK_APPS KeyUp, Shift+F10 KeyDown message or
        //                 without the WM_KEYDOWN/UP message(e.g. remote control).
        if ((uMsg == WM_CONTEXTMENU && !(m_fHandledShiftF10KeyDownForContextMenu || m_fHandledAppsKeyUpForContextMenu)))
        {
            fProcessContextMenuMessage = true;
        }

        // Process the window messages.
        if (fProcessContextMenuMessage)
        {
            fHandled = m_pControl->HandleWindowMessage(uMsg, wParam, lParam, contentRoot);
        }

        //  CoreWindow::KeyDown, CoreWindow::KeyUp events,
        //  threshold IInternalCoreWindow::SysKeyDown, IInternalCoreWindow::SysKeyUp events
        if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP)
        {
            if (wParam == VK_APPS)
            {
                // WM_CONTEXTMENU is followed with VK_APPS KeyUp.
                m_fHandledAppsKeyUpForContextMenu = (uMsg == WM_KEYDOWN) ? false : fHandled;
            }

            XUINT32 modifierKeys = 0;
            IFCFAILFAST(gps->GetKeyboardModifiersState(&modifierKeys));

            if (wParam == VK_F10 && (modifierKeys & KEY_MODIFIER_SHIFT))
            {
                // WM_CONTEXTMENU is followed with Shift+F10 KeyDown.
                m_fHandledShiftF10KeyDownForContextMenu = (uMsg == WM_SYSKEYDOWN) ? fHandled : false;
            }
        }
    }

    // Do not pass WM_CONTEXTMENU to CoreWindow not to process WM_CONTEXTMENU(that generate EdgeGesture)
    // if WM_KEYUP(VK_APPS), Shift+F10 KeyDown or WM_CONTEXTMENU is handled.
    if (uMsg == WM_CONTEXTMENU && (m_fHandledShiftF10KeyDownForContextMenu || m_fHandledAppsKeyUpForContextMenu))
    {
        fHandled = true;
    }

    return fHandled;
}

void CJupiterWindow::ResetContextMenuState()
{
    m_fHandledShiftF10KeyDownForContextMenu = false;
    m_fHandledAppsKeyUpForContextMenu = false;
}

_Check_return_ HRESULT CJupiterWindow::OnCoreWindowVisibilityChanged(
    _In_ wuc::ICoreWindow* pSender,
    _Inout_ wuc::IVisibilityChangedEventArgs* pArgs)
{
    IFC_RETURN(UpdateWindowVisibility(pSender, false));

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnDropTargetRequested(_In_ wadt::DragDrop::Core::ICoreDragDropManager* /* pSender */, _In_ wadt::DragDrop::Core::ICoreDropOperationTargetRequestedEventArgs* /* pArgs */)
{
    /*
    ctl::ComPtr<DirectUI::DropOperationTarget> spDropTarget;
    IFC_RETURN(ctl::make<DirectUI::DropOperationTarget>(&spDropTarget));
    IFC_RETURN(pArgs->SetTarget(spDropTarget.Get()));
    */

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnCoreWindowSizeChanged(
    _In_ wuc::ICoreWindow * pSender,
    _Inout_ wuc::IWindowSizeChangedEventArgs * pArgs)
{
    ASSERT(WindowType::CoreWindow == m_windowType);

    IGNOREHR(OnSizeChanged());

    return S_OK;
}

STDMETHODIMP CJupiterWindow::OnCoreWindowPositionChanged()
{
    if (m_pControl)
    {
        m_pControl->HandleWindowMessage(WM_MOVE, 0, 0, GetCoreWindowContentRootNoRef());
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnCoreWindowCharacterReceived(
    _In_ wuc::ICoreWindow*,
    _Inout_ wuc::ICharacterReceivedEventArgs* args)
{
    return S_OK;
}

// Island input
_Check_return_ HRESULT CJupiterWindow::OnIslandGotFocus(
    _In_opt_ CContentRoot* contentRoot)
{
    ProcessFocusEvents(WM_SETFOCUS, contentRoot);
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandLostFocus(
    _In_opt_ CContentRoot* contentRoot)
{
    ProcessFocusEvents(WM_KILLFOCUS, contentRoot);
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandCharacterReceived(
    _In_ ixp::ICharacterReceivedEventArgs* e,
    _In_opt_ CContentRoot* contentRoot)
{
    IFC_RETURN(ProcessCharEvents(e, contentRoot));
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandDirectManipulationHitTest(
    _In_opt_ CContentRoot* contentRoot,
    _In_ ixp::IPointerEventArgs* args)
{
    if (!contentRoot)
    {
        return S_OK;
    }

    wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFCFAILFAST(args->get_CurrentPoint(&pointerPoint));

    UINT32 pointerId = 0;
    IFCFAILFAST(pointerPoint->get_PointerId(&pointerId));

    bool handled = false;
    IFCFAILFAST(OnIslandDirectManipulationHitTest(contentRoot, pointerPoint.Get(), &handled));
    IFCFAILFAST(args->put_Handled(handled));

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandDirectManipulationHitTest(
    _In_opt_ CContentRoot* contentRoot,
    _In_ ixp::IPointerPoint* pointerPoint,
    _Out_ bool* handled)
{
    if (!contentRoot)
    {
        return S_OK;
    }

    UINT32 pointerId = 0;
    IFCFAILFAST(pointerPoint->get_PointerId(&pointerId));

    *handled = !!m_pControl->HandlePointerMessage(DM_POINTERHITTEST, static_cast<WPARAM>(pointerId), 0, contentRoot, false /* isGeneratedMessage */, pointerPoint);

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandKeyDown(
    _In_ ixp::IKeyEventArgs* e,
    _In_opt_ CContentRoot* contentRoot)
{
    IFC_RETURN(ProcessKeyEvents(e, WM_KEYDOWN, contentRoot));
    IFC_RETURN(e->put_Handled(TRUE));
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandKeyUp(
    _In_ ixp::IKeyEventArgs* e,
    _In_opt_ CContentRoot* contentRoot)
{
    IFC_RETURN(ProcessKeyEvents(e, WM_KEYUP, contentRoot));
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandSysKeyDown(
    _In_ ixp::IKeyEventArgs* e,
    _In_opt_ CContentRoot* contentRoot)
{
    IFC_RETURN(ProcessKeyEvents(e, WM_SYSKEYDOWN, contentRoot));

    wsy::VirtualKey virtualKey {};
    IFC_RETURN(e->get_VirtualKey(&virtualKey));

    bool handled = true;

    if (virtualKey == wsy::VirtualKey_F4)
    {
        XUINT32 modifierKeys {};
        IFC_RETURN(gps->GetKeyboardModifiersState(&modifierKeys));
        if ((modifierKeys & KEY_MODIFIER_ALT) != 0)
        {
            // If we get an ALT+F4 here, mark it as unhandled to let it through to the
            // defwindowproc so that the system will handle it and close the window.
            handled = false;
        }
    }

    IFC_RETURN(e->put_Handled(!!handled));

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandSysKeyUp(
    _In_ ixp::IKeyEventArgs* e,
    _In_opt_ CContentRoot* contentRoot)
{
    IFC_RETURN(ProcessKeyEvents(e, WM_SYSKEYUP, contentRoot));
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandPointerMessage(
    const UINT msg,
    _In_opt_ CContentRoot* contentRoot,
    _In_ ixp::IPointerPoint* pointerPoint,
    _In_ ixp::IPointerEventArgs* pointerEventArgs,
    const bool isReplayedMessage,
    _Out_ bool* handled)
{
    *handled = false;

    if (!contentRoot)
    {
        return S_OK;
    }

    UINT32 pointerId = 0;
    IFCFAILFAST(pointerPoint->get_PointerId(&pointerId));

    *handled = !!m_pControl->HandlePointerMessage(msg, static_cast<WPARAM>(pointerId), 0, contentRoot, isReplayedMessage, pointerPoint, pointerEventArgs);

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnIslandMessage(
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_opt_ CContentRoot* contentRoot)
{
    if (!contentRoot)
    {
        return S_OK;
    }

    // Ignore the return value -- it reports whether or not XAML handled the message
    m_pControl->HandleWindowMessage(uMsg, wParam, lParam, contentRoot);
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnUISettingsFontScaleChanged(
    _In_ wuv::IUISettings*,
    _Inout_ IInspectable*)
{
    VERIFYHR(UpdateFontScale());
    return S_OK;
}

void CJupiterWindow::OnCoreWindowDestroying()
{
    if (!m_pCoreWindow)
    {
        return;
    }

    // TODO: null out all references to the hwnd (for example see CInputServices::m_hWnd): bug 685188.
    // for now, code can query IXcpHostSite::IsWindowDestroyed().
    m_fWindowDestroyed = true;

    // Calling from CJupiterWindow into DXamlCore here introduces coupling.
    // If necessary we could refactor to use a listener approach instead.
    DirectUI::DXamlCore* pCore = DirectUI::DXamlCore::GetCurrent();
    if (pCore)
    {
        FAIL_FAST_ASSERT(m_hwnd);
        pCore->OnWindowDestroyed(GetWindowHandle());
    }

    // for now we simply disable ticks which should prevent most of our code that references the window handle from using it
    if (m_pControl)
    {
        // SetTicksEnabled(false) can not actually fail in the current implementation, so this is safe.
        IGNOREHR(m_pControl->SetTicksEnabled(false));
    }
}

_Check_return_ HRESULT CJupiterWindow::OnCoreWindowActivated(
    _In_ wuc::ICoreWindow * pSender,
    _Inout_ wuc::IWindowActivatedEventArgs * pArgs)
{
    wuc::CoreWindowActivationState activeState = wuc::CoreWindowActivationState::CoreWindowActivationState_CodeActivated;

    if (m_pControl)
    {
        IFC_RETURN(pArgs->get_WindowActivationState(&activeState));

        // We don't receive WM_ACTIVATE messages through WindowProc, so we insert one when we hit this event
        m_pControl->HandleWindowMessage(WM_ACTIVATE, activeState == wuc::CoreWindowActivationState::CoreWindowActivationState_Deactivated ? 0 : WA_ACTIVE, 0, GetCoreWindowContentRootNoRef());
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::RegisterCoreWindowEvents()
{
    ASSERT(WindowType::CoreWindow == m_windowType);
    ASSERT(m_pCoreWindow != nullptr);

    ComPtr<wuc::ICoreWindow> spCoreWindow(m_pCoreWindow);

    if (!m_visibilityChangedToken.value)
    {
        IFC_RETURN(spCoreWindow->add_VisibilityChanged(
            Microsoft::WRL::Callback<wf::ITypedEventHandler<
                wuc::CoreWindow*,
                wuc::VisibilityChangedEventArgs*>> (
                    this,
                    &CJupiterWindow::OnCoreWindowVisibilityChanged).Get(),
            &m_visibilityChangedToken));

        // Force update the window visibility state on creation.
        // Pass true for isStartingUp param.
        IFC_RETURN(UpdateWindowVisibility(m_pCoreWindow, true));
    }

    if (!m_sizeChangedToken.value)
    {
        IFC_RETURN(spCoreWindow->add_SizeChanged(
            Microsoft::WRL::Callback<wf::ITypedEventHandler<
                wuc::CoreWindow*,
                wuc::WindowSizeChangedEventArgs*>> (
                    this,
                    &CJupiterWindow::OnCoreWindowSizeChanged).Get(),
            &m_sizeChangedToken));
    }

    if (m_pControl->GetBrowserHost()->GetContextInterface()->UseWindowPosChanged())
    {
        IFC_RETURN(WindowPositionChanged_RegisterListener(this));
    }

    if (IsXamlBehaviorEnabledForCurrentSku(JupiterWindow_PluginFocusFromActivated)
        || XamlOneCoreTransforms::IsEnabled())
    {
        if (!m_activatedToken.value)
        {
            IFC_RETURN(spCoreWindow->add_Activated(
                Microsoft::WRL::Callback<wf::ITypedEventHandler<
                    wuc::CoreWindow*,
                    wuc::WindowActivatedEventArgs*>>(
                        this,
                        &CJupiterWindow::OnCoreWindowActivated).Get(),
                &m_activatedToken));
        }
    }

    return S_OK;
}

void CJupiterWindow::EnsureInputSiteAdapterForCoreWindow(_In_ ixp::IContentIsland* const coreWindowContentIsland)
{
    if (m_inputSiteAdapter != nullptr)
    {
        return;
    }

    m_inputSiteAdapter = std::make_unique<InputSiteAdapter>();
    m_inputSiteAdapter->Initialize(coreWindowContentIsland, GetCoreWindowContentRootNoRef(), this);
}

void CJupiterWindow::UninitializeInputSiteAdapterForCoreWindow()
{
    m_inputSiteAdapter = nullptr;
}

_Check_return_ HRESULT CJupiterWindow::UnregisterCoreWindowEvents()
{
    ASSERT(WindowType::CoreWindow == m_windowType);
    ASSERT(m_pCoreWindow != nullptr);

    ComPtr<wuc::ICoreWindow> spCoreWindow(m_pCoreWindow);

    if (m_visibilityChangedToken.value)
    {
        IFC_RETURN(spCoreWindow->remove_VisibilityChanged(m_visibilityChangedToken));
        m_visibilityChangedToken.value = 0;
    }

    if (m_sizeChangedToken.value)
    {
        IFC_RETURN(spCoreWindow->remove_SizeChanged(m_sizeChangedToken));
        m_sizeChangedToken.value = 0;
    }

    IFC_RETURN(WindowPositionChanged_UnregisterListener(this));

    if (m_activatedToken.value)
    {
        IFC_RETURN(spCoreWindow->remove_Activated(m_activatedToken));
        m_activatedToken.value = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::RegisterUISettingsEvents()
{
    wrl::ComPtr<wuv::IUISettings> spUISettings;

    // Not signing up for this event is ok, it just means we won't respond to changes but if the shell has not implemented this yet it's
    // benign to just move on.
    if (FAILED(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(), &spUISettings)))
    {
        return S_OK;
    }

    if (FAILED(spUISettings.As(&m_spUISettings3)))
    {
        return S_OK;
    }

    if (!m_colorValuesChangedToken.value)
    {
        wrl::ComPtr<msy::IDispatcherQueue> dispatcherQueue { m_pControl->GetXcpDispatcher()->GetDispatcherQueueNoRef() };

        const auto colorValuesChangedCallback = [dispatcherQueue](const auto&, const auto&) {

            auto colorValuesChangedCallbackOnUIThread = WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([]() {
                // Equivalent of System Xaml's OnUISettingsColorValuesChanged
                VERIFYHR(CJupiterControl::OnThemeChanged());
                IGNOREHR(CJupiterControl::NotifyImmersiveColorSetChanged());
                return S_OK;
            });

            // Don't worry about failure here.  If we're shutting down, it's not important that we process color changes.
            boolean enqueuedIgnore{};
            IGNOREHR(dispatcherQueue->TryEnqueue(colorValuesChangedCallbackOnUIThread.Get(), &enqueuedIgnore));
            return S_OK;
        };

        IFCFAILFAST(m_spUISettings3->add_ColorValuesChanged(
            WRLHelper::MakeAgileCallback<
                wf::ITypedEventHandler<wuv::UISettings*,
                IInspectable*>>(colorValuesChangedCallback).Get(),
            &m_colorValuesChangedToken));
    }

    IFCFAILFAST(spUISettings.As(&m_spUISettings2));
    if (!m_fontScaleChangedToken.value)
    {
        if (FAILED(m_spUISettings2->add_TextScaleFactorChanged(
            Microsoft::WRL::Callback<wf::ITypedEventHandler<
                wuv::UISettings*,
                IInspectable*>>(
                    this,
                    &CJupiterWindow::OnUISettingsFontScaleChanged).Get(),
            &m_fontScaleChangedToken)))
        {
            m_fontScaleChangedToken.value = 0;
            return S_OK;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::UnregisterUISettingsEvents()
{
    if (m_colorValuesChangedToken.value)
    {
        IFC_RETURN(m_spUISettings3->remove_ColorValuesChanged(m_colorValuesChangedToken));
        m_colorValuesChangedToken.value = 0;
    }

    if (m_fontScaleChangedToken.value)
    {
        IFC_RETURN(m_spUISettings2->remove_TextScaleFactorChanged(m_fontScaleChangedToken));
        m_fontScaleChangedToken.value = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::RegisterWithDragDropManager()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wadt::DragDrop::Core::ICoreDragDropManagerStatics> spDragDropManagerStatics;

    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow or application view, which is not supported in islands. Hence, prefer early return.
    if (m_pControl->GetCoreServices()->GetInitializationType() == InitializationType::IslandsOnly)
    {
        return S_OK;
    }

    // Only one handler for this event should exist
    ASSERT(m_dropTargetRequestedToken.value == 0);

    hr = ctl::GetActivationFactory(wrl_wrappers::HStringReference(
        RuntimeClass_Windows_ApplicationModel_DataTransfer_DragDrop_Core_CoreDragDropManager).Get(),
        &spDragDropManagerStatics);
    if (FAILED(hr))
    {
        if (REGDB_E_CLASSNOTREG == hr)
        {
            // If Class is not available, we just fallback on old behavior
            // (Happens currently on OneCore and Phone)
            return S_OK;
        }
        else
        {
            // All other failures are not accepted
            IFC_RETURN(hr);
        }
    }
    else
    {
        ctl::ComPtr<wadt::DragDrop::Core::ICoreDragDropManager> spDragDropManager;

        // Register current view as drop target
        IFC_RETURN(spDragDropManagerStatics->GetForCurrentView(&spDragDropManager));

        IFC_RETURN(spDragDropManager->add_TargetRequested(
            wrl::Callback<wf::ITypedEventHandler<
                wadt::DragDrop::Core::CoreDragDropManager*,
                wadt::DragDrop::Core::CoreDropOperationTargetRequestedEventArgs*>>(
                    this,
                    &CJupiterWindow::OnDropTargetRequested).Get(),
            &m_dropTargetRequestedToken));

        // If core drop target is set up successfully, use core Drag API
        SetUseCoreDragDrop(true);
    }

    return S_OK;
}

void CJupiterWindow::SetUseCoreDragDrop(bool useCoreDragDrop)
{
    if (ShouldSetUseCoreDragDrop(useCoreDragDrop))
    {
        DirectUI::DXamlCore::GetCurrent()->GetDragDrop()->SetUseCoreDragDrop(useCoreDragDrop);
    }
}

bool CJupiterWindow::ShouldSetUseCoreDragDrop(bool useCoreDragDrop)
{
    if (m_useCoreDragDropSet)
    {
        ASSERT(useCoreDragDrop == m_useCoreDragDrop);
        return false;
    }
    else
    {
        m_useCoreDragDropSet = true;
        m_useCoreDragDrop = useCoreDragDrop;
        return true;
    }
}

_Check_return_ HRESULT CJupiterWindow::SetIslandDragDropMode(bool value)
{
    SetUseCoreDragDrop(value);
    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::RegisterDropTargetRequested()
{
    return RegisterWithDragDropManager();
}

_Check_return_ HRESULT CJupiterWindow::UnregisterDropTargetRequested()
{
    if (m_dropTargetRequestedToken.value != 0)
    {
        ctl::ComPtr<wadt::DragDrop::Core::ICoreDragDropManagerStatics> spDragDropManagerStatics;
        ctl::ComPtr<wadt::DragDrop::Core::ICoreDragDropManager> spDragDropManager;
        SuspendFailFastOnStowedException suspender; // http://osgvsowi/6528338 - suspend fail fast until it is fixed

        IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
                RuntimeClass_Windows_ApplicationModel_DataTransfer_DragDrop_Core_CoreDragDropManager).Get(),
                &spDragDropManagerStatics));

        // We're seeing cases where the window is closed before CJupiterControl::Deinitialize calls here, which means
        // GetForCurrentView returns 0x80070578. No-op in that case - there's nothing to remove anymore.
        HRESULT hr = spDragDropManagerStatics->GetForCurrentView(&spDragDropManager);
        if (SUCCEEDED(hr))
        {
            IFC_RETURN(spDragDropManager->remove_TargetRequested(m_dropTargetRequestedToken));
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE))
        {
            // No-op. The window has been closed.
        }
        else
        {
            IFC_RETURN(hr);
        }

        m_dropTargetRequestedToken.value = 0;
    }

    return S_OK;
}


_Check_return_ HRESULT CJupiterWindow::UpdateWindowVisibility(_In_ wuc::ICoreWindow* pCoreWindow, bool isStartingUp)
{
    IXcpBrowserHost *pbh;
    if (pCoreWindow)
    {
        pbh = m_pControl->GetBrowserHost();
        ASSERT(pbh);
        {
            CCoreServices* pcs = pbh->GetContextInterface();
            boolean isVisible = TRUE;
            ASSERT(pcs);
            IFC_RETURN(pCoreWindow->get_Visible(&isVisible));

            // When pArgs is nullptr, it means that we are starting up. We do not want to disable render
            // during starting and hence the value of second param.
            IFC_RETURN(pcs->SetWindowVisibility(!!isVisible, isStartingUp, true /* freezeDWMSnapshotIfHidden */));

            // Now that we opt in to the new WinBlue LayoutCompleted mechanism (see SetShouldWaitForLayoutCompletion),
            // we need to call LayoutCompleted following re-layout after we become visible.
            if (isVisible)
            {
                IFC_RETURN(SetLayoutCompletedNeeded(LayoutCompletedNeededReason::WindowMadeVisible));
            }
        }
    }

    return S_OK;
}


// TODO: Once lifted IXP input is ingested, verify that this function is no longer needed and remove.
// This is tracked by Task #35745158: https://microsoft.visualstudio.com/DefaultCollection/OS/_workitems/edit/35745158
_Check_return_ HRESULT CJupiterWindow::SetCursorToCoreWindowCursor()
{
    wrl::ComPtr<wuc::ICoreCursor> pCursor;

    if (m_pCoreWindow == nullptr)
    {
        return S_OK;
    }

    IFC_RETURN(m_pCoreWindow->get_PointerCursor(&pCursor));

    IFC_RETURN(m_pCoreWindow->put_PointerCursor(pCursor.Get()));

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::Activate()
{
    switch (m_windowActivationState)
    {
        case JupiterWindowActivationState::Activatable:
            IFCEXPECT_RETURN(m_pCoreWindow);
            //There appears to be a case where the window was closed before it had a chance to show for the first time.
            //We can change this place to explicitly check for E_HANDLE and change the hr to S_FALSE.
            // So Fail Fast is avoided when the FailFastOnAnyStowedException feature is enabled via Shell Velocity settings.
            {
                HRESULT hr = m_pCoreWindow->Activate();
                if (hr == E_HANDLE)
                {
                    hr = S_FALSE;
                }
                IFC_RETURN(hr);
            }
            m_wasWindowEverActivated =  true;
            break;

        case JupiterWindowActivationState::FirstFramePending:
            // If we're waiting for the first frame and Activate() is called
            // just note that an activation was requested. We'll perform the
            // activation later when the first frame is ready.
            m_windowActivationState = JupiterWindowActivationState::ActivationRequested;
            break;

        case JupiterWindowActivationState::ActivationRequested:
            // No-op: we're already waiting to perform a previously requested
            // activation, and another activation is being requested.
            break;

        default:
            IFCEXPECT_ASSERT_RETURN(FALSE);
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::NotifyFirstFramePending()
{
    // If we're currently activatable and are notified that the first frame
    // is pending, go to a new state (FirstFramePending).

    // If we're in any other state, this notification should not change the state.

    if (JupiterWindowActivationState::Activatable == m_windowActivationState)
    {
        m_windowActivationState = JupiterWindowActivationState::FirstFramePending;
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::NotifyFirstFrameDrawn()
{
    // This function is UWP specific and has dependency on CoreWindow. Early return in Desktop/ Islands as we don't set hwnd.
    if (!m_hwnd)
    {
        return S_OK;
    }

    CFocusManager* focusManager = nullptr;
    auto pBH = m_pControl->GetBrowserHost();

    if (pBH != nullptr)
    {
        if (auto contentRoot = GetCoreWindowContentRootNoRef())
        {
            focusManager = contentRoot->GetFocusManagerNoRef();
        }
    }

    // Handle deferred focus. If our window was focused before the XAML content was available, we need
    // to take the same actions as if we were focused after the content was available. To do so
    // synthesize a WM_SETFOCUS message. If we're hosting ContentIslands in UWPs (CoreWindows), focus
    // will be on the ContentIsland, which can be retrieved via the InputSiteAdapter.
    const auto hwndInFocus = ::GetFocus();
    bool isIslandInFocus = (nullptr != m_inputSiteAdapter) ? m_inputSiteAdapter->HasFocus() : false;
    if ((hwndInFocus == m_hwnd || isIslandInFocus) && focusManager != nullptr && !focusManager->IsPluginFocused())
    {
        m_pControl->HandleWindowMessage(WM_SETFOCUS, 0, 0, GetCoreWindowContentRootNoRef());
    }

    switch (m_windowActivationState)
    {
        case JupiterWindowActivationState::Activatable:
            // No-op: we don't care about this notification in the Activatable state.
            break;

        case JupiterWindowActivationState::FirstFramePending:
            // Just go back to the Activatable state: no one requested an activation while
            // the first frame was pending.
            m_windowActivationState = JupiterWindowActivationState::Activatable;
            break;

        case JupiterWindowActivationState::ActivationRequested:
            // An activation was requested while the first frame was pending. Go back
            // to the Activatable state and perform the previously requested activation.
            m_windowActivationState = JupiterWindowActivationState::Activatable;
            IFC_RETURN(Activate());
            break;

        default:
            IFCEXPECT_ASSERT_RETURN(FALSE);
    }

    return S_OK;
}

void CJupiterWindow::OnThemeChanged()
{
    // If this event returns a failing HRESULT, then the invoker will failfast
    // the app. Given how easy it is for developers to incorrectly author their
    // theme resources (e.g. a ThemeResource key is not present in all theme
    // dictionaries) and that we have no compile-time checking for such mistakes,
    // it seems rude to bring down the app. Therefore, ignore errors during theme change
    // notification propagation.
    IGNOREHR(m_pControl->OnThemeChanged());
    IGNOREHR(UpdateFontScale());
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the stored font scale with the current value from settings.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterWindow::UpdateFontScale()
{
    float fontScale = 1.0f;

    fontScale = this->GetFontScale();
    IFC_RETURN(m_pControl->UpdateFontScale((XFLOAT)fontScale));

    return S_OK;
}

float CJupiterWindow::GetFontScale()
{
    double sf = 1.0f;

    if (m_spUISettings2)
    {
        m_spUISettings2->get_TextScaleFactor(&sf);
    }

    return (float)sf;
}

_Check_return_ HRESULT CJupiterWindow::OnCoreWindowPointerMessage(
    const UINT uMsg,
    _In_ ixp::IPointerEventArgs* args)
{
    wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFCFAILFAST(args->get_CurrentPoint(&pointerPoint));

    bool handled = false;
    IFC_RETURN(OnCoreWindowPointerMessage(uMsg, pointerPoint.Get(), args, false /* isReplayedMessage */, &handled));

    // Don't write false into the event args. This prevents overwriting the value if someone else already handled the event.
    if (handled)
    {
        IFC_RETURN(args->put_Handled(handled));
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnCoreWindowPointerMessage(
    const UINT uMsg,
    _In_ ixp::IPointerPoint* pointerPoint,
    _In_ ixp::IPointerEventArgs* pointerEventArgs,
    const bool isReplayedMessage,
    _Out_ bool* handled)
{
    *handled = false;

    UINT32 pointerId = 0;
    IFCFAILFAST(pointerPoint->get_PointerId(&pointerId));

    *handled = !!m_pControl->HandlePointerMessage(uMsg, static_cast<WPARAM>(pointerId), 0, GetCoreWindowContentRootNoRef(), isReplayedMessage, pointerPoint, pointerEventArgs);

    return S_OK;
}

_Check_return_ HRESULT CJupiterWindow::OnDirectManipulationHitTest(_In_ ixp::IPointerEventArgs* args)
{
    TraceDmPointerHitTestBegin();
    IFC_RETURN(OnCoreWindowPointerMessage(DM_POINTERHITTEST, args));
    TraceDmPointerHitTestEnd();

    return S_OK;
}

void CJupiterWindow::SetPointerCapture()
{
    IFCFAILFAST(m_inputSiteAdapter->SetPointerCapture());
}

void CJupiterWindow::ReleasePointerCapture()
{
    IFCFAILFAST(m_inputSiteAdapter->ReleasePointerCapture());
}

bool CJupiterWindow::HasPointerCapture() const
{
    return m_inputSiteAdapter->HasPointerCapture();
}

CContentRoot* CJupiterWindow::GetCoreWindowContentRootNoRef() const
{
    // Get the XamlRoot that is associated with the CoreWindow. This is safe because JupiterWindow is essentially
    // a large wrapper around CoreWindow, so we know that whenever JupiterWindow needs a content root, it's the one
    // associated with a CoreWindow
    const auto contentRootCoordinator = m_pControl->GetCoreServices()->GetContentRootCoordinator();
    return contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
}

_Check_return_ HRESULT CJupiterWindow::SetFocus()
{
    return m_inputSiteAdapter->SetFocus();
}

void CJupiterWindow::ReplayPointerUpdate()
{
    bool pointerUpdateReplayed = false;

    // Replay main window pointer messages.
    if (m_inputSiteAdapter)
    {
        if (m_inputSiteAdapter->ReplayPointerUpdate())
        {
            pointerUpdateReplayed = true;
        }
    }

    // Replay any windowed popup pointer messages.
    if (CPopupRoot* popupRoot = m_pControl->GetCoreServices()->GetMainPopupRoot())
    {
        bool popupPointerUpdateReplayed = popupRoot->ReplayPointerUpdate();
        if (popupPointerUpdateReplayed)
        {
            // Only one window should have valid pointer messages to replay.
            ASSERT(!pointerUpdateReplayed);
        }
    }
}
