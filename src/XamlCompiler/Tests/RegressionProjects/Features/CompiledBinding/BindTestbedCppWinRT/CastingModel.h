// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/windows.foundation.h"
#include "winrt/Microsoft.UI.Xaml.data.h"
#include "CastingModel.g.h"

namespace winrt::BindTestbed::implementation
{
    struct CastingModel : CastingModelT<CastingModel, 
        wux::Data::INotifyPropertyChanged,
        wf::IStringable>
    {
        hstring ToString()
        {
            return Username();
        }

        hstring Username()
        {
            return username;
        }
        void Username(hstring const& value)
        {
            if (username != value)
            {
                username = value;
                RaisePropertyChanged(L"Username");
            }
        }

        bool IsVisible()
        {
            return isVisible;
        }
        void IsVisible(bool value)
        {
            if (isVisible != value)
            {
                isVisible = value;
                RaisePropertyChanged(L"IsVisible");
            }
        }

        wf::IReference<bool> IsVisibleNullable()
        {
            return wf::IReference<bool>(isVisible);
        }
        void IsVisibleNullable(wf::IReference<bool> const& value)
        {
            isVisible = value.Value();
            RaisePropertyChanged(L"IsVisibleNullable");
        }

        wux::Visibility VisibilityValue()
        {
            return visibilityValue;
        }
        void VisibilityValue(wux::Visibility const& value)
        {
            if (visibilityValue != value)
            {
                visibilityValue = value;
                RaisePropertyChanged(L"VisibilityValue");
            }
        }

        bool IsChecked()
        {
            return isChecked;
        }
        void IsChecked(bool const& value)
        {
            if (isChecked != value)
            {
                isChecked = value;
                RaisePropertyChanged(L"IsChecked");
            }
        }

        double DoubleVal()
        {
            return doubleVal;
        }
        void DoubleVal(double const& value)
        {
            if (doubleVal != value)
            {
                doubleVal = value;
                RaisePropertyChanged(L"DoubleVal");
            }
        }

        int IntVal()
        {
            return intVal;
        }
        void IntVal(int value)
        {
            if (intVal != value)
            {
                intVal = value;
                RaisePropertyChanged(L"IntVal");
            }
        }

        hstring Prefix()
        {
            return L"Converting double to int.";
        }

        double Postfix()
        {
            return 15.4;
        }

        hstring CombineStringWithInt(hstring str, int number)
        {
            wchar_t n[20];
            std::wstring s = str.c_str();
            _itow_s(number, n, 10);
            return s.append(n).c_str();
        }

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

        hstring username;
        bool isVisible = false;
        wux::Visibility visibilityValue = wux::Visibility::Collapsed;
        bool isChecked = true;
        double doubleVal = 15.0;
        int intVal = 20;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct CastingModel : CastingModelT<CastingModel, implementation::CastingModel>
    {
    };
}
