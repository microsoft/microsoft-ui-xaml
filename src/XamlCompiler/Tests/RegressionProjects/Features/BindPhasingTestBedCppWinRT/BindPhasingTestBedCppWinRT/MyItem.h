// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the data model exercised by the x:Phase bindings in MainPage.xaml.
//

#pragma once

#include "ExtraInfo.g.h"
#include "MyInfo.g.h"
#include "MyItem.g.h"

namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    struct ExtraInfo : ExtraInfoT<ExtraInfo>
    {
        ExtraInfo(hstring const& caption);

        hstring Caption();
        void Caption(hstring const& value);

    private:
        hstring m_caption;
    };

    struct MyInfo : MyInfoT<MyInfo>
    {
        MyInfo(hstring const& imageUrl, hstring const& caption);

        hstring ImageUrl();
        void ImageUrl(hstring const& value);
        hstring Caption();
        void Caption(hstring const& value);

    private:
        hstring m_imageUrl;
        hstring m_caption;
    };

    struct MyItem : MyItemT<MyItem>
    {
        MyItem(
            hstring const& title,
            hstring const& subtitle,
            hstring const& description,
            BindPhasingTestBedCppWinRT::MyInfo const& info,
            BindPhasingTestBedCppWinRT::ExtraInfo const& otherInfo,
            hstring const& dp);

        hstring Title();
        void Title(hstring const& value);
        hstring Subtitle();
        void Subtitle(hstring const& value);
        hstring Description();
        void Description(hstring const& value);
        BindPhasingTestBedCppWinRT::MyInfo Info();
        void Info(BindPhasingTestBedCppWinRT::MyInfo const& value);
        BindPhasingTestBedCppWinRT::ExtraInfo OtherInfo();
        void OtherInfo(BindPhasingTestBedCppWinRT::ExtraInfo const& value);

        hstring DPOnMyItem();
        void DPOnMyItem(hstring const& value);
        static Microsoft::UI::Xaml::DependencyProperty DPOnMyItemProperty();

        winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;

    private:
        void NotifyPropertyChanged(hstring const& propertyName);

        static Microsoft::UI::Xaml::DependencyProperty s_dpOnMyItemProperty;

        hstring m_title;
        hstring m_subtitle;
        hstring m_description;
        BindPhasingTestBedCppWinRT::MyInfo m_info{ nullptr };
        BindPhasingTestBedCppWinRT::ExtraInfo m_otherInfo{ nullptr };
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}

namespace winrt::BindPhasingTestBedCppWinRT::factory_implementation
{
    struct ExtraInfo : ExtraInfoT<ExtraInfo, implementation::ExtraInfo>
    {
    };

    struct MyInfo : MyInfoT<MyInfo, implementation::MyInfo>
    {
    };

    struct MyItem : MyItemT<MyItem, implementation::MyItem>
    {
    };
}
