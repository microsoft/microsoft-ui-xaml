// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// B.xaml.h
// Declaration of the B class
//

#pragma once

#include "B.g.h"

namespace ControlsCX
{
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class B sealed
	{
	public:
		B();
        property Platform::String^ StringPropertyOnB
        {
            Platform::String^ get()
            {
                return "";
            }
        }
	};
}
