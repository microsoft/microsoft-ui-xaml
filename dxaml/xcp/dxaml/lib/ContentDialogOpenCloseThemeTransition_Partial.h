// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentDialogOpenCloseThemeTransition.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ContentDialogOpenCloseThemeTransition)
    {

    public:
        _Check_return_ HRESULT CreateStoryboardImpl(_In_ xaml::IUIElement* element, _In_ wf::Rect start, _In_ wf::Rect destination, _In_ xaml::TransitionTrigger transitionTrigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards, _Out_ xaml::TransitionParent* parentForTransition) override;
        _Check_return_ HRESULT ParticipatesInTransitionImpl(_In_ xaml::IUIElement* element, _In_ xaml::TransitionTrigger transitionTrigger, _Out_ BOOLEAN* returnValue) override;

        _Check_return_ HRESULT SetSmokeLayer(_In_ xaml::IUIElement* smokeLayer)
        {
            IFC_RETURN(ctl::AsWeak(smokeLayer, &m_wrTargetSmokeLayer));
            return S_OK;
        }

        void GetSmokeLayer(_Outptr_result_maybenull_ xaml::IUIElement** smokeLayer)
        {
            *smokeLayer = m_wrTargetSmokeLayer ? m_wrTargetSmokeLayer.AsOrNull<xaml::IUIElement>().Detach() : nullptr;
        }

    public:
        static const INT64 s_OpenScaleDuration = 250;
        static const INT64 s_CloseScaleDuration = 167;
        static const INT64 s_OpacityChangeDuration = 83;

    private:
        ctl::WeakRefPtr m_wrTargetSmokeLayer{};
    };
}
