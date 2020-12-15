// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BreadcrumbItemClickedEventArgs.g.h"

class BreadcrumbItemClickedEventArgs :
    public winrt::implementation::BreadcrumbItemClickedEventArgsT<BreadcrumbItemClickedEventArgs>
{
public:
    winrt::IInspectable Item();
    void Item(const winrt::IInspectable& reason);

private:
    winrt::IInspectable m_item{ };
};
