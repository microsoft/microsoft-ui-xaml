// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarFlyoutItem.g.h"
#include "InkToolBarFlyoutItem.properties.h"

class InkToolBarFlyoutItem :
    public ReferenceTracker<InkToolBarFlyoutItem, winrt::implementation::InkToolBarFlyoutItemT>, 
    public InkToolBarFlyoutItemProperties
{
public:

    winrt::InkToolBarFlyoutItemKind Kind() { return m_kind; }
    void Kind(winrt::InkToolBarFlyoutItemKind value) { m_kind = value; }
    
    bool IsChecked() { return m_isChecked; }
    void IsChecked(bool value) { m_isChecked = value; }

private:
    winrt::InkToolBarFlyoutItemKind m_kind{ winrt::InkToolBarFlyoutItemKind::Simple };
    bool m_isChecked{ false };
};

