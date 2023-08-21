﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "IconSource.h"
#include "SymbolIconSource.h"

winrt::IconElement SymbolIconSource::CreateIconElementCore()
{
    winrt::SymbolIcon symbolIcon;
    symbolIcon.Symbol(Symbol());
    if (const auto newForeground = Foreground())
    {
        symbolIcon.Foreground(newForeground);
    }
    return symbolIcon;
}

winrt::DependencyProperty SymbolIconSource::GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty)
{
    if (sourceProperty == s_SymbolProperty)
    {
        return winrt::SymbolIcon::SymbolProperty();
    }

    return __super::GetIconElementPropertyCore(sourceProperty);
}
