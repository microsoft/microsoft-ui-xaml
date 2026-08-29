// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <collection.h>
#include <type_traits>

namespace Tests { namespace Native { namespace External { namespace Framework {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class DataStructureHolder sealed
    {
    public:
        property ::Windows::Foundation::Collections::IObservableVector<int>^ Collection;
        property ::Windows::Foundation::Collections::IObservableVector<Object^>^ ObjectCollection;
    };

} } } }
