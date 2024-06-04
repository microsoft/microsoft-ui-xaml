// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <depends.h>

class IFocusable
{

public:

    virtual KnownPropertyIndex GetElementSoundModePropertyIndex() = 0;
    virtual KnownPropertyIndex GetFocusStatePropertyIndex() = 0;
    virtual bool GetIsTabStop() = 0;
    virtual int GetTabIndex() = 0;

    virtual KnownPropertyIndex GetXYFocusDownPropertyIndex() = 0;
    virtual KnownPropertyIndex GetXYFocusDownNavigationStrategyPropertyIndex() = 0;
    virtual KnownPropertyIndex GetXYFocusLeftPropertyIndex() = 0;
    virtual KnownPropertyIndex GetXYFocusLeftNavigationStrategyPropertyIndex() = 0;
    virtual KnownPropertyIndex GetXYFocusRightPropertyIndex() = 0;
    virtual KnownPropertyIndex GetXYFocusRightNavigationStrategyPropertyIndex() = 0;
    virtual KnownPropertyIndex GetXYFocusUpPropertyIndex() = 0;
    virtual KnownPropertyIndex GetXYFocusUpNavigationStrategyPropertyIndex() = 0;

    virtual KnownEventIndex GetLostFocusEventIndex() = 0;
    virtual KnownEventIndex GetGotFocusEventIndex() = 0;

    virtual CDependencyObject* GetDOForIFocusable() = 0;
    virtual bool IsFocusable() = 0;

    IFocusable() {}
    virtual ~IFocusable() {}


};