// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomControl.h"

using namespace Microsoft::UI::Xaml;
using namespace ::Tests::Native::External::Framework;

DependencyProperty^ CustomControl::m_WorkingTagProperty = nullptr;
DependencyProperty^ CustomControl::m_AttachedTagProperty = nullptr;
DependencyProperty^ CustomControl::m_MyWidthProperty = nullptr;
DependencyProperty^ CustomControl::m_VectorProperty = nullptr;
DependencyProperty^ CustomControl::m_MyUriProperty = nullptr;
DependencyProperty^ CustomControl::m_MyGuidProperty = nullptr;
DependencyProperty^ CustomControl::m_CustomStringProperty = nullptr;
DependencyProperty^ CustomControl::m_CustomIntProperty = nullptr;
DependencyProperty^ CustomControl::m_CustomSizeProperty = nullptr;
DependencyProperty^ CustomControl::m_CustomThicknessProperty = nullptr;
DependencyProperty^ CustomControl::m_CustomFontFamilyProperty = nullptr;
DependencyProperty^ CustomControl::m_CustomStyleProperty = nullptr;
DependencyProperty^ CustomControl::m_CustomEnumProperty = nullptr;

void CustomControl::RegisterDependencyProperties()
{
    if (!m_WorkingTagProperty)
    {
        m_WorkingTagProperty = DependencyProperty::Register(
            L"WorkingTag",
            Platform::Object::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_AttachedTagProperty)
    {
        m_AttachedTagProperty = DependencyProperty::RegisterAttached(
            L"AttachedTag",
            Platform::Object::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_MyWidthProperty)
    {
        m_MyWidthProperty = DependencyProperty::RegisterAttached(
            L"MyWidth",
            double::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_MyUriProperty)
    {
        m_MyUriProperty = DependencyProperty::Register(
            L"MyUri",
            ::Windows::Foundation::Uri::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_MyGuidProperty)
    {
        m_MyGuidProperty = DependencyProperty::Register(
            L"MyGuid",
            Platform::Guid::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_VectorProperty)
    {
        m_VectorProperty = DependencyProperty::RegisterAttached(
            L"Vector",
            ::Windows::Foundation::Collections::IVector<CustomControl^>::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_CustomStringProperty)
    {
        m_CustomStringProperty =
            DependencyProperty::Register(
            L"CustomString",
            Platform::String::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_CustomIntProperty)
    {
        m_CustomIntProperty =
            DependencyProperty::Register(
            L"CustomInt",
            int::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_CustomSizeProperty)
    {
        m_CustomSizeProperty =
            DependencyProperty::Register(
            L"CustomSize",
            ::Windows::Foundation::Size::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_CustomThicknessProperty)
    {
        m_CustomThicknessProperty =
            DependencyProperty::Register(
            L"CustomThickness",
            Microsoft::UI::Xaml::Thickness::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_CustomFontFamilyProperty)
    {
        m_CustomFontFamilyProperty =
            DependencyProperty::Register(
            L"CustomFontFamily",
            Microsoft::UI::Xaml::Media::FontFamily::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_CustomStyleProperty)
    {
        m_CustomStyleProperty =
            DependencyProperty::Register(
            L"CustomStyle",
            Microsoft::UI::Xaml::Style::typeid,
            CustomControl::typeid,
            nullptr);
    }

    if (!m_CustomEnumProperty)
    {
        m_CustomEnumProperty =
            DependencyProperty::Register(
            L"CustomEnum",
            CustomEnumValues::typeid,
            CustomControl::typeid,
            ref new PropertyMetadata(CustomEnumValues::CustomEnumValue0));
    }
}

void CustomControl::ClearDependencyProperties()
{
    m_WorkingTagProperty = nullptr;
    m_AttachedTagProperty = nullptr;
    m_MyWidthProperty = nullptr;
    m_VectorProperty = nullptr;
    m_MyUriProperty = nullptr;
    m_MyGuidProperty = nullptr;
    m_CustomStringProperty = nullptr;
    m_CustomIntProperty = nullptr;
    m_CustomSizeProperty = nullptr;
    m_CustomThicknessProperty = nullptr;
    m_CustomFontFamilyProperty = nullptr;
    m_CustomStyleProperty = nullptr;
    m_CustomEnumProperty = nullptr;
}
