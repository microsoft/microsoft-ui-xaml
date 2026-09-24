// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MyItem.g.h"

namespace winrt::BindTestbed::implementation
{
    struct MyItem : MyItemT<MyItem>
    {
        MyItem() = default;
        MyItem(
            int index,
            hstring const& title,
            hstring const& subtitle,
            hstring const& description,
            BindTestbed::MyInfo const& info,
            BindTestbed::ExtraInfo const& otherInfo,
            hstring const& dp)
        {
            Index(index);
            Title(title);
            Subtitle(subtitle);
            Description(description);
            Info(info);
            OtherInfo(otherInfo);
            DPOnMyItem(dp);
        }

        hstring Title()
        {
            return title;
        }
        void Title(hstring value)
        {
            title = value;
        }

        hstring Subtitle()
        {
            return subtitle;
        }
        void Subtitle(hstring value)
        {
            subtitle = value;
        }

        hstring Description()
        {
            return description;
        }
        void Description(hstring value)
        {
            if (description != value)
            {
                description = value;
                NotifyPropertyChanged(L"Description");
            }
        }

        int32_t Index()
        {
            return index;
        }
        void Index(int32_t value)
        {
            index = value;
        }

        BindTestbed::MyInfo Info()
        {
            return myInfo;
        }
        void Info(BindTestbed::MyInfo value)
        {
            myInfo = value;
        }

        BindTestbed::ExtraInfo OtherInfo()
        {
            return otherInfo;
        }
        void OtherInfo(BindTestbed::ExtraInfo value)
        {
            otherInfo = value;
        }

        hstring DPOnMyItem()
        {
            return unbox_value<hstring>(GetValue(dpOnMyItemProperty));
        }
        void DPOnMyItem(hstring const& value)
        {
            SetValue(dpOnMyItemProperty, box_value(value));
        }
        static wux::DependencyProperty DPOnMyItemProperty()
        {
            return dpOnMyItemProperty;
        }
        static void DPOnMyItemProperty(wux::DependencyProperty value)
        {
            dpOnMyItemProperty = value;
        }

        void PropertyChanged(event_token const token)
        {
            propertyChanged.remove(token);
        }
        event_token PropertyChanged(wux::Data::PropertyChangedEventHandler const& handler)
        {
            return propertyChanged.add(handler);
        }

    private:
        event<wux::Data::PropertyChangedEventHandler> propertyChanged;
        void NotifyPropertyChanged(hstring const& propertyName)
        {
            propertyChanged(*this, wux::Data::PropertyChangedEventArgs(propertyName));
        }

    private:
        hstring title{};
        hstring subtitle{};
        hstring description{};
        int32_t index;
        BindTestbed::MyInfo myInfo{ nullptr };
        BindTestbed::ExtraInfo otherInfo{ nullptr };
        static wux::DependencyProperty dpOnMyItemProperty;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct MyItem : MyItemT<MyItem, implementation::MyItem>
    {
    };
}
