// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DOCollectionUnitTests.h"

#include <DOCollection.h>
#include <CDependencyObject.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace DependencyObject {

void DOCollectionUnitTests::ValidateGetCollection()
{
    // CDOCollection::ValidateItem checks the type information. CDOCollection has no type index and will return
    // UnknownType_UnknownProperty, which hits an assert. Use CDependencyObjectCollection instead.
    CDependencyObjectCollection collection;

    auto& items = collection.GetCollection();
    VERIFY_ARE_EQUAL(static_cast<size_t>(0), items.size());

    xref_ptr<CDependencyObject> pObject1;
    pObject1.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject2;
    pObject2.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject3;
    pObject3.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject4;
    pObject4.attach(new CDependencyObject());

    pObject1->GetClassName();

    collection.push_back(pObject1);
    collection.push_back(pObject2);
    collection.push_back(pObject3);
    collection.push_back(pObject4);

    VERIFY_ARE_EQUAL(static_cast<size_t>(4), items.size());
    VERIFY_ARE_EQUAL(pObject1, items[0]);
    VERIFY_ARE_EQUAL(pObject2, items[1]);
    VERIFY_ARE_EQUAL(pObject3, items[2]);
    VERIFY_ARE_EQUAL(pObject4, items[3]);
}

void DOCollectionUnitTests::ValidateRemoveIfModifiesCollection()
{
    // CDOCollection::ValidateItem checks the type information. CDOCollection has no type index and will return
    // UnknownType_UnknownProperty, which hits an assert. Use CDependencyObjectCollection instead.
    CDependencyObjectCollection collection;

    xref_ptr<CDependencyObject> pObject1;
    pObject1.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject2;
    pObject2.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject3;
    pObject3.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject4;
    pObject4.attach(new CDependencyObject());

    collection.push_back(pObject1);
    collection.push_back(pObject2);
    collection.push_back(pObject3);
    collection.push_back(pObject4);

    collection.remove_if([&pObject1] (CDependencyObject* obj) { return obj == pObject1.get(); });

    VERIFY_ARE_EQUAL(collection.size(), static_cast<size_t>(3));
    VERIFY_ARE_NOT_EQUAL(collection[0], pObject1.get());
}

void DOCollectionUnitTests::ValidateRemoveModifiesCollection()
{
    // CDOCollection::ValidateItem checks the type information. CDOCollection has no type index and will return
    // UnknownType_UnknownProperty, which hits an assert. Use CDependencyObjectCollection instead.
    CDependencyObjectCollection collection;

    xref_ptr<CDependencyObject> pObject1;
    pObject1.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject2;
    pObject2.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject3;
    pObject3.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject4;
    pObject4.attach(new CDependencyObject());

    collection.push_back(pObject1);
    collection.push_back(pObject2);
    collection.push_back(pObject3);
    collection.push_back(pObject4);

    collection.remove(pObject2.get());

    VERIFY_ARE_EQUAL(collection.size(), static_cast<size_t>(3));
    VERIFY_ARE_NOT_EQUAL(collection[1], pObject2.get());
}

void DOCollectionUnitTests::ValidateClearModifiesCollection()
{
    // CDOCollection::ValidateItem checks the type information. CDOCollection has no type index and will return
    // UnknownType_UnknownProperty, which hits an assert. Use CDependencyObjectCollection instead.
    CDependencyObjectCollection collection;

    xref_ptr<CDependencyObject> pObject1;
    pObject1.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject2;
    pObject2.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject3;
    pObject3.attach(new CDependencyObject());

    xref_ptr<CDependencyObject> pObject4;
    pObject4.attach(new CDependencyObject());

    collection.push_back(pObject1);
    collection.push_back(pObject2);
    collection.push_back(pObject3);
    collection.push_back(pObject4);

    collection.clear();

    VERIFY_ARE_EQUAL(collection.size(), static_cast<size_t>(0));
}

void DOCollectionUnitTests::ValidateCDOCollectionReserve()
{
    CDOCollection collection;
    VERIFY_ARE_EQUAL(collection.Reserve(33), S_OK);
}

void DOCollectionUnitTests::ValidateCDOCollectionShouldAssociateChildren()
{
    CDOCollection collection;
    VERIFY_IS_TRUE(collection.ShouldAssociateChildren(nullptr));
}

} } } } } }
