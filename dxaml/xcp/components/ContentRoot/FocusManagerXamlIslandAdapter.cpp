// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FocusManagerXamlIslandAdapter.h"

#include "ContentRoot.h"

using namespace ContentRootAdapters;

FocusManagerXamlIslandAdapter::FocusManagerXamlIslandAdapter(_In_ CContentRoot& contentRoot)
    : FocusAdapter(contentRoot)
{
}

void FocusManagerXamlIslandAdapter::SetFocus()
{
    // We have moved the focus to an element hosted in an Island
    // Make sure that this island has also focus
    boolean hasFocusNow = false;
    VERIFYHR(m_contentRoot.GetXamlIslandRootNoRef()->TrySetFocus(&hasFocusNow));
    ASSERT(hasFocusNow, L"Failed to move focus to xaml island");
}

bool FocusManagerXamlIslandAdapter::ShouldDepartFocus(_In_ DirectUI::FocusNavigationDirection direction) const
{
    const bool isTabbingDirection = direction == DirectUI::FocusNavigationDirection::Next || direction == DirectUI::FocusNavigationDirection::Previous;
    const bool focusScopeIsIsland = m_contentRoot.GetXamlIslandRootNoRef()->IsActive();

    return isTabbingDirection && focusScopeIsIsland;
}