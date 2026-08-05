// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace ConsumerCpp;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	_otherProviderLoaded = false;
	InitializeComponent();
}


void ConsumerCpp::MainPage::Button_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider^ ApplicationProvider = safe_cast<::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider^>(ConsumerCpp::App::Current);
	::Microsoft::UI::Xaml::Markup::IXamlType^ type = ApplicationProvider->GetXamlType("ProviderCpp.MainPage");
	if (!_otherProviderLoaded && type != nullptr)
	{
		throw ref new Exception(E_FAIL);
	}
	if (!_otherProviderLoaded)
	{
		::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider^ provider;
		provider = ref new ProviderCpp::ProviderCpp_XamlTypeInfo::XamlMetaDataProvider();
		ConsumerCpp::App^ a = safe_cast<ConsumerCpp::App^>(ConsumerCpp::App::Current);
		a->AddOtherProvider(provider);
		_otherProviderLoaded = true;
	}
	// Retry the operation
	type = ApplicationProvider->GetXamlType("ProviderCpp.MainPage");
	if (type == nullptr)
	{
		throw ref new Exception(E_FAIL);
	}

	ProviderCpp::MainPage::DoSomething();
	textBlock1->Text = ProviderCpp::MainPage::GetTextToShow();
}
