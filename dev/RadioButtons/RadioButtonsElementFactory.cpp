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
        // The current default behavior of the RadioButton control is that multiple RadioButton controls with the _same_
        // visual parent will be grouped together. As a consequence, if a RadioButton is to be selected while another RadioButton
        // in the same group is already selected, logic is automatically run to first deselect the currently selected RadioButton
        // and then select the desired RadioButton.
        //
        // As seen in issue https://github.com/microsoft/microsoft-ui-xaml/issues/3128 this behavior leads to a scenario where the
        // RadioButtons control raises _two_ SelectionChanged events with split up selection changed information (deselected item
        // in first event, newly selected item in second event). However, the developer expects the event to be raised only _once_
        // per selection change and carrying the complete selection changed information.
        //
        // Thus, in order for our RadioButtons control to manage selection changes entirely on its own, we wrap each of its RadioButton
        // controls into a Grid as its visual parent. That way, no two of its RadioButtons will have the same visual parent and
        // selecting a RadioButton while another one is already selected no longer raises the RadioButton::Unchecked event automatically.
        auto const parentGrid = winrt::Grid{};
        parentGrid.Children().Append(radioButton);
        return parentGrid;
    }
    else
    {
        auto const newRadioButton = winrt::RadioButton{};
        newRadioButton.Content(args.Data());

        // See the comment above as to why we wrap the RadioButton in a Grid.
        auto const parentGrid = winrt::Grid{};
        parentGrid.Children().Append(newRadioButton);
        return parentGrid;
    }
}

void RadioButtonsElementFactory::RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args)
{
    // A UIElement can only have at most one parent at any given time. Since we wrap a RadioButton in a Grid in GetElementCore()
    // and the same RadioButton might be added to the RadioButtons control again, we need to remove the added parent Grid
    // from the RadioButton control here.
    if (auto const grid = args.Element().try_as<winrt::Grid>())
    {
        if (grid.Children().Size() == 1)
        {
            grid.Children().RemoveAt(0);
        }
    }
}
