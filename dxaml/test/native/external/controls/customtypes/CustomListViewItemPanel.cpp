// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <pch.h>
#include <CustomListViewItemPanel.h>

namespace Tests { namespace External { namespace Controls { namespace CustomTypes {

    CustomListViewItemPanel::CustomListViewItemPanel()
    {
        textBlock = ref new Microsoft::UI::Xaml::Controls::TextBlock();
        textBlock->Text = "this is a very long sentence just cause longness is awesome";

        Children->Append(textBlock);
    }

    ::Windows::Foundation::Size CustomListViewItemPanel::MeasureOverride(::Windows::Foundation::Size availableSize)
    {
        __super::MeasureOverride(availableSize);

        textBlock->Measure(availableSize);

        return ::Windows::Foundation::Size(availableSize.Width, textBlock->DesiredSize.Height);
    }

    ::Windows::Foundation::Size CustomListViewItemPanel::ArrangeOverride(::Windows::Foundation::Size finalSize)
    {
        __super::ArrangeOverride(finalSize);

        textBlock->Arrange(::Windows::Foundation::Rect(12, 0, finalSize.Width - 12, 40));

        return ::Windows::Foundation::Size(finalSize.Width, textBlock->DesiredSize.Height);
    }
}}}}