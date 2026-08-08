// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// A.xaml.h
// Declaration of the A class
//

#pragma once

#include "A.g.h"
#include "B.xaml.h"

namespace ControlsCX
{
    [::Windows::Foundation::Metadata::WebHostHidden]
	public ref class A sealed
	{
	public:
		A();
        property Platform::String^ StringPropertyOnA
        {
            Platform::String^ get()
            {
                return "";
            }
        }
        property B^ BPropertyOnA
        {
            B^ get()
            {
                return nullptr;
            }
        }
	};
}
