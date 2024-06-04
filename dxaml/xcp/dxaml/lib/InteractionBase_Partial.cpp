// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "InteractionBase.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT InteractionBase::GetSupportedEventsImpl(_Outptr_ wfc::IVectorView<xaml::RoutedEvent*>** returnVal)
{
    IFC_RETURN(GetSupportedEventsCoreProtected(returnVal));
    return S_OK;
}
