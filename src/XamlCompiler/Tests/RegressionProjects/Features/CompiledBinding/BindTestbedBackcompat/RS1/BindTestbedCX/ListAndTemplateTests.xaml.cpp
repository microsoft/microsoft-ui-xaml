// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// ListAndTemplateTests.xaml.cpp
// Implementation of the ListAndTemplateTests class.
//

#include "pch.h"
#include "ListAndTemplateTests.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbed;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Interop;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace ::Windows::UI::Popups;

ListAndTemplateTests::ListAndTemplateTests()
{
    InitializeComponent();
    DetectLeaksPage::TrackObject(this, ListAndTemplateTests::GetType()->FullName);
}

void ListAndTemplateTests::UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
        this->Model->UpdateValues();
        this->DOModel->UpdateValues();
        this->ModelCX->UpdateValues();

	BindTestbedModel::IEmployee^ temp = Employees->GetAt(0);
	Employees->RemoveAt(0);
	Employees->Append(temp);

}
void ListAndTemplateTests::ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->Model->InitializeValues();
	this->DOModel->UpdateValues();
	this->ModelCX->InitializeValues();
}