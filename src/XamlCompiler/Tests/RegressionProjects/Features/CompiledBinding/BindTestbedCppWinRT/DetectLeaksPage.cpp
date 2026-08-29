// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "queue"
#include "DetectLeaksPage.h"
#include "DetectLeaksPage.g.cpp"
#include "MainPage.h"

namespace winrt::BindTestbed::implementation
{
    static std::queue<std::pair<std::wstring, ::winrt::weak_ref<IInspectable>>> objects;

    DetectLeaksPage::DetectLeaksPage()
    {
        InitializeComponent();
    }

    void DetectLeaksPage::TrackObject(impl::com_ref<IInspectable> const& obj, ::winrt::hstring const& name)
    {
        ::winrt::weak_ref<IInspectable> wr(obj);
        objects.push(std::make_pair(name.c_str(), wr));
    }

    void DetectLeaksPage::BackButton_Click(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        while (!objects.empty())
        {
            objects.pop();
        }
        Frame().Navigate(::winrt::xaml_typename<::winrt::BindTestbed::MainPage>());
    }

    void DetectLeaksPage::DetectLeakedObjects_Click(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        leakedObjectNames().Text(L"");
        while (!objects.empty())
        {
            auto weakRef = objects.front().second;
            auto name = objects.front().first;
            auto o = weakRef.get();
            if (o)
            {
                std::wstring newValue = leakedObjectNames().Text().c_str();
                newValue = newValue.append(name);
                newValue = newValue.append(L", ");
                leakedObjectNames().Text(newValue);
            }
            objects.pop();
        }
        DetectLeakedObjectsButton().IsEnabled(leakedObjectNames().Text().size() > 0);
    }


    void DetectLeaksPage::UpdateValues(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        App::Model.UpdateValues();
        //TODO: Convert BindTestbedModelCX to C++/WinRT
        //App::ModelCX.UpdateValues();
        App::DOModel.UpdateValues();
    }
}