// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transition.g.h"

namespace DirectUI
{
    /// <summary>
    ///     Notifies an element that it is getting storyboards created on its behalf
    /// </summary>
    MIDL_INTERFACE("41960271-3fc8-4759-8c2c-ef3b5e6a1d05")
    ILayoutTransitionStoryboardNotification : public IInspectable
    {
        virtual _Check_return_ HRESULT NotifyLayoutTransitionStart() = 0;
        virtual _Check_return_ HRESULT NotifyLayoutTransitionEnd() = 0;
    };

    PARTIAL_CLASS(Transition)
    {
    public:
        virtual _Check_return_ HRESULT CreateStoryboardImpl(_In_ xaml::IUIElement* element, _In_ wf::Rect start, _In_ wf::Rect destination, _In_ xaml::TransitionTrigger transitionTrigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards, _Out_ xaml::TransitionParent* parentForTransition) { RRETURN(E_NOTIMPL); }
        virtual _Check_return_ HRESULT ParticipatesInTransitionImpl(_In_ xaml::IUIElement* element, _In_ xaml::TransitionTrigger transitionTrigger, _Out_ BOOLEAN* returnValue) { RRETURN(E_NOTIMPL); }

    // callbacks
    public:
        static _Check_return_ HRESULT ParticipateInTransition(_In_ CDependencyObject* nativeTransition, _In_ CDependencyObject* nativeUIElement, _In_ XINT32 transitionTrigger, _Out_ bool* DoesParticipate);
        static _Check_return_ HRESULT CreateStoryboardsForTransition(_In_ CDependencyObject* nativeTransition, _In_ CDependencyObject* nativeUIElement, _In_ XRECTF startBounds, _In_ XRECTF destinationBounds, _In_ XINT32 transitionTrigger, _Out_ XINT32* cStoryboardCount, _Outptr_result_buffer_(*cStoryboardCount) CStoryboard*** pppStoryboardArray, _Out_ DirectUI::TransitionParent* parentToTransitionEnum);
        static _Check_return_ HRESULT NotifyLayoutTransitionStart(_In_ CDependencyObject* nativeUIElement);
        static _Check_return_ HRESULT NotifyLayoutTransitionEnd( _In_ CDependencyObject* nativeUIElement);
    };
}

