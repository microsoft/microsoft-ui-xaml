// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#include "CastingTests.xaml.h"

using namespace Platform;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Data;

namespace BindTestbedCX
{
    #pragma region CastingTestsVM
    String^ CastingTestsVM::Username::get()
    {
        return _username;
    }
    void CastingTestsVM::Username::set(String^ value)
    {
        _username = value;
        RaisePropertyChanged("Username");
    }

    bool CastingTestsVM::IsVisible::get()
    {
        return _isVisible;
    }
    void CastingTestsVM::IsVisible::set(bool value)
    {
        _isVisible = value;
        RaisePropertyChanged("IsVisible");
    }

    IBox<bool>^ CastingTestsVM::IsVisibleNullable::get()
    {
        return ref new Box<bool>(_isVisible);
    }
    void CastingTestsVM::IsVisibleNullable::set(IBox<bool>^ value)
    {
        _isVisible = (nullptr != value) ? value->Value : false;
        RaisePropertyChanged("IsVisibleNullable");
    }

    Visibility CastingTestsVM::VisibilityValue::get()
    {
        return _visibilityValue;
    }
    void CastingTestsVM::VisibilityValue::set(Visibility value)
    {
        _visibilityValue = value;
        RaisePropertyChanged("VisibilityValue");
    }

    bool CastingTestsVM::IsChecked::get()
    {
        return _isChecked;
    }
    void CastingTestsVM::IsChecked::set(bool value)
    {
        _isChecked = value;
        RaisePropertyChanged("IsChecked");
    }

    double CastingTestsVM::DoubleVal::get()
    {
        return _doubleVal;
    }
    void CastingTestsVM::DoubleVal::set(double value)
    {
        _doubleVal = value;
        RaisePropertyChanged("DoubleVal");
    }

    int CastingTestsVM::IntVal::get()
    {
        return _intVal;
    }
    void CastingTestsVM::IntVal::set(int value)
    {
        _intVal = value;
        RaisePropertyChanged("IntVal");
    }

    String^ CastingTestsVM::Prefix::get()
    {
        return "Converting double to int.";
    }

    double CastingTestsVM::Postfix::get()
    {
        return 15.4;
    }

    String^ CastingTestsVM::CombineStringWithInt(String^ str, int number)
    {
        return str + number;
    }

    void CastingTestsVM::RaisePropertyChanged(String^ propertyName)
    {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
    }
    #pragma endregion

    CastingTests::CastingTests()
    {
        InitializeComponent();
    }
}