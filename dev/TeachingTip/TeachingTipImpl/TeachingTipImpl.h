#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "TeachingTipTemplatePartHelpers.h"
using namespace ::winrt::Windows::Foundation;

namespace TeachingTipImpl
{
    TeachingTipContentStates GetContentStateImpl(const winrt::IInspectable& content);
    winrt::hstring GetPopupAutomationNameImpl(const winrt::hstring& automationName, const winrt::hstring& title);
    TeachingTipTitleBlockStates GetTitleVisibilityStateImpl(const winrt::hstring& title);
    TeachingTipSubtitleBlockStates GetSubtitleVisibilityStateImpl(const winrt::hstring& subtitle);
};
