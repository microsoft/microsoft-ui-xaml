// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DynamicTimeline.h>
#include <GeneratedClasses.g.h>

void CEdgeUIThemeTransition::InitializeDefaults()
{
    m_edge = DirectUI::EdgeTransitionLocation::Top;
}

void CEntranceThemeTransition::InitializeDefaults()
{
    m_isStaggeringEnabled = false;
}

void CRepositionThemeTransition::InitializeDefaults()
{
    m_isStaggeringEnabled = true;
}

void CFadeInThemeAnimation::InitializeDefaults()
{
}

void CFadeOutThemeAnimation::InitializeDefaults()
{
}

void CFrame::InitializeDefaults()
{
}

void CScrollBar::InitializeDefaults()
{
    m_orientation = DirectUI::Orientation::Vertical;
}

void CSlider::InitializeDefaults()
{
    m_maximum = 100;
}

void CPageStackEntry::InitializeDefaults()
{
}

void CPVLStaggerFunction::InitializeDefaults()
{
}

void CListBox::InitializeDefaults()
{
    m_singleSelectionFollowsFocus = true;
}

void CListViewBase::InitializeDefaults()
{
    m_singleSelectionFollowsFocus = true;
}

void CSelectorItem::InitializeDefaults()
{
}
