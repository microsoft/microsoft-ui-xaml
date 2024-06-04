// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PointerUpThemeAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(PointerUpThemeAnimation)
    {
        public:
            PointerUpThemeAnimation();

            void SetAnimationTarget(
                _In_ DependencyObject* const pAnimationTarget)
            {
                SetPtrValue(m_tpAnimationTarget, pAnimationTarget);
            }

            _Check_return_ HRESULT CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection) override;

        private:
            EventRegistrationToken m_completedToken;
            TrackerPtr<DependencyObject> m_tpAnimationTarget;

            _Check_return_ HRESULT OnTimelineCompleted(_In_ IInspectable* /*pSender*/, _In_ IInspectable* /*pArgs*/);
    };
}
