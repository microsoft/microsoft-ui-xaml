// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomUserControl.h"
#include <collection.h>
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace ::Windows::Foundation::Collections;
using namespace ::Tests::Native::External::Framework; 

DependencyProperty^ CustomUserControl::m_WorkingTagProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomStringProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomSizeProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomThicknessProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomIntProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomDoubleProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomUnknownProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomUserCollectionProperty;
DependencyProperty^ CustomUserControl::m_CustomAttachedProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomBrushProperty = nullptr;

CustomUserControl::CustomUserControl()
{
    Application::LoadComponent(
        this,
        ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/CustomUserControl.xaml"),
        Primitives::ComponentResourceLocation::Application);
    CustomUserCollection = ref new Platform::Collections::Vector<DependencyObject^>();
}

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
    
    if (!m_CustomIntProperty)
    {
        m_CustomIntProperty =
            DependencyProperty::Register(
            L"CustomInt",
            int::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomDoubleProperty)
    {
        m_CustomDoubleProperty =
            DependencyProperty::Register(
            L"CustomDouble",
            double::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomUnknownProperty)
    {
        m_CustomUnknownProperty =
            DependencyProperty::Register(
            nullptr, // pass in nullptr so the property has no name and verify the default value
            Object::typeid,
            Object::typeid,
            nullptr);
    }

    if (!m_CustomUserCollectionProperty)
    {
        m_CustomUserCollectionProperty =
            DependencyProperty::Register(
            L"CustomUserCollection",
            IVector<DependencyObject^>::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomAttachedProperty)
    {
        ::Windows::UI::Xaml::Interop::TypeName unknownType;
        unknownType.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Custom;
        unknownType.Name = "UnknownClass";
        m_CustomAttachedProperty = DependencyProperty::RegisterAttached(
            L"CustomAttached",
            int::typeid,
            // The xaml runtime has no knowledge of the UnknownClass
            unknownType,
            nullptr
        );
    }

    if (!m_CustomBrushProperty)
    {
        m_CustomBrushProperty = 
            DependencyProperty::Register(
            L"CustomBrush",
            Microsoft::UI::Xaml::Media::Brush::typeid,
            CustomUserControl::typeid,
            nullptr);
    }
}

void CustomUserControl::ClearDependencyProperties()
{
    m_WorkingTagProperty = nullptr;
    m_CustomStringProperty = nullptr;
    m_CustomSizeProperty = nullptr;
    m_CustomThicknessProperty = nullptr;
    m_CustomIntProperty = nullptr;
    m_CustomDoubleProperty = nullptr;
    m_CustomUnknownProperty = nullptr;
    m_CustomUserCollectionProperty = nullptr;
    m_CustomAttachedProperty = nullptr;
    m_CustomBrushProperty = nullptr;
}
