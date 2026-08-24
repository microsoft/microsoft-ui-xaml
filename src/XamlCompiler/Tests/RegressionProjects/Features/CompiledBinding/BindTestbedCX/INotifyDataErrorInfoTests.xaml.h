// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// PageWithINotifyDataErrorInfo.xaml.h
// Declaration of the PageWithINotifyDataErrorInfo class
//

#pragma once

#include "INotifyDataErrorInfoTests.g.h"

namespace BindTestbedCX
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class INotifyDataErrorInfoTests sealed
	{
	public:
        INotifyDataErrorInfoTests();
        property BindTestbedModel::DataErrorModel^ ErrorModel;
        property BindTestbedModel::DODataErrorModel^ DOErrorModel;
	};
}
