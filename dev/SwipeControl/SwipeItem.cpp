// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SwipeControl.h"
#include "SwipeItems.h"
#include "SwipeItemInvokedEventArgs.h"
#include "SwipeItem.h"
#include "RuntimeProfiler.h"
#include "CommandingHelpers.h"

// IconSource is implemented in WUX in the OS repo, so we don't need to
// include IconSource.h on that side.
#include "IconSource.h"

static const double s_swipeItemWidth = 68.0;
static const double s_swipeItemHeight = 60.0;

SwipeItem::SwipeItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SwipeItem);
}

#pragma endregion


void SwipeItem::InvokeSwipe(const winrt::SwipeControl& swipeControl)
{
    auto eventArgs = winrt::make_self<SwipeItemInvokedEventArgs>();
    eventArgs->SwipeControl(swipeControl);
    m_invokedEventSource(*this, *eventArgs);

    if (s_CommandProperty)
    {
        auto command = Command().as<winrt::ICommand>();
        auto param = CommandParameter();

        if (command && command.CanExecute(param))
        {
            command.Execute(param);
        }
    }
    // It stays open when onInvoked is expand.
    if (BehaviorOnInvoked() == winrt::SwipeBehaviorOnInvoked::Close ||
        BehaviorOnInvoked() == winrt::SwipeBehaviorOnInvoked::Auto)
    {
        swipeControl.Close();
    }
}

void SwipeItem::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.Property() == winrt::SwipeItem::CommandProperty())
    {
        OnCommandChanged(args.OldValue().as<winrt::ICommand>(), args.NewValue().as<winrt::ICommand>());
    }
}

void SwipeItem::OnCommandChanged(const winrt::ICommand& /*oldCommand*/, const winrt::ICommand& newCommand)
{
    if (auto newUICommand = newCommand.try_as<winrt::XamlUICommand>())
    {
        CommandingHelpers::BindToLabelPropertyIfUnset(newUICommand, *this, winrt::SwipeItem::TextProperty());
        CommandingHelpers::BindToIconSourcePropertyIfUnset(newUICommand, *this, winrt::SwipeItem::IconSourceProperty());
    }
}

void SwipeItem::GenerateControl(const winrt::AppBarButton& appBarButton, const winrt::Style& swipeItemStyle)
{
    appBarButton.Style(swipeItemStyle);
    if (Background())
    {
        appBarButton.Background(Background());
    }

    if (Foreground())
    {
        appBarButton.Foreground(Foreground());
    }

    if (auto const source = IconSource())
    {
        appBarButton.Icon(SharedHelpers::MakeIconElementFrom(source));
    }

    appBarButton.Label(Text());
    AttachEventHandlers(appBarButton);
}


void SwipeItem::AttachEventHandlers(const winrt::AppBarButton& appBarButton)
{
    auto weakThis = get_weak();
    appBarButton.Tapped({ [weakThis](auto& sender, auto& args) {
        if (auto temp = weakThis.get()) temp->OnItemTapped(sender, args);
    } });
    appBarButton.PointerPressed({ [weakThis](auto& sender, auto& args) {
        if (auto temp = weakThis.get()) temp->OnPointerPressed(sender, args);
    } });
}

void SwipeItem::OnItemTapped(
    const winrt::IInspectable& sender,
    const winrt::TappedRoutedEventArgs& args)
{
    auto current = winrt::VisualTreeHelper::GetParent(sender.try_as<winrt::DependencyObject>());
    while (current)
    {
        auto control = current.try_as<winrt::SwipeControl>();
        if (control)
        {
            InvokeSwipe(control);
            args.Handled(true);
        }
        current = winrt::VisualTreeHelper::GetParent(current);
    }
}

void SwipeItem::OnPointerPressed(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    if (args.Pointer().PointerDeviceType() == winrt::Devices::Input::PointerDeviceType::Touch)
    {
        // if we press an item, we want to handle it and not let the parent SwipeControl receive the input
        args.Handled(true);
    }
}
