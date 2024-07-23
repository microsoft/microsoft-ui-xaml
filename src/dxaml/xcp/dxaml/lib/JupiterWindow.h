// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.ui.viewmanagement.h>
#include <fwd/windows.ui.core.h>
#include <fwd/microsoft.ui.xaml.h>
#include <microsoft.ui.input.experimental.h>
#include <microsoft.ui.input.inputkeyboardsource.interop.h>
#include <microsoft.ui.input.inputpretranslatesource.interop.h>
#include <FrameworkUdk/CoreWindowIntegrationInterface.h>

#include "WeakReferenceSourceNoThreadId.h"

namespace WindowType
{
    enum Enum
    {
        None,
        CoreWindow,
        DesktopWindow,
    };
}

namespace JupiterWindowActivationState
{
    enum Enum
    {
        Activatable,
        FirstFramePending,
        ActivationRequested
    };
}

class CJupiterControl;
class FocusObserver;
class CContentRoot;
class InputSiteAdapter;

class CJupiterWindow : public ::ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener,
                       public ctl::WeakReferenceSourceNoThreadId
{
    BEGIN_INTERFACE_MAP(CJupiterWindow, WeakReferenceSourceNoThreadId)
        INTERFACE_ENTRY(CJupiterWindow, ::ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener)
    END_INTERFACE_MAP(CJupiterWindow, WeakReferenceSourceNoThreadId)

public:
    static _Check_return_ HRESULT ConfigureJupiterWindow(
        _In_opt_ wuc::ICoreWindow* pCoreWindow,
        _In_ CJupiterControl* pControl,
        _Outptr_ CJupiterWindow** ppWindow);

    void SetControl(_In_ CJupiterControl* pControl);

    _Check_return_ HRESULT ShowWindow();
    HWND GetWindowHandle() const { return m_hwnd; }

    // returns non-NULL when GetType() is WindowType::CoreWindow
    _Check_return_ HRESULT GetCoreWindow(_Outptr_result_maybenull_ wuc::ICoreWindow** ppWindow);
    wuc::ICoreWindow* GetCoreWindowNoRef();

    // returns E_UNEXPECTED if GetType() is not WindowType::CoreWindow
    _Check_return_ HRESULT RunCoreWindowMessageLoop();

    WindowType::Enum GetType()
    {
        return m_windowType;
    }

    _Check_return_ HRESULT Activate();
    _Check_return_ HRESULT NotifyFirstFramePending();
    _Check_return_ HRESULT NotifyFirstFrameDrawn();

    ~CJupiterWindow() override;

    _Check_return_ HRESULT SetCursorToCoreWindowCursor();

    bool IsWindowDestroyed()
    {
        return m_fWindowDestroyed;
    }

    XSIZE GetJupiterWindowPhysicalSize(HWND hwndContext) const;

    _Check_return_ HRESULT UpdateFontScale();

    _Check_return_ HRESULT Initialize(_In_ HWND hwnd, WindowType::Enum windowType);

    CJupiterWindow();

    wrl::ComPtr<ixp::IInputPointerSource> GetInputSiteAdapterInputPointerSource();
    wrl::ComPtr<ixp::IPointerPoint> GetInputSiteAdapterPointerPoint();

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return AddRefImpl();
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        return ReleaseImpl();
    }

    void OnThemeChanged();

    // Core window input site adapter functions
    void EnsureInputSiteAdapterForCoreWindow(_In_ ixp::IContentIsland* const coreWindowContentIsland);
    void UninitializeInputSiteAdapterForCoreWindow();

    // Input from Islands
    _Check_return_ HRESULT OnIslandGotFocus(_In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT OnIslandLostFocus(_In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT OnIslandCharacterReceived(_In_ ixp::ICharacterReceivedEventArgs* e, _In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT OnIslandKeyDown(_In_ ixp::IKeyEventArgs* e, _In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT OnIslandKeyUp(_In_ ixp::IKeyEventArgs* e, _In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT OnIslandSysKeyDown(_In_ ixp::IKeyEventArgs* e, _In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT OnIslandSysKeyUp(_In_ ixp::IKeyEventArgs* e, _In_opt_ CContentRoot* contentRoot);

    _Check_return_ HRESULT OnIslandDirectManipulationHitTest(_In_opt_ CContentRoot* contentRoot, _In_ ixp::IPointerEventArgs* args);
    _Check_return_ HRESULT OnIslandDirectManipulationHitTest(_In_opt_ CContentRoot* contentRoot, _In_ ixp::IPointerPoint* pointerPoint, _Out_ bool* handled);

    _Check_return_ HRESULT OnIslandPointerMessage(
        const UINT msg,
        _In_opt_ CContentRoot* contentRoot,
        _In_ ixp::IPointerPoint* pointerPoint,
        _In_ ixp::IPointerEventArgs* pointerEventArgs,
        const bool isReplayedMessage,
        _Out_ bool* handled);

    _Check_return_ HRESULT OnIslandNonClientPointerMessage(
        const UINT msg,
        _In_opt_ CContentRoot* contentRoot,
        _In_ ixp::IPointerPoint* pointerPoint,
        const bool isReplayedMessage);

    _Check_return_ HRESULT OnIslandMessage(
        _In_ UINT uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _In_opt_ CContentRoot* contentRoot);

    _Check_return_ HRESULT SetIslandDragDropMode(bool value);

    void SetPointerCapture();
    void ReleasePointerCapture();
    bool HasPointerCapture() const;

    _Check_return_ HRESULT OnCoreWindowPointerMessage(const UINT uMsg, _In_ ixp::IPointerEventArgs* args);
    _Check_return_ HRESULT OnCoreWindowPointerMessage(const UINT uMsg, _In_ ixp::IPointerPoint* pointerPoint, _In_ ixp::IPointerEventArgs* pointerEventArgs, const bool isReplayedMessage, _Out_ bool* handled);

    _Check_return_ HRESULT ProcessCharEvents(
                            _Inout_ ixp::ICharacterReceivedEventArgs * pArgs,
                            _In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT ProcessKeyEvents(
                            _Inout_ ixp::IKeyEventArgs * pArgs,
                            _In_ UINT32 uMsg,
                            _In_opt_ CContentRoot* contentRoot,
                            _In_ bool processContextMenu = true);
    bool ProcessFocusEvents(
                            _In_ UINT32 uMsg,
                            _In_opt_ CContentRoot* contentRoot);
    _Check_return_ HRESULT PreTranslateMessage(
        _In_opt_ CContentRoot* contentRoot,
        _In_ mui::IInputPreTranslateKeyboardSourceInterop* source,
        _In_ mui::IInputKeyboardSourceInterop* keyboardSource,
        _In_ const MSG* msg,
        _In_ UINT keyboardModifiers,
        _In_ bool focusPass,
        _Inout_ bool* handled);
    _Check_return_ HRESULT SetFocus();

    bool ProcessContextMenu(
        _In_ UINT32 uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam,
        _In_ bool wasKeyEventHandled,
        _In_opt_ CContentRoot* contentRoot);

    void ResetContextMenuState();

    void ReplayPointerUpdate();

protected:
    _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
    {
        if (InlineIsEqualGUID(iid, __uuidof(::ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener)))
        {
            *ppObject = static_cast<::ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener*>(this);
        }
        else
        {
            return ctl::WeakReferenceSourceNoThreadId::QueryInterfaceImpl(iid, ppObject);
        }

        AddRefOuter();
        return S_OK;
    }

private:
    static LRESULT CALLBACK StaticCoreWindowSubclassProc(_In_ HWND hwnd, UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam); // [shonji] to be deprecated
    static _Check_return_ HRESULT RegisterWindowClass();
    static _Check_return_ HRESULT Create(_In_ HWND hwnd, WindowType::Enum windowType, _In_ CJupiterControl* pControl, _Outptr_ CJupiterWindow** ppWindow);

    LRESULT CALLBACK CoreWindowSubclassProc(_In_ HWND hwnd, UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    // Event handler for ::Windows::Graphics::Display events
    HRESULT OnLogicalDpiChangeEvent(_In_ IInspectable *sender);

    _Check_return_ HRESULT RegisterCoreWindowEvents();
    _Check_return_ HRESULT UnregisterCoreWindowEvents();
    _Check_return_ HRESULT RegisterUISettingsEvents();
    _Check_return_ HRESULT UnregisterUISettingsEvents();

    _Check_return_ HRESULT OnCoreWindowVisibilityChanged(_In_  wuc::ICoreWindow * pSender, _Inout_ wuc::IVisibilityChangedEventArgs * pArgs);
    _Check_return_ HRESULT OnCoreWindowSizeChanged(_In_  wuc::ICoreWindow * pSender, _Inout_ wuc::IWindowSizeChangedEventArgs * pArgs);
    STDMETHOD (OnCoreWindowPositionChanged)() override; //ICoreWindowPositionChangedListener
    _Check_return_ HRESULT OnCoreWindowCharacterReceived(_In_  wuc::ICoreWindow * pSender, _Inout_ wuc::ICharacterReceivedEventArgs * pArgs);
    void OnCoreWindowDestroying();
    _Check_return_ HRESULT OnCoreWindowActivated(_In_  wuc::ICoreWindow * pSender, _Inout_ wuc::IWindowActivatedEventArgs * pArgs);

    _Check_return_ HRESULT OnDirectManipulationHitTest(_In_ ixp::IPointerEventArgs* args);

    _Check_return_ HRESULT OnUISettingsFontScaleChanged(_In_ wuv::IUISettings * pSender, _Inout_ IInspectable* pArgs);

    _Check_return_ HRESULT OnDropTargetRequested(_In_ wadt::DragDrop::Core::ICoreDragDropManager* pSender, _In_ wadt::DragDrop::Core::ICoreDropOperationTargetRequestedEventArgs* pArgs);
    _Check_return_ HRESULT RegisterDropTargetRequested();
    _Check_return_ HRESULT RegisterWithDragDropManager();
    _Check_return_ HRESULT UnregisterDropTargetRequested();
    bool ShouldSetUseCoreDragDrop(bool desiredValue);
    void SetUseCoreDragDrop(bool useCoreDragDrop);
    _Check_return_ HRESULT UpdateComponentDisplayInfo();

    _Check_return_ HRESULT OnSizeChanged();
    _Check_return_ HRESULT UpdateWindowVisibility(_In_ wuc::ICoreWindow* pCoreWindow, bool isStartingUp);
    _Check_return_ HRESULT SetInitialCursor();

    _Check_return_ HRESULT SetLayoutCompletedNeeded(const LayoutCompletedNeededReason reason);

    _Ret_maybenull_  _Check_return_ FocusObserver* GetFocusObserverNoRef() const;

    CContentRoot* GetCoreWindowContentRootNoRef() const;

    _Check_return_ HRESULT ProcessCopyAndPaste(_In_ wsy::VirtualKey virtualKey);
    _Check_return_ HRESULT PackIntoWin32StyleCharArgs(
                            _In_ ixp::ICharacterReceivedEventArgs* pArgs,
                            _In_ UINT32 uMsg,
                            _Out_ UINT32* wParam,
                            _Out_ UINT32* lParam);

    _Check_return_ HRESULT SetCoreWindow(_In_opt_ wuc::ICoreWindow* coreWindow);

    float GetFontScale();

    _Check_return_ HRESULT AcceleratorKeyActivated(
        _In_ CContentRoot* contentRoot,
        _In_ const MSG* msg,
        _In_ UINT keyboardModifiers,
        _In_ wsy::VirtualKey virtualKey,
        _Inout_ bool* handled);

    _Check_return_ HRESULT OnContentAutomationProviderRequested(
        _In_ ixp::IContentIsland* content,
        _In_ ixp::IContentIslandAutomationProviderRequestedEventArgs* e);

    HWND                m_hwnd;
    WindowType::Enum    m_windowType;
    CJupiterControl*    m_pControl;
    Microsoft::WRL::ComPtr<wuv::IUISettings3> m_spUISettings3;
    Microsoft::WRL::ComPtr<wuv::IUISettings2> m_spUISettings2;
    bool               m_fWindowDestroyed;
    bool               m_fHandledAppsKeyUpForContextMenu;
    bool               m_fHandledShiftF10KeyDownForContextMenu;

    UINT                m_inputPanelMessage;
    bool                m_registerInputPaneHandler;

    // These fields are used when m_windowType is CoreWindow
    wuc::ICoreWindow* m_pCoreWindow;
    WNDPROC                         m_subclassedWndProc;
    EventRegistrationToken          m_visibilityChangedToken;
    EventRegistrationToken          m_sizeChangedToken;
    EventRegistrationToken          m_dropTargetRequestedToken;
    EventRegistrationToken          m_activatedToken;
    EventRegistrationToken          m_colorValuesChangedToken;
    EventRegistrationToken          m_fontScaleChangedToken;

    bool m_wasWindowEverActivated;  // Used for debugging/post-mortem purposes

    JupiterWindowActivationState::Enum     m_windowActivationState;

    bool m_useCoreDragDropSet = false;
    bool m_useCoreDragDrop = false;

    std::unique_ptr<InputSiteAdapter> m_inputSiteAdapter{nullptr};
    bool m_inputSiteAdapterHwndHandlesWmGetObject{false};
    bool m_legacyCoreWindowUiaProviderSet{false};

    wrl::ComPtr<ixp::IContentIsland> m_contentIsland{nullptr};
    EventRegistrationToken m_automationProviderRequestedToken = {};

#pragma warning(push)
#pragma warning(disable:4996) // IApplicationViewStatics is marked as [[deprecated]]
    static Microsoft::WRL::ComPtr<wuv::IApplicationViewStatics> s_spApplicationViewStatics;
#pragma warning(pop)
};
