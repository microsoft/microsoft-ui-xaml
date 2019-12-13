// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "common.h"
#include "ElementFactory.h"

class RadioButtonsElementFactory :
    public winrt::implements<RadioButtonsElementFactory, ElementFactory>
{
public:
    RadioButtonsElementFactory();
    
    winrt::UIElement GetElementCore(const winrt::ElementFactoryGetArgs& args) override;
    void RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args) override;
private:
};

