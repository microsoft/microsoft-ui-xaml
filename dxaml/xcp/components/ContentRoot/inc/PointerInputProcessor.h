// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "paltypes.h"
#include "Events.h"
#include "XcpList.h"

class CInputManager;

class CDependencyObject;
struct InputMessage;
struct PointerInfo;
class CPointerEventArgs;
struct REQUEST;
class CPointer;

namespace ContentRootInput
{
    class PointerInputProcessor
    {
    public:
        PointerInputProcessor(_In_ CInputManager& inputManager);

        bool IsBarrelButtonPressed() const;
        void SetBarrelButtonPressed(_In_ bool value);

        bool HasPrimaryPointerLastPositionOverride() const;
        _Check_return_ HRESULT TryGetPrimaryPointerLastPosition(_Out_ XPOINTF *pLastPosition, _Out_ bool *pSucceeded);
        void SetPrimaryPointerLastPositionOverride(XPOINTF value);
        void ClearPrimaryPointerLastPositionOverride();

        _Check_return_ HRESULT RaiseRightTappedEventFromContextMenu(_Out_ bool* handled);

        _Check_return_ HRESULT ProcessPointerInput(
            _In_ InputMessage *pMsg,
            _Out_ XINT32 *handled);

        InputMessage* GetCurrentMsgForDirectManipulationProcessing() const;
        void SetCurrentMsgForDirectManipulationProcessing(_In_opt_ InputMessage* inputMessage);

        bool IsProcessingPointerInput() const { return m_messageBeingProcessed != nullptr; }

        XINT32 GetPrimaryPointerId() const { return m_iPrimaryPointerId; }
        void SetPrimaryPointerId(_In_ XINT32 value) { m_iPrimaryPointerId = value; }

        xref_ptr<CPointerEventArgs>& GetPendingPointerEventArgs() { return m_pPendingPointerEventArgs; }
        void ResetPendingPointerEventArgs();

        static _Check_return_ HRESULT SetPointerKeyModifiers(_In_ XUINT32 keyModifiers, _In_ CPointerEventArgs* pPointerEventArgs);

        _Check_return_ HRESULT HitTestHelper(
            _In_ XPOINTF ptHit,
            _In_opt_ CDependencyObject *pHitTestRoot,
            _Outptr_result_maybenull_ CDependencyObject **ppVisualHit);

        _Check_return_ HRESULT HitTestWithLightDismissAwareness(
            _Inout_ xref_ptr<CDependencyObject>& contactDO,
            XPOINTF contactPoint,
            _In_ MessageMap message,
            _In_opt_ PointerInfo *pointerInfo,
            _In_ CDependencyObject* hitTestRoot);

        _Check_return_ HRESULT ProcessPointerEnterLeave(
            _In_opt_ CDependencyObject *pContactElement,
            _In_ CDependencyObject *pPointerEnterDO,
            _In_ XUINT32 pointerId,
            _In_ CPointerEventArgs *pArgs,
            _In_ XINT32 bSkipLeave,
            _In_ bool bForceRaisePointerEntered,
            _In_ bool bIgnoreHitTestVisibleForPointerExited = false,
            _In_ bool bAsyncEvent = false,
            _In_ bool bAddEventRequest = false,
            _Out_opt_ bool* enterLeaveFound = nullptr);

        _Check_return_ HRESULT ProcessPointerLeave(
            _In_ CDependencyObject *pExitedElement,
            _In_ XUINT32 pointerId,
            _In_ CPointerEventArgs *pPointerArgs,
            _In_ bool bAsyncEvent = false,
            _In_ bool bAddEventRequest = false,
            _In_opt_ CDependencyObject *pNewEnteredElement = NULL,
            _In_ bool bRaiseOnce = false);

        _Check_return_ HRESULT ReleasePointerCapture(_In_ CDependencyObject *pObject, _In_ CPointer* pPointer);
        _Check_return_ HRESULT SetPointerCapture( _In_ CDependencyObject *pObject, _In_ CPointer* pPointer, _Out_ bool* pResult );
        _Check_return_ HRESULT ReleaseAllPointerCaptures(_In_opt_ CDependencyObject *pObject);

        bool IsInputTypeTreatedLikeTouch(_In_ XPointerInputType pointerType);

        bool EnsureIslandMouseCaptureReleased();

    private:
        bool ShouldEventAlwaysPassThroughPopupLightDismissLayer(_In_ MessageMap message, _In_opt_ PointerInfo *pointerInfo);
        static bool ShouldEventCloseFlyout(_In_ MessageMap message);

        static _Check_return_ HRESULT GetRemovedPointerExitedEventRequest(
            _In_ EventHandle hPointerExited,
            _In_ CXcpList<REQUEST>* pEventList,
            _Out_ REQUEST** pEventRequestPointerExited);

        _Check_return_ HRESULT SetPointerFromPointerMessage(_In_ InputMessage *pMsg, _In_ CPointerEventArgs* pPointerEventArgs);

        _Check_return_ HRESULT ProcessPointerExitedState(
            _In_ XINT32 pointerId,
            _In_ CPointerEventArgs* pPointerEventArgs);

        _Check_return_ HRESULT ProcessPointerCaptureEnterLeave(
            _In_opt_ CDependencyObject *pContactElement,
            _In_opt_ CDependencyObject *pPointerEnterDO,
            _In_ CDependencyObject *pPointerCaptureDO,
            _In_ XUINT32 pointerId,
            _In_ CPointerEventArgs *pArgs);

        _Check_return_ HRESULT ReleasePointerCaptureById(_In_ XINT32 releasePointerId);

        CInputManager& m_inputManager;

        bool m_fBarrelButtonPressed = false;

        // PointerDownThemeAnimation uses PointerAnimationUsingKeyFrames, which gets the position of the pointer. Since
        // input injection isn't reliable, we're going to return a constant provided by the test.
        bool m_hasPrimaryPointerLastPositionOverride = false;
        XPOINTF m_primaryPointerLastPositionOverride{};

        // This is only set when the InputMessage is being held by a stack variable, so a raw pointer is OK here.
        InputMessage* m_messageBeingProcessed = nullptr;

        XINT32 m_iPrimaryPointerId = -1;
        XINT32 m_iRightButtonPointerId = -1;

        bool m_fSawMouseLeave = true;

        xref_ptr<CPointerEventArgs> m_pPendingPointerEventArgs;

        // InputMessage currently being process by the InputManager synchronously.
        // Is used by the ProcessInputMessageWithDirectManipulation method invoked
        // by the DM container synchronously.
        InputMessage* m_pCurrentMsgForDirectManipulationProcessing = nullptr;

        xref_ptr<CPointer> GetPointerForPointerInfo(_In_ const PointerInfo& pointerInfo);
        xref_ptr<CPointer> m_cachedPointer;

    };
};
