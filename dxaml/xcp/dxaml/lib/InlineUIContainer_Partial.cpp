// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InlineUIContainer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;



//-----------------------------------------------------------------------------
//
//  OnDisconnectVisualChildren
//
//  During a DisconnectVisualChildrenRecursive tree walk, also clear the Child property.
//
//-----------------------------------------------------------------------------

IFACEMETHODIMP
InlineUIContainer::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;

    IFC(ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::InlineUIContainer_Child)));

    IFC(InlineUIContainerGenerated::OnDisconnectVisualChildren());

Cleanup:
    RRETURN(hr);
}




