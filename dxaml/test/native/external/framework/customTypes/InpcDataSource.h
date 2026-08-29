// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DataSource.h"

namespace Tests { namespace Native { namespace External { namespace Framework {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class InpcDataSource sealed : DataSource, Microsoft::UI::Xaml::Data::INotifyPropertyChanged
    {
    public:
        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

        virtual void SetValue(Platform::Type^ type, Platform::Object^ value) override
        {
            DataSource::SetValue(type, value);
            RaisePropertyChanged(GetPropertyName(type));
        }

    private:
        void RaisePropertyChanged(Platform::String^ propertyName)
        {
            PropertyChanged(this, ref new Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
        }
    };

} } } }