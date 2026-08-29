// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <CustomPropertySupport.h>

namespace Tests { namespace Native { namespace External { namespace Controls { namespace Helpers {

    /*
     * A data object class without a ToString implementation, used to exercise
     * AutomationProperties.Name resolutions when it isn't set and the obvious
     * sources are not available.
     */
    ref class DataWithoutToString sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase {
    public:
        DataWithoutToString() {}
        DataWithoutToString(Platform::String^ name)
        {
            this->Name = name;
        }

    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"Name", Platform::String::typeid,
                MAKEPROPGET(DataWithoutToString^, Name),
                MAKEPROPSET(DataWithoutToString^, Name, Platform::String^)
                );
        }

    public:
        property Platform::String^ Name;
    };
} } } } }
