// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RoutedEvent.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

HRESULT
RoutedEvent::Initialize(_In_ KnownEventIndex eventId, _In_ HSTRING hNativeRepresentation)
{
    HRESULT hr = S_OK;
    m_EventId = eventId;
    IFC(m_strNativeRepresentation.Set(hNativeRepresentation));
Cleanup:
    RRETURN(hr);
}
