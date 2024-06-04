// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "InputManager.h"
#include "ContentRoot.h"

#include "KeyboardEventArgs.h"

#include "corep.h"

#include "InputPaneHandler.h"

#include "KeyboardUtility.h"
#include "RemapVirtualKey.h"

#include "FocusMovement.h"

#include "CaretBrowsingGlobal.h"
#include "FxCallbacks.h"

#include "RootVisual.h"
#include "CharacterReceivedEventArgs.h"
#include "ContextRequestedEventArgs.h"

#include "KeyboardAcceleratorUtility.h"
#include "FrameworkInputViewHandler.h"

#include "MUX-ETWEvents.h"

#include "InputPaneHandler.h"

using namespace ContentRootInput;

CInputManager::CInputManager(_In_ CCoreServices& coreServices, _In_ CContentRoot& contentRoot)
    : m_coreServices(coreServices)
    , m_contentRoot(contentRoot)
    , m_keyboardInputProcessor(*this)
    , m_inputPaneProcessor(*this)
    , m_inputDeviceCache(*this)
    , m_activationManager(*this)
    , m_pointerInputProcessor(*this)
    , m_contextMenuProcessor(contentRoot)
    , m_dragDropProcessor(contentRoot)
{
    m_inputDeviceCache.Init();
}

/* A focus sound should only be requested during processing of a user initiated interaction
   like a key press or mouse/touch interaction */
bool CInputManager::ShouldRequestFocusSound() const
{
    return m_keyboardInputProcessor.ShouldRequestFocusSound();
}

void CInputManager::CreateInputPaneHandler()
{
    m_inputPaneProcessor.CreateInputPaneHandler();
}

_Check_return_ HRESULT CInputManager::RegisterInputPaneHandler(_In_opt_ XHANDLE hCoreWindow)
{
    IFC_RETURN(m_inputPaneProcessor.RegisterInputPaneHandler(hCoreWindow));
    return S_OK;
}

void CInputManager::DestroyInputPaneHandler()
{
    m_inputPaneProcessor.DestroyInputPaneHandler();
}

IXcpInputPaneHandler* CInputManager::GetInputPaneHandler() const
{
    return m_inputPaneProcessor.GetInputPaneHandler();
}

IPALInputPaneInteraction* CInputManager::GetInputPaneInteraction() const
{
    return m_inputPaneProcessor.GetInputPaneInteraction();
}

DirectUI::InputPaneState CInputManager::GetInputPaneState() const
{
    return m_inputPaneProcessor.GetInputPaneState();
}

_Check_return_ HRESULT CInputManager::GetInputPaneBounds(_Out_ XRECTF* pInputPaneBounds) const
{
    IFC_RETURN(m_inputPaneProcessor.GetInputPaneBounds(pInputPaneBounds));
    return S_OK;
}

void CInputManager::AdjustBringIntoViewRecHeight(_In_ float topGlobal, _In_ float bottomGlobal, _Inout_ float &height)
{
    m_inputPaneProcessor.AdjustBringIntoViewRecHeight(topGlobal, bottomGlobal, height);
}

bool CInputManager::IsInputPaneShowing() const
{
    return m_inputPaneProcessor.IsInputPaneShowing();
}

DirectUI::InputDeviceType CInputManager::GetLastInputDeviceType() const
{
    return m_inputDeviceCache.GetLastInputDeviceType();
}

bool CInputManager::IsSipOpen() const
{
    return m_inputPaneProcessor.IsSipOpen();
}

DirectUI::FocusInputDeviceKind CInputManager::GetLastFocusInputDeviceKind() const
{
    return m_inputDeviceCache.GetLastFocusInputDeviceKind();
}

void CInputManager::OnSetFocusFromUIA()
{
    m_inputDeviceCache.OnSetFocusFromUIA();
}

bool CInputManager::GetWasUIAFocusSetSinceLastInput() const
{
    return m_inputDeviceCache.GetWasUIAFocusSetSinceLastInput();
}

void CInputManager::SetLastInputDeviceType(DirectUI::InputDeviceType deviceType, bool keepUIAFocusState)
{
    return m_inputDeviceCache.SetLastInputDeviceType(deviceType, keepUIAFocusState);
}

bool CInputManager::LastInputWasNonFocusNavigationKeyFromSIP() const
{
    return m_inputDeviceCache.LastInputWasNonFocusNavigationKeyFromSIP();
}

void CInputManager::SetPrimaryPointerLastPositionOverride(XPOINTF value)
{
    m_pointerInputProcessor.SetPrimaryPointerLastPositionOverride(value);
}

void CInputManager::ClearPrimaryPointerLastPositionOverride()
{
    m_pointerInputProcessor.ClearPrimaryPointerLastPositionOverride();
}

_Check_return_ HRESULT CInputManager::TryGetPrimaryPointerLastPosition(_Out_ XPOINTF *pLastPosition, _Out_ bool *pSucceeded)
{
    IFC_RETURN(m_pointerInputProcessor.TryGetPrimaryPointerLastPosition(pLastPosition, pSucceeded));
    return S_OK;
}

