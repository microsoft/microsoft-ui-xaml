// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// T.xaml.h
// Declaration of the T class
//

#pragma once

#include "T.g.h"

namespace SubControlsCX
{
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class T sealed
	{
	public:
		T();
        property Platform::String^ StringPropertyOnT
        {
            Platform::String^ get()
            {
                return "";
            }
        }
	};
}
