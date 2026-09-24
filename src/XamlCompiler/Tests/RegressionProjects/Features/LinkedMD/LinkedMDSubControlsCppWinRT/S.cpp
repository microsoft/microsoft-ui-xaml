// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "S.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::LinkedMDSubControlsCppWinRT::implementation
{
    S::S()
    {
        InitializeComponent();
    }

    hstring S::StringPropertyOnS()
    {
        return L"";
    }

    ::winrt::LinkedMDSubControlsCppWinRT::T S::TPropertyOnS()
    {
        ::winrt::LinkedMDSubControlsCppWinRT::T t{ nullptr };
        return t;
    }
}
