// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// BindTests.xaml.h
// Declaration of the BindTests class
//

#pragma once

#include "BindTests.g.h"

namespace Conditionals 
{
    namespace SubFolder
    {
        using namespace ::Platform;
        using namespace ::Microsoft::UI::Xaml;
        using namespace ConditionalControls;

        /// <summary>
        /// An empty page that can be used on its own or navigated to within a Frame.
        /// </summary>
        [::Windows::Foundation::Metadata::WebHostHidden]
        public ref class BindTests sealed
            : public ::Microsoft::UI::Xaml::Data::INotifyPropertyChanged
        {
        public:
            BindTests();
            void Click_V2(Object^ sender, RoutedEventArgs^ e);
            void Click_V3(Object^ sender, RoutedEventArgs^ e);
            void NotifyPropertyChanged(String^ propertyName);

            // Inherited via INotifyPropertyChanged
            virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler ^ PropertyChanged;
            property String^ V2Property
            {
                String^ get();
                void set(String^ value);
            }

            property String^ V3Property
            {
                String^ get();
                void set(String^ value);
            }

            property Model^ Model;

            property String^ NullProperty;
            property IEmployee^ NullEmployee;
        };
    }
}
