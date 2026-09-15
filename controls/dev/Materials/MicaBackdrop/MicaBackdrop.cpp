// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MicaBackdrop.h"

void MicaBackdrop::OnTargetConnected(ICompositionSupportsSystemBackdrop target, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot)
{
    __super::OnTargetConnected(target, xamlRoot);
    throw winrt::hresult_not_implemented(
        L"MicaBackdrop is unavailable when WinUI uses the system composition stack.");
}

void MicaBackdrop::OnTargetDisconnected(ICompositionSupportsSystemBackdrop target)
{
    __super::OnTargetDisconnected(target);

}

void MicaBackdrop::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    const winrt::IDependencyProperty& property = args.Property();

    if (property == s_KindProperty)
    {
    }
}