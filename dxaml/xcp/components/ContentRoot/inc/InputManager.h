// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <paltypes.h>

#include "KeyboardInputProcessor.h"
#include "PointerInputProcessor.h"
#include "InputPaneProcessor.h"
#include "InputDeviceCache.h"
#include "ActivationManager.h"
#include "ContextMenuProcessor.h"
#include "DragDropProcessor.h"

#include "XcpInputPaneHandler.h"
#include "enumdefs.g.h"

class CCoreServices;
class CContentRoot;
class CKeyEventArgs;

struct IPALInputPaneInteraction;

class CDependencyObject;
class CFrameworkInputViewHandler;

class CInputManager
{
public:
    CInputManager(_In_ CCoreServices& coreServices, _In_ CContentRoot& contentRoot);

    ContentRootInput::PointerInputProcessor& GetPointerInputProcessor() { return m_pointerInputProcessor; }
    ContentRootInput::ContextMenuProcessor& GetContextMenuProcessor() { return m_contextMenuProcessor; }
    ContentRootInput::DragDropProcessor& GetDragDropProcessor() { return m_dragDropProcessor; }

    _Check_return_ HRESULT ProcessKeyboardInput(
        _In_ const wsy::VirtualKey virtualKey,
        _In_ const PhysicalKeyStatus& keyStatus,
        _In_ const MessageMap msgId,
        _In_opt_ const wrl_wrappers::HString* const deviceId,
        _In_ const bool isSecondaryMessage,
        _In_ const XHANDLE platformPacket,
        _Out_ bool* handled);

    _Check_return_ HRESULT ProcessKeyEvent(
        _In_ mui::IKeyEventArgs* args,
        _In_ UINT32 msg,
        _In_ const XHANDLE platformPacket,
        _Out_ bool* handled);

    _Check_return_ HRESULT RaiseContextRequestedEvent(
        _In_ CDependencyObject* pSource,
        _In_ wf::Point point,
        _In_ bool isTouchInput);

    _Check_return_ HRESULT RaiseRightTappedEventFromContextMenu(_Out_ bool* pHandled);

    _Check_return_ HRESULT ProcessWindowActivation(
            _In_ bool shiftPressed,
            _In_ bool fWndActive);

    _Check_return_ HRESULT ProcessFocusInput(
        _In_ InputMessage *pMsg,
        _Out_ XINT32 *handled);

    bool ShouldRequestFocusSound() const;

    bool IsSipOpen() const;
    bool IsInputPaneShowing() const;

    DirectUI::FocusInputDeviceKind GetLastFocusInputDeviceKind() const;
    DirectUI::InputDeviceType GetLastInputDeviceType() const;
    void SetLastInputDeviceType(DirectUI::InputDeviceType deviceType, bool keepUIAFocusState = false);

    void CreateInputPaneHandler();
    _Check_return_ HRESULT RegisterInputPaneHandler(_In_opt_ XHANDLE hCoreWindow);
    void DestroyInputPaneHandler();

    IXcpInputPaneHandler* GetInputPaneHandler() const;
    IPALInputPaneInteraction* GetInputPaneInteraction() const;
    DirectUI::InputPaneState GetInputPaneState() const;

    _Check_return_ HRESULT GetInputPaneBounds(_Out_ XRECTF* pInputPaneBounds) const;
    void AdjustBringIntoViewRecHeight(_In_ float topGlobal, _In_ float bottomGlobal, _Inout_ float &height);

    void OnSetFocusFromUIA();
    bool GetWasUIAFocusSetSinceLastInput() const;

    bool LastInputWasNonFocusNavigationKeyFromSIP() const;

    _Check_return_ HRESULT TryGetPrimaryPointerLastPosition(_Out_ XPOINTF *pLastPosition, _Out_ bool *pSucceeded);
    void SetPrimaryPointerLastPositionOverride(XPOINTF value);
    void ClearPrimaryPointerLastPositionOverride();

    _Check_return_ HRESULT RegisterFrameworkInputView();

    _Check_return_ HRESULT NotifyFocusChanged(_In_opt_ CDependencyObject* pFocusedElement, _In_ bool bringIntoView, _In_ bool animateIfBringIntoView = false);

    void SetShouldAllRequestFocusSound(_In_ bool value);
    void SetKeyDownHandled(_In_ bool value);
    void SetBarrelButtonPressed(_In_ bool value);
    InputMessage* GetCurrentMsgForDirectManipulationProcessing() const;
    void SetCurrentMsgForDirectManipulationProcessing(_In_opt_ InputMessage* inputMessage);
    void SetNoCandidateDirectionPerTick(_In_ DirectUI::FocusNavigationDirection direction);
    bool GetIsContextMenuOnHolding() const;
    void SetIsContextMenuOnHolding(_In_ bool value);

    int GetKeyDownCountBeforeSubmitFrame() const;
    void SetKeyDownCountBeforeSubmitFrame(_In_ int value);

public:
    CCoreServices& m_coreServices;
    xref_ptr<CContentRoot> GetContentRoot() const
    {
        return xref_ptr<CContentRoot>(&m_contentRoot);
    }
    ContentRootInput::KeyboardInputProcessor m_keyboardInputProcessor;
    ContentRootInput::InputPaneProcessor m_inputPaneProcessor;
    ContentRootInput::InputDeviceCache m_inputDeviceCache;
    ContentRootInput::ActivationManager m_activationManager;
    ContentRootInput::PointerInputProcessor m_pointerInputProcessor;
    ContentRootInput::ContextMenuProcessor m_contextMenuProcessor;
    ContentRootInput::DragDropProcessor m_dragDropProcessor;

private:
    CContentRoot& m_contentRoot;
};
