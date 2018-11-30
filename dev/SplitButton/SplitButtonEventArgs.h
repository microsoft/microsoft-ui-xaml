// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitButtonClickEventArgs.g.h"
#include "ToggleSplitButtonIsCheckedChangedEventArgs.g.h"

class SplitButtonClickEventArgs :
    public winrt::implementation::SplitButtonClickEventArgsT<SplitButtonClickEventArgs>
{
public:
    SplitButtonClickEventArgs() {}
};

class ToggleSplitButtonIsCheckedChangedEventArgs :
    public winrt::implementation::ToggleSplitButtonIsCheckedChangedEventArgsT<ToggleSplitButtonIsCheckedChangedEventArgs>
{
public:
    ToggleSplitButtonIsCheckedChangedEventArgs() {}
};

