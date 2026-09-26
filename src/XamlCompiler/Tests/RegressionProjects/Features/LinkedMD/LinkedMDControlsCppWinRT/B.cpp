// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "B.h"
#include "B.g.cpp"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::LinkedMDControlsCppWinRT::implementation
{
    B::B()
    {
        InitializeComponent();
    }

    hstring B::StringPropertyOnB()
    {
        return L"";
    };
}
