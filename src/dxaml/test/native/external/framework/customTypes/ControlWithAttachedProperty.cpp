// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ControlWithAttachedProperty.h"

using namespace Microsoft::UI::Xaml;
using namespace Tests::Native::External::Framework;

DependencyProperty^ ControlWithAttachedProperty::m_CustomAttachedProperty = nullptr;
DependencyProperty^ ControlWithAttachedProperty::m_CustomAttachedIntProperty = nullptr;
DependencyProperty^ ControlWithAttachedProperty::m_CustomAttachedBrushProperty = nullptr;
DependencyProperty^ ControlWithAttachedProperty::m_CustomAttachedBrushCollectionProperty = nullptr;

void ControlWithAttachedProperty::RegisterDependencyProperties()
{
    if (!m_CustomAttachedProperty)
    {
        m_CustomAttachedProperty =
            DependencyProperty::RegisterAttached(
            L"CustomAttached",
            Platform::String::typeid,
            ControlWithAttachedProperty::typeid,
            nullptr);
    }

    if (!m_CustomAttachedIntProperty)
    {
        m_CustomAttachedIntProperty =
            DependencyProperty::RegisterAttached(
            L"CustomAttachedInt",
            int::typeid,
            ControlWithAttachedProperty::typeid,
            nullptr);
    }

    if (!m_CustomAttachedBrushProperty)
    {
        m_CustomAttachedBrushProperty =
            DependencyProperty::RegisterAttached(
            L"CustomAttachedBrush",
            Microsoft::UI::Xaml::Media::Brush::typeid,
            ControlWithAttachedProperty::typeid,
            nullptr);
    }

    if (!m_CustomAttachedBrushCollectionProperty)
    {
        m_CustomAttachedBrushCollectionProperty =
            DependencyProperty::RegisterAttached(
            L"CustomAttachedBrushCollection",
            Microsoft::UI::Xaml::Media::BrushCollection::typeid,
            ControlWithAttachedProperty::typeid,
            nullptr);
    }
}

void ControlWithAttachedProperty::ClearDependencyProperties()
{
    m_CustomAttachedProperty = nullptr;
    m_CustomAttachedIntProperty = nullptr;
    m_CustomAttachedBrushProperty = nullptr;
    m_CustomAttachedBrushCollectionProperty = nullptr;
}
