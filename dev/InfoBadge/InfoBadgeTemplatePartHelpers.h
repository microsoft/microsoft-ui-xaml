// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "pch.h"
#include "common.h"

enum class InfoBadgeNamedTemplatePart
{
    RootGrid,
    ValueTextBlock,
    IconPresenter
};

enum class InfoBadgeDisplayKindState
{
    Dot,
    FontIcon,
    Icon,
    Value
};

class InfoBadgeTemplateHelpers
{
public:
    // Abstract Class
    virtual ~InfoBadgeTemplateHelpers() = 0;

#pragma region DisplayKindStates
public:
    static bool GoToState(winrt::Control control, InfoBadgeDisplayKindState state, bool useTransitions = true)
    {
        return winrt::VisualStateManager::GoToState(control, ToString(state), useTransitions);
    }

private:
    static winrt::hstring ToString(InfoBadgeDisplayKindState infoBadgeDisplayKindStates)
    {
        switch (infoBadgeDisplayKindStates)
        {
        case InfoBadgeDisplayKindState::Dot:
            return c_DotDisplayKindState.data();
        case InfoBadgeDisplayKindState::FontIcon:
            return c_FontIconDisplayKindState.data();
        case InfoBadgeDisplayKindState::Icon:
            return c_IconDisplayKindState.data();
        default:
            return c_ValueDisplayKindState.data();
        }
    }

    static constexpr wstring_view c_DotDisplayKindState{ L"Dot"sv };
    static constexpr wstring_view c_FontIconDisplayKindState{ L"FontIcon"sv };
    static constexpr wstring_view c_IconDisplayKindState{ L"Icon"sv };
    static constexpr wstring_view c_ValueDisplayKindState{ L"Value"sv };
#pragma endregion

#pragma region NamedTemplateParts
public:
    static winrt::hstring ToString(InfoBadgeNamedTemplatePart infoBadgeNamedTemplateParts)
    {
        switch (infoBadgeNamedTemplateParts)
        {
        case InfoBadgeNamedTemplatePart::RootGrid:
            return c_RootGridName.data();
        case InfoBadgeNamedTemplatePart::ValueTextBlock:
            return c_ValueTextBlockName.data();
        default:
            return c_IconPresenterName.data();
        }
    }
private:
    static constexpr wstring_view c_RootGridName{ L"RootGrid"sv };
    static constexpr wstring_view c_ValueTextBlockName{ L"ValueTextBlock"sv };
    static constexpr wstring_view c_IconPresenterName{ L"IconPresenter"sv };
#pragma endregion
};

