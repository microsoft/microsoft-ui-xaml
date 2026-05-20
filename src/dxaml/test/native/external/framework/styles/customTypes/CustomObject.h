// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Styles {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::Bindable]
    public ref class CustomObject sealed
    {
    public:
        CustomObject();
        virtual ~CustomObject();
        static unsigned int GetInstanceCount();
        static void ClearInstanceCount();
    private:
        static unsigned int InstanceCount;
    };
} } } } }