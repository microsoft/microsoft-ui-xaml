// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "NumberBoxTextBox.g.h"
//#include "NumberBoxTextBox.properties.h"

class NumberBoxTextBox :
    public ReferenceTracker<NumberBoxTextBox, winrt::implementation::NumberBoxTextBoxT>
    //public NumberBoxTextBoxProperties
{

public:
    NumberBoxTextBox();
    ~NumberBoxTextBox() {}

    // IFrameworkElement
    //void OnApplyTemplate();

    // IUIElement
    virtual winrt::AutomationPeer OnCreateAutomationPeer();
    
    //void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);


};
