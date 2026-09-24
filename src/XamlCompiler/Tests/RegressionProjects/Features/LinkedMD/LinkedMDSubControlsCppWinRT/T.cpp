// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "T.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::LinkedMDSubControlsCppWinRT::implementation
{
    T::T()
    {
        InitializeComponent();
    }

    hstring T::StringPropertyOnT()
    {
        return L"";
    }
}
