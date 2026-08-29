// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ValueConverters.h"
namespace Tests { namespace Tools { namespace Shared {
        Platform::Object^ IconConverter::Convert(Platform::Object^ value, ::Windows::UI::Xaml::Interop::TypeName, Platform::Object^, Platform::String^)
        {
            auto valString = safe_cast<Platform::String^>(value);

            if (valString == "Smile")
            {
                return ref new Platform::String(L"");
            }

            return nullptr;
        }

        Platform::Object^ IconConverter::ConvertBack(Platform::Object^ value, ::Windows::UI::Xaml::Interop::TypeName, Platform::Object^, Platform::String^)
        {
            return nullptr;
        }
} } }