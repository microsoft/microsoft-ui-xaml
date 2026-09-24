// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MyUserControl.xaml.h
// Declaration of the MyUserControl class
//

#pragma once

#include "MyUserControl.g.h"

namespace EventHandling_968976
{
    public delegate void MyFirstEvent(const Platform::Array<unsigned int>^ args);
    public delegate void MySecondEvent(Platform::WriteOnlyArray<unsigned int>^ args);
    public delegate void MyThirdEvent(Platform::Array<unsigned int>^* args);
    public delegate void MyFourthEvent(Platform::String^* args);

    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class MyUserControl sealed
    {
    public:
        MyUserControl();
        event MyFirstEvent^ TheFirstEvent;
        event MySecondEvent^ TheSecondEvent;
        event MyThirdEvent^ TheThirdEvent;
        event MyFourthEvent^ TheFourthEvent;
    };
}
