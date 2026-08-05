// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the BindTests class.
//

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "BindTests.g.h"

namespace winrt::ConditionalsCppWinRT::implementation
{
    struct BindTests : BindTestsT<BindTests>
    {
        BindTests();

        ConditionalControls::Model Model() { return model; }
        void Model(ConditionalControls::Model value) { model = value; }

        ConditionalControls::IEmployee NullEmployee() { return nullemployee; }
        void NullEmployee(ConditionalControls::IEmployee value) { nullemployee = value; }

        hstring NullProperty() { return nullproperty; }
        void NullProperty(hstring value) { nullproperty = value; }

        hstring V2Property() { return aButton().V2Property(); }
        void V2Property(hstring value)
        {
            if (aButton().V2Property() != value)
            {
                aButton().V2Property(value);
                RaisePropertyChanged(L"V2Property");
            }
        }

        hstring V3Property() { return aButton().V3Property(); }
        void V3Property(hstring value)
        {
            if (aButton().V3Property() != value)
            {
                aButton().V3Property(value);
                RaisePropertyChanged(L"V3Property");
            }
        }

        void Click_V2(::Windows::Foundation::IInspectable const& sender, wux::RoutedEventArgs const& e);
        void Click_V3(::Windows::Foundation::IInspectable const& sender, wux::RoutedEventArgs const& e);

        // Inherited via INotifyPropertyChanged

        void PropertyChanged(event_token const token)
        {
            propertyChanged.remove(token);
        }
        event_token PropertyChanged(wux::Data::PropertyChangedEventHandler const& handler)
        {
            return propertyChanged.add(handler);
        }

    private:
        event<wux::Data::PropertyChangedEventHandler> propertyChanged;
        void RaisePropertyChanged(hstring const& propertyName)
        {
            propertyChanged(*this, wux::Data::PropertyChangedEventArgs(propertyName));
        }

        hstring nullproperty = L"";
        ConditionalControls::Model model = nullptr;
        ConditionalControls::IEmployee nullemployee = nullptr;
    };
}

namespace winrt::ConditionalsCppWinRT::factory_implementation
{
    struct BindTests : BindTestsT<BindTests, implementation::BindTests>
    {
    };
}
