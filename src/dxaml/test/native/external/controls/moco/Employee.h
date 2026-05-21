// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests {
    namespace Native {
        namespace External {
            namespace Controls {
                namespace MoCo {
                    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
                    [Microsoft::UI::Xaml::Data::BindableAttribute]
                    public ref class Employee sealed : Microsoft::UI::Xaml::Data::INotifyPropertyChanged
                    {
                    public:
                        Employee()
                        {
                            _isPropertyChangedObserved = false;
                        }

                        property Platform::String^ Name
                        {
                            Platform::String^ get()
                            {
                                return _name;
                            }
                            void set(Platform::String^ value)
                            {
                                _name = value;

                            }
                        }

                        virtual Platform::String^ ToString() override
                        {
                            return Name;
                        }

                        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged
                        {
                            virtual ::Windows::Foundation::EventRegistrationToken add(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ e)
                            {
                                _isPropertyChangedObserved = true;
                                return _privatePropertyChanged += e;
                            }
                            virtual void remove(::Windows::Foundation::EventRegistrationToken t)
                            {
                                _privatePropertyChanged -= t;
                            }

                        protected:
                            virtual void raise(Object^ sender, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs^ e)
                            {
                                if (_isPropertyChangedObserved)
                                {
                                    _privatePropertyChanged(sender, e);
                                }
                            }
                        }

                    private:
                        Platform::String^ _name;
                        bool _isPropertyChangedObserved;
                        event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ _privatePropertyChanged;
                    };

                    /*[::Windows::Foundation::Metadata::WebHostHiddenAttribute]
                    [Microsoft::UI::Xaml::Data::BindableAttribute]
                    public ref class EmployeeGroup sealed : MocoBasicGroupedDataModel
                    {
                    public:
                        property Platform::String^ Name
                        {
                            Platform::String^ get()
                            {
                                return _name;
                            }
                            void set(Platform::String^ value)
                            {
                                _name = value;
                            }
                        }

                        property ::Windows::Foundation::Collections::IObservableVector<Employee^>^ Items12
                        {
                            ::Windows::Foundation::Collections::IObservableVector<Employee^>^ get()
                            {
                                return _items;
                            }
                            void set(::Windows::Foundation::Collections::IObservableVector<Employee^>^ value)
                            {
                                _items = value;
                            }
                        }

                        virtual Platform::String^ ToString() override
                        {
                            return Name;
                        }

                        EmployeeGroup()
                        {
                            _isPropertyChangedObserved = false;
                        }

                        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged
                        {
                            virtual ::Windows::Foundation::EventRegistrationToken add(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ e)
                            {
                                _isPropertyChangedObserved = true;
                                return _privatePropertyChanged += e;
                            }
                            virtual void remove(::Windows::Foundation::EventRegistrationToken t)
                            {
                                _privatePropertyChanged -= t;
                            }

                        protected:
                            virtual void raise(Object^ sender, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs^ e)
                            {
                                if (_isPropertyChangedObserved)
                                {
                                    _privatePropertyChanged(sender, e);
                                }
                            }
                        }

                    protected:
                        void OnPropertyChanged(Platform::String^ propertyName)
                        {
                            if (_isPropertyChangedObserved)
                            {
                                PropertyChanged(this, ref new Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
                            }
                        }
                    private:
                        Platform::String^ _name;
                        ::Windows::Foundation::Collections::IObservableVector<Employee^>^ _items;
                        bool _isPropertyChangedObserved;
                        event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ _privatePropertyChanged;

                    };*/
                }
            }
        }
    }
}
