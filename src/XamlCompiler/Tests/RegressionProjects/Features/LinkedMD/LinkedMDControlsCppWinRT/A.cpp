// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "A.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::LinkedMDControlsCppWinRT::implementation
{
    A::A()
    {
        InitializeComponent();
    }

    hstring A::StringPropertyOnA()
    {
        return L"";
    }

    ::winrt::LinkedMDControlsCppWinRT::B A::BPropertyOnA()
    {
        winrt::LinkedMDControlsCppWinRT::B b{ nullptr };
        return b;
    }
}
