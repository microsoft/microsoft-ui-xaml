// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace Simple
{
    // This grid will have a non-DirectUIXamlType and is helping tests scenarios where,
    // we might make asumptions about types being all DUI
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class MyGrid sealed : Microsoft::UI::Xaml::Controls::Grid
    {
        public:
            MyGrid() {};
    };
}
