// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "ThemeGenerator.h"

#pragma once

#include "DropTargetItemThemeAnimation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DropTargetItemThemeAnimation)
    {
        // Used for phone only.
        struct FloatingTimingFunctionDescription: TimingFunctionDescription
        {
            FloatingTimingFunctionDescription()
            {
                // Exponential ease.
                cp1.X = 0.0f;
                cp1.Y = 0.0f;
                cp2.X = 0.1f;
                cp2.Y = 0.25f;
                cp3.X = 0.75f;
                cp3.Y = 0.9f;
                cp4.X = 1.0f;
                cp4.Y = 1.0f;
            }
        };

        static std::default_random_engine m_generator;

        public:
            _Check_return_ HRESULT CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection) override;
    };
}
