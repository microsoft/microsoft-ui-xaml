// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// S.xaml.h
// Declaration of the S class
//

#pragma once

#include "S.g.h"
#include "T.xaml.h"

namespace SubControlsCX
{
	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class S sealed
	{
	public:
		S();
        property Platform::String^ StringPropertyOnS
        {
            Platform::String^ get()
            {
                return "";
            }
        }
        property T^ TPropertyOnS
        {
            T^ get()
            {
                return nullptr;
            }
        }
	};
}
