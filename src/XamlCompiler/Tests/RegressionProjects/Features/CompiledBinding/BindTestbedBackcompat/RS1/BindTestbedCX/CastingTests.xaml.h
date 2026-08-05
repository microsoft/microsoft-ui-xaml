// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "CastingTests.g.h"

namespace BindTestbed
{
    public ref class CastingTestsVM sealed :
        public Microsoft::UI::Xaml::Data::INotifyPropertyChanged
    {
    public:

        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

        property Platform::String^ Username
        {
            Platform::String^ get();
            void set(Platform::String^ value);
        }

        property bool IsVisible
        {
            bool get();
            void set(bool value);
        }

        property Platform::IBox<bool>^ IsVisibleNullable
        {
            Platform::IBox<bool>^ get();
            void set(Platform::IBox<bool>^ value);
        }

        property Microsoft::UI::Xaml::Visibility VisibilityValue
        {
            Microsoft::UI::Xaml::Visibility get();
            void set(Microsoft::UI::Xaml::Visibility value);
        }

        property bool IsChecked
        {
            bool get();
            void set(bool value);
        }

        property double DoubleVal
        {
            double get();
            void set(double value);
        }

        property int IntVal
        {
            int get();
            void set(int value);
        }

        property Platform::String^ Prefix
        {
            Platform::String^ get();
        }

        property double Postfix
        {
            double get();
        }

        Platform::String^ CombineStringWithInt(Platform::String^ str, int number);

    private:

        void RaisePropertyChanged(Platform::String^ propertyName);

        Platform::String^ _username;
        bool _isVisible = false;
        Microsoft::UI::Xaml::Visibility _visibilityValue = Microsoft::UI::Xaml::Visibility::Collapsed;
        bool _isChecked = true;
        double _doubleVal = 15.0;
        int _intVal = 20;
    };

    public ref class CastingTests sealed
    {
    public:
        CastingTests();
    };
}