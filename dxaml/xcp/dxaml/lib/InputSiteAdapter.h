// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.ui.viewmanagement.h>
#include <fwd/windows.ui.core.h>
#include <fwd/microsoft.ui.xaml.h>
#include <microsoft.ui.input.h>
#include <microsoft.ui.input.inputkeyboardsource.interop.h>
#include <microsoft.ui.input.inputpretranslatesource.interop.h>
#include "NamespaceAliases.h"

class CJupiterWindow;
class CContentRoot;

class InputSiteAdapter
{
public:
    InputSiteAdapter();
    virtual ~InputSiteAdapter();

    void Initialize(_In_ ixp::IContentIsland* contentIsland, _In_ CContentRoot* contentRoot, _In_ CJupiterWindow* jupiterWindow);

    _Check_return_ HRESULT SetFocus();
    _Check_return_ HRESULT SetPointerCapture();
    _Check_return_ HRESULT ReleasePointerCapture();
    bool HasPointerCapture();

    wrl::ComPtr<ixp::IPointerPoint> GetPreviousPointerPoint();
    wrl::ComPtr<ixp::IInputPointerSource> GetInputPointerSource();

    virtual bool ReplayPointerUpdate();
    void ClearLastPointerPointForReplay();

    // PreTranslate and accelerator support, called back from PreTranslateHandler template class
    HRESULT PreTranslateMessage(
        mui::IInputPreTranslateKeyboardSourceInterop* source,
        const MSG* msg,
        UINT keyboardModifiers,
        bool focusPass,
        bool* handled);

protected:
    virtual _Check_return_ HRESULT OnDirectManipulationHitTest(_In_ ixp::IPointerEventArgs* args);

    virtual _Check_return_ HRESULT OnPointerMessage(const UINT uMsg, _In_ ixp::IPointerEventArgs* args);

    void UpdateLastPointerPointForReplay(const UINT uMsg, _In_ ixp::IPointerPoint* pointerPoint, _In_ ixp::IPointerEventArgs* pointerEventArgs);

    CJupiterWindow* m_jupiterWindow {nullptr};
    xref::weakref_ptr<CContentRoot> m_contentRootWeak;

    // Returns a strong ref because the reutrn value is often passed down a stack where a sync callback to the app
    // occurs. It's important to hold a strong reference in this case because the app might release all other references
    // to the CContentRoot in that callback.
    // Note that it's safe here to pass Foo(GetContentRoot()); even if Foo takes a raw CContentRoot*, because in C++
    // temporaries aren't destroyed until the end of the statement.
    xref_ptr<CContentRoot> GetContentRoot() const;

    wrl::ComPtr<ixp::IPointerPoint> m_previousPointerPoint;
    wrl::ComPtr<ixp::IPointerEventArgs> m_previousPointerEventArgs;
    wrl::ComPtr<ixp::IInputPointerSource2> m_inputPointerSource2;

private:
    void InitInputObjects(_In_ ixp::IContentIsland* const contentIsland);
    void SubscribeToInputKeyboardSourceEvents();
    void SubscribeToInputPointerSourceEvents();
    void UnsubscribeToInputEvents();

    _Check_return_ HRESULT OnCharacterReceived(_Inout_ ixp::ICharacterReceivedEventArgs *args);
    _Check_return_ HRESULT OnKeyDown(_In_ ixp::IKeyEventArgs *pArgs);
    _Check_return_ HRESULT OnKeyUp(_In_ ixp::IKeyEventArgs *pArgs);
    _Check_return_ HRESULT OnSysKeyUp(_Inout_ ixp::IKeyEventArgs * pArgs);
    _Check_return_ HRESULT OnSysKeyDown(_Inout_ ixp::IKeyEventArgs * pArgs);
    _Check_return_ HRESULT OnContextMenuKey(_Inout_ ixp::IContextMenuKeyEventArgs* pArgs);
    _Check_return_ HRESULT OnGotFocus();
    _Check_return_ HRESULT OnLostFocus();

    _Check_return_ HRESULT OnPointerCaptureLost(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerEntered(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerExited(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerMoved(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerPressed(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerReleased(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerWheelChanged(_In_ ixp::IPointerEventArgs*e);
    _Check_return_ HRESULT OnPointerRoutedAway(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerRoutedReleased(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnPointerRoutedTo(_In_ ixp::IPointerEventArgs* e);
    _Check_return_ HRESULT OnTouchHitTesting(_In_ ixp::ITouchHitTestingEventArgs* args);

    CUIElement* GetPublicRootVisual();

    bool m_hasCapture = false;

    wrl::ComPtr<ixp::IInputFocusController> m_inputFocusController;
    wrl::ComPtr<ixp::IInputKeyboardSource2> m_inputKeyboardSource2;
    wrl::ComPtr<ixp::IInputPointerSource> m_inputPointerSource;
    wrl::ComPtr<ixp::IInputActivationListener> m_inputActivationListener;
    wrl::ComPtr<ixp::IInputPreTranslateKeyboardSourceInterop> m_inputPreTranslateKeyboardSourceInterop;
    wrl::ComPtr<mui::IInputPreTranslateKeyboardSourceHandler> m_preTranslateHandler;

    EventRegistrationToken m_characterReceivedToken{};
    EventRegistrationToken m_keyDownToken{};
    EventRegistrationToken m_keyUpToken{};
    EventRegistrationToken m_sysKeyDownToken{};
    EventRegistrationToken m_sysKeyUpToken{};
    EventRegistrationToken m_contextMenuToken{};
    EventRegistrationToken m_gotFocusToken{};
    EventRegistrationToken m_lostFocusToken{};
    EventRegistrationToken m_pointerCaptureLostToken{};
    EventRegistrationToken m_pointerEnteredToken{};
    EventRegistrationToken m_pointerExitedToken{};
    EventRegistrationToken m_pointerMovedToken{};
    EventRegistrationToken m_pointerPressedToken{};
    EventRegistrationToken m_pointerReleasedToken{};
    EventRegistrationToken m_pointerWheelChangedToken{};
    EventRegistrationToken m_pointerRoutedAwayToken{};
    EventRegistrationToken m_pointerRoutedReleasedToken{};
    EventRegistrationToken m_pointerRoutedToToken{};
    EventRegistrationToken m_directManipulationHitTestToken{};
    EventRegistrationToken m_touchHitTestRequestedToken{};
    EventRegistrationToken m_activationChangedToken{};
};
