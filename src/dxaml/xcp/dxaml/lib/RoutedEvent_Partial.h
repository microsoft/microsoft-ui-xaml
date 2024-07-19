// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the RoutedEvent, which is just a placeholder required for
//      compatibility with SL and WPF. The native representation is simply
//      a string.

#pragma once

namespace DirectUI
{
    PARTIAL_CLASS(RoutedEvent)
    {
        friend class CValueBoxer;
        friend class EventTrigger;

        public:
            RoutedEvent() : m_strNativeRepresentation()
            {
            }

            using ctl::WeakReferenceSource::Initialize;
            HRESULT Initialize(_In_ KnownEventIndex eventId, _In_ HSTRING hNativeRepresentation);

            KnownEventIndex GetEventId()
            {
                return m_EventId;
            }

        private:
            wrl_wrappers::HString m_strNativeRepresentation;
            KnownEventIndex m_EventId;
    };
}
