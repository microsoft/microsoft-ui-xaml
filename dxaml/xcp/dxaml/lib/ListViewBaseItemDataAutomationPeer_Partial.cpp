// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBaseItemDataAutomationPeer_partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the ListViewBaseItemDataAutomationPeer class.
ListViewBaseItemDataAutomationPeer::ListViewBaseItemDataAutomationPeer()
{
}

// Deconstructor
ListViewBaseItemDataAutomationPeer::~ListViewBaseItemDataAutomationPeer()
{
}

IFACEMETHODIMP ListViewBaseItemDataAutomationPeer::Realize()
{
    HRESULT hr = S_OK;

    IFC(ScrollIntoViewCommon());

Cleanup:
    RRETURN(hr);
}
