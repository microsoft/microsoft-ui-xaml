// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    template<typename EventSender, typename EventArgs>
    struct _EventHandlerInfo
    {
        typedef EventSender Sender;
        typedef EventArgs Args;
    };
    template<typename THandler> struct EventHandlerInfo {};

    #define MAP_EVENT_ARGS(eventhandler, eventargs) \
        template<> struct EventHandlerInfo<eventhandler> : _EventHandlerInfo<Platform::Object, eventargs> {};

    #define MAP_TYPEDEVENT(eventSender, eventargs) \
        template<> struct EventHandlerInfo<wf::TypedEventHandler<eventSender^, eventargs^>> : _EventHandlerInfo<eventSender, eventargs> {};

    MAP_EVENT_ARGS(xaml::RoutedEventHandler                , xaml::RoutedEventArgs);
    MAP_EVENT_ARGS(xaml_input::PointerEventHandler         , xaml_input::PointerRoutedEventArgs);
    MAP_EVENT_ARGS(xaml_controls::TextChangedEventHandler  , xaml_controls::TextChangedEventArgs);

} } } } } // namespace Microsoft::UI::Xaml::Tests::Common
