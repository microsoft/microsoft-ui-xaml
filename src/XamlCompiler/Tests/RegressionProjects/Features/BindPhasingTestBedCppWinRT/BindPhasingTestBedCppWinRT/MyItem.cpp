// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the data model exercised by the x:Phase bindings in MainPage.xaml.
//

#include "pch.h"
#include "MyItem.h"
// MyItem.h/.cpp carry all three model types, so this translation unit also has to pull in
// the activation stubs cppwinrt generates for the other two.
#include "ExtraInfo.g.cpp"
#include "MyInfo.g.cpp"
#include "MyItem.g.cpp"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Interop;

namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    ExtraInfo::ExtraInfo(hstring const& caption) : m_caption(caption)
    {
    }

    hstring ExtraInfo::Caption()
    {
        return m_caption;
    }

    void ExtraInfo::Caption(hstring const& value)
    {
        m_caption = value;
    }

    MyInfo::MyInfo(hstring const& imageUrl, hstring const& caption) : m_imageUrl(imageUrl), m_caption(caption)
    {
    }

    hstring MyInfo::ImageUrl()
    {
        return m_imageUrl;
    }

    void MyInfo::ImageUrl(hstring const& value)
    {
        m_imageUrl = value;
    }

    hstring MyInfo::Caption()
    {
        return m_caption;
    }

    void MyInfo::Caption(hstring const& value)
    {
        m_caption = value;
    }

    DependencyProperty MyItem::s_dpOnMyItemProperty =
        DependencyProperty::Register(
            L"DPOnMyItem",
            xaml_typename<hstring>(),
            xaml_typename<BindPhasingTestBedCppWinRT::MyItem>(),
            nullptr);

    MyItem::MyItem(
        hstring const& title,
        hstring const& subtitle,
        hstring const& description,
        BindPhasingTestBedCppWinRT::MyInfo const& info,
        BindPhasingTestBedCppWinRT::ExtraInfo const& otherInfo,
        hstring const& dp)
    {
        Title(title);
        Subtitle(subtitle);
        Description(description);
        Info(info);
        OtherInfo(otherInfo);
        DPOnMyItem(dp);
    }

    hstring MyItem::Title()
    {
        return m_title;
    }

    void MyItem::Title(hstring const& value)
    {
        m_title = value;
    }

    hstring MyItem::Subtitle()
    {
        return m_subtitle;
    }

    void MyItem::Subtitle(hstring const& value)
    {
        m_subtitle = value;
    }

    hstring MyItem::Description()
    {
        return m_description;
    }

    void MyItem::Description(hstring const& value)
    {
        if (value != m_description)
        {
            m_description = value;
            NotifyPropertyChanged(L"Description");
        }
    }

    BindPhasingTestBedCppWinRT::MyInfo MyItem::Info()
    {
        return m_info;
    }

    void MyItem::Info(BindPhasingTestBedCppWinRT::MyInfo const& value)
    {
        m_info = value;
    }

    BindPhasingTestBedCppWinRT::ExtraInfo MyItem::OtherInfo()
    {
        return m_otherInfo;
    }

    void MyItem::OtherInfo(BindPhasingTestBedCppWinRT::ExtraInfo const& value)
    {
        m_otherInfo = value;
    }

    hstring MyItem::DPOnMyItem()
    {
        return unbox_value_or<hstring>(GetValue(s_dpOnMyItemProperty), L"");
    }

    void MyItem::DPOnMyItem(hstring const& value)
    {
        SetValue(s_dpOnMyItemProperty, box_value(value));
    }

    DependencyProperty MyItem::DPOnMyItemProperty()
    {
        return s_dpOnMyItemProperty;
    }

    winrt::event_token MyItem::PropertyChanged(PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void MyItem::PropertyChanged(winrt::event_token const& token) noexcept
    {
        m_propertyChanged.remove(token);
    }

    void MyItem::NotifyPropertyChanged(hstring const& propertyName)
    {
        m_propertyChanged(*this, PropertyChangedEventArgs(propertyName));
    }
}
