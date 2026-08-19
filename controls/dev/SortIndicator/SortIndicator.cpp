// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SortIndicator.h"
#include "SortIndicatorAutomationPeer.h"
#include "RuntimeProfiler.h"

static constexpr std::wstring_view s_NoSortStateName{ L"NoSort"sv };
static constexpr std::wstring_view s_AscendingStateName{ L"Ascending"sv };
static constexpr std::wstring_view s_DescendingStateName{ L"Descending"sv };
static constexpr std::wstring_view s_LayoutRootPartName{ L"LayoutRoot"sv };
static constexpr std::wstring_view s_GlyphIconPartName{ L"GlyphIcon"sv };

// Segoe Fluent Icons ScrollChevronUpLegacy / ScrollChevronDownLegacy -- chevrons, not the
// SortUp/SortDown glyphs, because a chevron reads correctly at header scale.
static constexpr std::wstring_view s_AscendingGlyph{ L"\uE96D"sv };
static constexpr std::wstring_view s_DescendingGlyph{ L"\uE96E"sv };

SortIndicator::SortIndicator()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SortIndicator);

    SetDefaultStyleKey(this);
}

void SortIndicator::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    // Opacity/Glyph are driven imperatively: WinUI 3 VisualState.Setters can fail to re-apply
    // Opacity on NoSort -> Ascending (microsoft-ui-xaml#6203), leaving the chevron invisible.
    m_layoutRoot.set(GetTemplateChildT<winrt::FrameworkElement>(s_LayoutRootPartName, *this));
    m_glyphIcon.set(GetTemplateChildT<winrt::FontIcon>(s_GlyphIconPartName, *this));

    // Missing either part renders nothing with no other symptom; catch it in chk.
    MUX_ASSERT(m_layoutRoot.get() != nullptr);
    MUX_ASSERT(m_glyphIcon.get() != nullptr);

    UpdateVisualState(false /* useTransitions */);
}

winrt::AutomationPeer SortIndicator::OnCreateAutomationPeer()
{
    return winrt::make<SortIndicatorAutomationPeer>(*this);
}

void SortIndicator::OnDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    UpdateVisualState(true /* useTransitions */);
}

void SortIndicator::UpdateVisualState(bool useTransitions)
{
    const auto direction = Direction();

    bool hasSort{};
    std::wstring_view glyph{ s_AscendingGlyph };
    std::wstring_view directionStateName{};
    switch (direction)
    {
        case winrt::SortIndicatorDirection::Ascending:
            hasSort = true;
            glyph = s_AscendingGlyph;
            directionStateName = s_AscendingStateName;
            break;
        case winrt::SortIndicatorDirection::Descending:
            hasSort = true;
            glyph = s_DescendingGlyph;
            directionStateName = s_DescendingStateName;
            break;
        case winrt::SortIndicatorDirection::None:
        default:
            hasSort = false;
            glyph = s_AscendingGlyph;
            directionStateName = s_NoSortStateName;
            break;
    }

    // GoToState first, so the imperative writes below win over any consumer VSM Setter.
    winrt::VisualStateManager::GoToState(*this, directionStateName, useTransitions);

    if (auto layoutRoot = m_layoutRoot.get())
    {
        layoutRoot.Opacity(hasSort ? 1.0 : 0.0);
    }
    if (auto glyphIcon = m_glyphIcon.get())
    {
        // Opacity above hides it when None; still set a glyph so debugger inspection is sane.
        glyphIcon.Glyph(hstring{ glyph });
    }
}
