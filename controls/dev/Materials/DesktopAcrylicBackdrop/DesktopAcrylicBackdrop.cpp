// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "DesktopAcrylicBackdrop.h"

#include "DesktopAcrylicBackdrop.properties.cpp"

void DesktopAcrylicBackdrop::OnTargetConnected(ICompositionSupportsSystemBackdrop target, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot)
{
    __super::OnTargetConnected(target, xamlRoot);
    throw winrt::hresult_not_implemented(
        L"DesktopAcrylicBackdrop is unavailable when WinUI uses the system composition stack.");
}

void DesktopAcrylicBackdrop::OnTargetDisconnected(ICompositionSupportsSystemBackdrop target)
{
    __super::OnTargetDisconnected(target);

}