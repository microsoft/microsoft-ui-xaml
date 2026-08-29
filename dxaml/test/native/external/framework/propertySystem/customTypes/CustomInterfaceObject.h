// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace PropertySystem {

    public interface class ICustomInterface
    {
    };

    [Microsoft::UI::Xaml::Data::Bindable]
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class CustomInterfaceObject sealed : public Microsoft::UI::Xaml::DependencyObject, public ICustomInterface
    {
    };

} } } } }