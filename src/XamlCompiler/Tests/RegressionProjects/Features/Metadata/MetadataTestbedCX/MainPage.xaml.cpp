// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#include "App.xaml.h"
#include "MainPage.xaml.h"

using namespace MetadataTestbedCX;
using namespace concurrency;

MainPage::MainPage()
{
    InitializeComponent();
}

void MainPage::GetTypeMemberManyTimesClicked(Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    GetTypeMemberButton->IsEnabled = false;
    create_task([&] { Test(); });
}

void MainPage::Test()
{
    for (int i =0; i < 50000; i++)
    {
        provider = ref new ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider();
        auto t1 = create_task([&]{ GetTypeMemberTest(); });
        auto t2 = create_task([&]{ GetTypeMemberTest(); });
        t1.wait();
        t2.wait();
    }
    Dispatcher->RunAsync(::Windows::UI::Core::CoreDispatcherPriority::Normal, ref new ::Windows::UI::Core::DispatchedHandler([&] {
        GetTypeMemberButton->IsEnabled = true;
    }));
}

void MainPage::GetTypeMemberTest()
{
    auto type = provider->GetXamlTypeByName("MetadataTestbedCX.MainPage");
    auto member = type->GetMember("TestProperty");
    if (type == nullptr || member == nullptr)
    {
        throw ref new Platform::FailureException("Test failed");
    }
}