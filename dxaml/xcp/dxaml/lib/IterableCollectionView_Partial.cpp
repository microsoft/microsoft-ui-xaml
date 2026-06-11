// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IterableCollectionView.g.h"

using namespace DirectUI;
using namespace xaml_data;

_Check_return_
HRESULT IterableCollectionView::CreateInstance(
    _In_ wfc::IIterable<IInspectable *> *pSource,
    _Outptr_ xaml_data::ICollectionView **instance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ReadOnlyTrackerCollection<IInspectable *>> spVector;
    ctl::ComPtr<IterableCollectionView> spCV;

    IFCPTR(pSource);
    IFCPTR(instance);

    IFC(ctl::make<ReadOnlyTrackerCollection<IInspectable *>>(&spVector));
    IFC(InitializeReadOnlyCollectionFromIterable(spVector.Get(), pSource));

    // Now that we have the vector we can just use the derived implementation
    // to work over it
    IFC(ctl::make<IterableCollectionView>(&spCV));
    IFC(spCV->SetSource(spVector.Get()));

    // Return the new instance to the caller
    *instance = spCV.Detach();

Cleanup:

    RRETURN(hr);
}



