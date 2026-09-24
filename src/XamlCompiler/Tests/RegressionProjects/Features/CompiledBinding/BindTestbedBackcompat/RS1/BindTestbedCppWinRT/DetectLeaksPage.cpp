// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DetectLeaksPage.h"
#include "MainPage.h"
#include "queue"

using namespace winrt::BindTestbed::implementation;

// static std::queue<std::pair<std::wstring, WeakReference>> objects;

DetectLeaksPage::DetectLeaksPage()
{
    InitializeComponent();
}

void DetectLeaksPage::TrackObject(IInspectable const& obj, ::winrt::hstring const& name)
{
    obj;
    name;
    /*objects.push(std::make_pair(name, WeakReference(obj)));*/
}

void DetectLeaksPage::BackButton_Click(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
{
   /* while (!objects.empty())
    {
        objects.pop();
    }
    */
    Frame().Navigate(::winrt::xaml_typename<::winrt::BindTestbed::MainPage>());
}

void DetectLeaksPage::DetectLeakedObjects_Click(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
{
   /* leakedObjectNames->Text = "";
    while (!objects.empty())
    {
        const ::Platform::WeakReference& ref = objects.front().second;
        String^ name = objects.front().first;
        ::Platform::Object^ o = ref.Resolve<::Platform::Object>();
        if (o != nullptr)
        {
            leakedObjectNames->Text = leakedObjectNames->Text->Concat(leakedObjectNames->Text, name);
            leakedObjectNames->Text = leakedObjectNames->Text->Concat(leakedObjectNames->Text, ", ");
        }
        objects.pop();
    }
    DetectLeakedObjectsButton->IsEnabled = leakedObjectNames->Text->Length() > 0;*/
}


void DetectLeaksPage::UpdateValues(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
{
   /* App::Model->UpdateValues();
    App::ModelCX->UpdateValues();
    App::DOModel->UpdateValues();*/
}
