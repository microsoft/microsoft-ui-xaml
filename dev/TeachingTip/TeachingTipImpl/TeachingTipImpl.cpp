// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "pch.h"
#include "TeachingTipImpl.h"

namespace TeachingTipImpl
{
    winrt::hstring TeachingTipImpl::GetPopupAutomationNameImpl(const winrt::hstring& automationName, const winrt::hstring& title)
    {
        if (automationName.empty())
        {
            return title;
        }
        return automationName;
    }

    TeachingTipContentStates TeachingTipImpl::GetContentStateImpl(const winrt::IInspectable& content)
    {
        if (content)
        {
            return TeachingTipContentStates::Content;
        }
        else
        {
            return TeachingTipContentStates::NoContent;
        }
    }

    TeachingTipTitleBlockStates  TeachingTipImpl::GetTitleVisibilityStateImpl(const winrt::hstring& title)
    {
        if (title)
        {
            return TeachingTipTitleBlockStates::ShowTitleTextBlock;
        }
        else
        {
            return TeachingTipTitleBlockStates::CollapseTitleTextBlock;
        }
    }

    TeachingTipSubtitleBlockStates TeachingTipImpl::GetSubtitleVisibilityStateImpl(const winrt::hstring& subtitle)
    {
        if (subtitle)
        {
            return TeachingTipSubtitleBlockStates::ShowSubtitleTextBlock;
        }
        else
        {
            return TeachingTipSubtitleBlockStates::CollapseSubtitleTextBlock;
        }
    }

};
