// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SwipeItem.g.h"
#include "SwipeItem.properties.h"

class SwipeItem :
    public ReferenceTracker<SwipeItem, winrt::implementation::SwipeItemT>,
    public SwipeItemProperties
{
public:
    SwipeItem();
    virtual ~SwipeItem();

    // Property changed handler.
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void GenerateControl(const winrt::AppBarButton& appBarButton, const winrt::Style& swipeItemStyle);

    void InvokeSwipe(const winrt::SwipeControl& content);

private:
    void OnItemTapped(
        const winrt::IInspectable& sender,
        const winrt::TappedRoutedEventArgs& args);
        
    void OnPointerPressed(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);

    void OnCommandChanged(const winrt::ICommand& oldCommand, const winrt::ICommand& newCommand);
    
    void AttachEventHandlers(const winrt::AppBarButton& appBarButton);
};
