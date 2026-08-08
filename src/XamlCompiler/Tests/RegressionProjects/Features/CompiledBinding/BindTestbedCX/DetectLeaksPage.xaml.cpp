// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Shutdown.xaml.cpp
// Implementation of the Shutdown class
//

#include "pch.h"
#include "queue"
#include "DetectLeaksPage.xaml.h"
#include "MainPage.xaml.h"

using namespace BindTestbedCX;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

static std::queue<std::pair<String^, WeakReference>> objects;

DetectLeaksPage::DetectLeaksPage()
{
    InitializeComponent();
}

void DetectLeaksPage::TrackObject(Object^ obj, String^ name)
{
    objects.push(std::make_pair(name, WeakReference(obj)));
}

void DetectLeaksPage::BackButton_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    while (!objects.empty())
    {
        objects.pop();
    }
    Frame->Navigate(MainPage::typeid);
}

void DetectLeaksPage::DetectLeakedObjects_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    leakedObjectNames->Text = "";
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
    DetectLeakedObjectsButton->IsEnabled = leakedObjectNames->Text->Length() > 0;
}


void BindTestbedCX::DetectLeaksPage::UpdateValues(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    App::Model->UpdateValues();
    App::ModelCX->UpdateValues();
    App::DOModel->UpdateValues();
}
