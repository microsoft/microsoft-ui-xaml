// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomUserControl.h"
#include <collection.h>
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace ::Windows::Foundation::Collections;
using namespace ::Tests::Tools::Shared; 

DependencyProperty^ CustomUserControl::m_CustomIntProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomDoubleProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomUnknownProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomUserCollectionProperty;
DependencyProperty^ CustomUserControl::m_CustomAttachedProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomBrushProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomChildProperty = nullptr;
DependencyProperty^ CustomUserControl::m_CustomThoughtsProperty = nullptr;

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
        // Pass in a random TypeName for the ownerType. Xaml won't be able to resolve it (since it doesn't exist)
        // and we'll report the property as ".CustomAttached" in the LPE
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

    if (!m_CustomChildProperty)
    {
        m_CustomChildProperty =
            DependencyProperty::Register(
            L"CustomChild",
            CustomUserControl::typeid,
            CustomUserControl::typeid,
            nullptr);
    }

    if (!m_CustomThoughtsProperty)
    {
        m_CustomThoughtsProperty =
            DependencyProperty::Register(
            L"CustomThoughts",
            ThoughtProcess::typeid,
            CustomUserControl::typeid,
            ref new PropertyMetadata(ThoughtProcess::MaybeSo));
    }
}

void CustomUserControl::ClearDependencyProperties()
{
    m_CustomIntProperty = nullptr;
    m_CustomDoubleProperty = nullptr;
    m_CustomUnknownProperty = nullptr;
    m_CustomUserCollectionProperty = nullptr;
    m_CustomAttachedProperty = nullptr;
    m_CustomBrushProperty = nullptr;
    m_CustomChildProperty = nullptr;
    m_CustomThoughtsProperty = nullptr;
}
