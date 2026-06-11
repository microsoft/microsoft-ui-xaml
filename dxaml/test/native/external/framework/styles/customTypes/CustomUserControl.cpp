// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomUserControl.h"

using namespace Microsoft::UI::Xaml;
using namespace Tests::Native::External::Framework::Styles;

DependencyProperty^ CustomUserControl::m_WorkingTagProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomStringProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomIntProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomSizeProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomThicknessProperty = nullptr;

void CustomUserControl::RegisterDependencyProperties()
{
    if (!m_WorkingTagProperty)
    {
        m_WorkingTagProperty =
            DependencyProperty::Register(
            L"WorkingTag",
            Platform::Object::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomStringProperty)
    {
        m_CustomStringProperty =
            DependencyProperty::Register(
            L"CustomString",
            Platform::String::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomIntProperty)
    {
        m_CustomIntProperty =
            DependencyProperty::Register(
            L"CustomInt",
            int::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomSizeProperty)
    {
        m_CustomSizeProperty =
            DependencyProperty::Register(
            L"CustomSize",
            ::Windows::Foundation::Size::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomThicknessProperty)
    {
        m_CustomThicknessProperty =
            DependencyProperty::Register(
            L"CustomThickness",
            Microsoft::UI::Xaml::Thickness::typeid,
            CustomUserControl::typeid,
            nullptr);
    }
}

void CustomUserControl::ClearDependencyProperties()
{
    m_WorkingTagProperty = nullptr;
    m_CustomStringProperty = nullptr;
    m_CustomIntProperty = nullptr;
    m_CustomSizeProperty = nullptr;
    m_CustomThicknessProperty = nullptr;
}