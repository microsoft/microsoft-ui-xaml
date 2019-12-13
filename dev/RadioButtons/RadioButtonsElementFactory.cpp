// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "RadioButtonsElementFactory.h"

RadioButtonsElementFactory::RadioButtonsElementFactory()
{
}

winrt::UIElement RadioButtonsElementFactory::GetElementCore(const winrt::ElementFactoryGetArgs& args)
{
    if (auto const radioButton = args.Data().try_as<winrt::RadioButton>())
    {
        return radioButton;
    }
    else
    {
        auto const newRadioButton = winrt::RadioButton{};
        newRadioButton.Content(args.Data());
        return newRadioButton;
    }
}

void RadioButtonsElementFactory::RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args)
{

}
