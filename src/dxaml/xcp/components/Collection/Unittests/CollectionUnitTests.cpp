// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DoCollection.h>
#include "CollectionUnitTests.h"
#include <XamlLogging.h>



namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Collection {

    void CollectionUnitTests::ValidateCCollectionReserve()
    {
        auto pDoCollection = make_xref<CDOCollection>();
        auto pCollection = static_cast<CCollection*>(pDoCollection);

        VERIFY_ARE_EQUAL(pCollection->Reserve(33), S_OK);
    }


} } } } }