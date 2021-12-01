// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BreadcrumbBarItemClickedEventArgs.g.h"

class BreadcrumbBarItemClickedEventArgs :
    public winrt::implementation::BreadcrumbBarItemClickedEventArgsT<BreadcrumbBarItemClickedEventArgs>
{
public:
    winrt::IInspectable Item();
    void Item(const winrt::IInspectable& item);

    int Index();
    void Index(const int& index);

private:
    int m_index{ -1 };
    winrt::IInspectable m_item{ };
};
