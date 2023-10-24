// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"

class CFocusManager;

namespace Focus {

    struct XYFocusOptions;
    class FocusMovementResult;

    class FocusMovement
    {
    public:
        void* operator new(size_t) = delete;
        FocusMovement(const FocusMovement& copy) = default;

        FocusMovement(
            _In_ XYFocusOptions& xyFocusOptions,
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_opt_ CDependencyObject* pTarget);

        FocusMovement(
            _In_ CDependencyObject* pTarget,
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_ DirectUI::FocusState focusState)
            : direction(direction), pTarget(pTarget), focusState(focusState) { UuidCreate(&correlationId ); }

        FocusMovement(
            _In_ CDependencyObject* pTarget,
            _In_ const FocusMovement& copy)
            : pTarget(pTarget),
            focusState(copy.focusState),
            direction(copy.direction),
            xyFocusOptions(copy.xyFocusOptions),
            forceBringIntoView(copy.forceBringIntoView),
            animateIfBringIntoView(copy.animateIfBringIntoView),
            isProcessingTab(copy.isProcessingTab),
            isShiftPressed(copy.isShiftPressed),
            canCancel(copy.canCancel),
            canDepartFocus(copy.canDepartFocus),
            canNavigateFocus(copy.canNavigateFocus),
            raiseGettingLosingEvents(copy.raiseGettingLosingEvents),
            shouldCompleteAsyncOperation(copy.shouldCompleteAsyncOperation),
            correlationId(copy.correlationId),
            isForLightDismiss(copy.isForLightDismiss),
            requestInputActivation(copy.requestInputActivation)
            {}
        
    private:
        CDependencyObject* pTarget = nullptr;
        DirectUI::FocusState focusState = DirectUI::FocusState::Unfocused;
        DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::None;
        XYFocusOptions* const xyFocusOptions = nullptr;
        GUID correlationId = {};

    public:
        CDependencyObject* GetTarget() const { return pTarget; }
        DirectUI::FocusState GetFocusState() const { return focusState; }
        DirectUI::FocusNavigationDirection GetDirection() const { return direction; }
        XYFocusOptions* const GetXYFocusOptions() const { return xyFocusOptions; }
        GUID GetCorrelationId() const { return correlationId; }
        void SetCorrelationId(GUID cid) { correlationId = cid; }

        bool forceBringIntoView = false;
        bool animateIfBringIntoView = false;
        bool isProcessingTab = false;
        bool isShiftPressed = false;
        bool canCancel = true;
        bool canDepartFocus = true;
        bool canNavigateFocus = true;
        bool raiseGettingLosingEvents = true;
        bool shouldCompleteAsyncOperation = false;
        bool isForLightDismiss = false;
        bool requestInputActivation = true; // Defaults to true to match legacy behavior all focus movement used to have.
                                            // See InputActivationBehavior.h for more details.
    };

    class FocusMovementResult
    {
    public:
        void* operator new(size_t) = delete;
        FocusMovementResult(const FocusMovementResult& copy) = default;

        FocusMovementResult() = default;
        FocusMovementResult(HRESULT hrResult, const FocusMovementResult& copy)
            : wasMoved(copy.wasMoved),
            wasProcessed(copy.wasProcessed),
            wasCanceled(copy.wasCanceled),
            hResult(copy.hResult) {}
        FocusMovementResult(bool wasMoved, bool wasCanceled, HRESULT hResult)
            : wasMoved(wasMoved),
            wasCanceled(wasCanceled),
            hResult(hResult) {}

    public:
        bool WasMoved() const
        {
            return wasMoved;
        }

        bool WasCanceled() const
        {
            return wasCanceled;
        }

        _Check_return_ HRESULT GetHResult() const
        {
            return hResult;
        }

    private:
        bool wasMoved = false;
        bool wasProcessed = false;
        bool wasCanceled = false;
        HRESULT hResult = S_OK;
    };

}
