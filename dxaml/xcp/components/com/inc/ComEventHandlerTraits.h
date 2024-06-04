// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace ctl
{
    namespace Internal
    {
        // This class can be used as the type argument to EventPtr<T>
        class EventPtr_compatible_tag
        { };
        
        // This class can be used as the type argument to WeakEventPtr<T>
        class WeakEventPtr_compatible_tag
        { };
    }

    // This trait will return true in its static member value if T is an (tokenless_)event_handler
    template <typename T>
    struct IsEventPtrCompatible
    {
        static const bool value = std::is_base_of<Internal::EventPtr_compatible_tag, T>::value;
    };
    
    // This trait will return true in its static member value if T is an weak_event_handler
    template <typename T>
    struct IsWeakEventPtrCompatible
    {
        static const bool value = std::is_base_of<Internal::WeakEventPtr_compatible_tag, T>::value;
    };
}
