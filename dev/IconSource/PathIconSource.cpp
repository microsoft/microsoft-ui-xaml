// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "IconSource.h"
#include "PathIconSource.h"

winrt::IconElement PathIconSource::CreateIconElementCore()
{
    winrt::PathIcon pathIcon;

    if (auto const data = Data())
    {
        pathIcon.Data(data);
    }
    if (const auto newForeground = Foreground())
    {
        pathIcon.Foreground(newForeground);
    }
    return pathIcon;
}
