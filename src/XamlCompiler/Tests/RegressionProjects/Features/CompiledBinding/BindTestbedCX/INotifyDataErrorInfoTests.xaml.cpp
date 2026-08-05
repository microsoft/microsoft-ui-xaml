// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// PageWithINotifyDataErrorInfo.xaml.cpp
// Implementation of the PageWithINotifyDataErrorInfo class
//

#include "pch.h"
#include "INotifyDataErrorInfoTests.xaml.h"

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

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

INotifyDataErrorInfoTests::INotifyDataErrorInfoTests()
{
	InitializeComponent();
    ErrorModel = ref new BindTestbedModel::DataErrorModel();
    DOErrorModel = ref new BindTestbedModel::DODataErrorModel();
}