_Check_return_ HRESULT CInputManager::ProcessKeyboardInput(
    _In_ const wsy::VirtualKey virtualKey,
    _In_ const PhysicalKeyStatus& keyStatus,
    _In_ const MessageMap msgId,
    _In_opt_ const wrl_wrappers::HString* const deviceId,
    _In_ const bool isSecondaryMessage,
    _In_ const XHANDLE platformPacket,
    _Out_ bool* handled)
{
    IFC_RETURN(m_keyboardInputProcessor.ProcessKeyboardInput(virtualKey, keyStatus, msgId, deviceId, isSecondaryMessage, platformPacket, handled));
    return S_OK;
}

_Check_return_ HRESULT CInputManager::RaiseContextRequestedEvent(
    _In_ CDependencyObject* pSource,
    _In_ wf::Point point,
    _In_ bool isTouchInput)
{
    IFC_RETURN(m_contextMenuProcessor.RaiseContextRequestedEvent(pSource, point, isTouchInput));
    return S_OK;
}

_Check_return_ HRESULT CInputManager::ProcessKeyEvent(
    _In_ mui::IKeyEventArgs* args,
    _In_ UINT32 msg,
    _In_ const XHANDLE platformPacket,
    _Out_ bool* handled)
{
    IFC_RETURN(m_keyboardInputProcessor.ProcessKeyEvent(args, msg, platformPacket, handled));
    return S_OK;
}

_Check_return_ HRESULT CInputManager::RaiseRightTappedEventFromContextMenu(_Out_ bool* pHandled)
{
    IFC_RETURN(m_pointerInputProcessor.RaiseRightTappedEventFromContextMenu(pHandled));
    return S_OK;
}

_Check_return_ HRESULT CInputManager::ProcessWindowActivation(
    _In_ bool shiftPressed,
    _In_ bool fWndActive)
{
    IFC_RETURN(m_activationManager.ProcessWindowActivation(shiftPressed, fWndActive));
    return S_OK;
}

_Check_return_ HRESULT CInputManager::ProcessFocusInput(
    _In_ InputMessage *pMsg,
    _Out_ XINT32 *handled)
{
    IFC_RETURN(m_activationManager.ProcessFocusInput(pMsg, handled));
    return S_OK;
}

_Check_return_ HRESULT CInputManager::RegisterFrameworkInputView()
{
    // If we're in the context of XAML islands, then we don't want to use FrameworkInputView -
    // that requires a UIContext instance, which is not supported in WinUI 3.
    if (m_coreServices.GetInitializationType() != InitializationType::IslandsOnly)
    {
        IFC_RETURN(m_inputPaneProcessor.RegisterFrameworkInputView());
    }

    return S_OK;
}

_Check_return_ HRESULT CInputManager::NotifyFocusChanged(_In_opt_ CDependencyObject* pFocusedElement, _In_ bool bringIntoView, _In_ bool animateIfBringIntoView)
{
    IFC_RETURN(m_inputPaneProcessor.NotifyFocusChanged(pFocusedElement, bringIntoView, animateIfBringIntoView));
    return S_OK;
}

void CInputManager::SetShouldAllRequestFocusSound(_In_ bool value)
{
    m_keyboardInputProcessor.SetShouldAllRequestFocusSound(value);
}

void CInputManager::SetKeyDownHandled(_In_ bool value)
{
    m_keyboardInputProcessor.SetKeyDownHandled(value);
}

void CInputManager::SetBarrelButtonPressed(_In_ bool value)
{
    m_pointerInputProcessor.SetBarrelButtonPressed(value);
}

InputMessage* CInputManager::GetCurrentMsgForDirectManipulationProcessing() const
{
    return m_pointerInputProcessor.GetCurrentMsgForDirectManipulationProcessing();
}

void CInputManager::SetCurrentMsgForDirectManipulationProcessing(_In_opt_ InputMessage* inputMessage)
{
    m_pointerInputProcessor.SetCurrentMsgForDirectManipulationProcessing(inputMessage);
}

void CInputManager::SetNoCandidateDirectionPerTick(_In_ DirectUI::FocusNavigationDirection direction)
{
    m_keyboardInputProcessor.SetCandidateDirectionPerTick(direction);
}

bool CInputManager::GetIsContextMenuOnHolding() const
{
    return m_contextMenuProcessor.IsContextMenuOnHolding();
}

void CInputManager::SetIsContextMenuOnHolding(_In_ bool value)
{
    return m_contextMenuProcessor.SetIsContextMenuOnHolding(value);
}

int CInputManager::GetKeyDownCountBeforeSubmitFrame() const
{
    return m_keyboardInputProcessor.m_cKeyDownCountBeforeSubmitFrame;
}

void CInputManager::SetKeyDownCountBeforeSubmitFrame(_In_ int value)
{
    m_keyboardInputProcessor.m_cKeyDownCountBeforeSubmitFrame = value;
}
