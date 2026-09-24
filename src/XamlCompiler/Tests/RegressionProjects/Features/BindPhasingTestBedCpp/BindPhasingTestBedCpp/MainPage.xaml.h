// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace BindPhasingTestBedCpp
{
    public ref class ExtraInfo sealed
    {
    public:
        property Platform::String^ Caption;

        ExtraInfo(Platform::String^ caption);
    };
    public ref class MyInfo sealed
    {
    public:
        property Platform::String^ ImageUrl;
        property Platform::String^ Caption;

        MyInfo(Platform::String^ imageUrl, Platform::String^ caption);
    };
    public ref class MyItem sealed : Microsoft::UI::Xaml::Data::INotifyPropertyChanged, public Microsoft::UI::Xaml::DependencyObject
    {
    public:
        property Platform::String^ Title;
        property Platform::String^ Subtitle;
        property Platform::String^ Description
        {
            Platform::String^ get()
            {
                return _description;
            }
            void set (Platform::String^ value)
            {
                if (value != _description)
                {
                    _description = value;
                    NotifyPropertyChanged("Description");
                }
            }
        };
        property MyInfo^ Info;
        property ExtraInfo^ OtherInfo;

        property ::Platform::String^ DPOnMyItem
        {
            ::Platform::String^ get()
            {
                return (::Platform::String^)GetValue(DPOnMyItemProperty);
            }
            void set(::Platform::String^ value)
            {
                SetValue(DPOnMyItemProperty, value);
            }
        }
        static property ::Microsoft::UI::Xaml::DependencyProperty^ DPOnMyItemProperty
        {
            ::Microsoft::UI::Xaml::DependencyProperty^ get()
            {
                return _DPOnMyItemProperty;
            }
            void set(::Microsoft::UI::Xaml::DependencyProperty^ value)
            {
                _DPOnMyItemProperty = value;
            }
        }
        MyItem(Platform::String^ title, Platform::String^ subtitle, Platform::String^ description, MyInfo^ info, ExtraInfo^ otherInfo, Platform::String^ dp);

        // Fired when properties change
        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

    private:
        static ::Microsoft::UI::Xaml::DependencyProperty^ _DPOnMyItemProperty;
        Platform::String^ _description;
        void NotifyPropertyChanged(Platform::String^ propertyName);
    };

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public ref class MainPage sealed
    {
    public:
        MainPage();
        void CreateTestItems();

    protected:
        virtual void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:
        Platform::Collections::Vector<MyItem^>^ myItems;

        void MyGridView_ContainerContentChanging(
            Microsoft::UI::Xaml::Controls::ListViewBase^ sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);

        void ShowTitle(
            Microsoft::UI::Xaml::Controls::ListViewBase^ sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);

        void ShowSubtitle(
            Microsoft::UI::Xaml::Controls::ListViewBase^ sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);

        void ShowDescription(
            Microsoft::UI::Xaml::Controls::ListViewBase^ sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);
    };
}
