// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace DependencyObject {

class DOCollectionUnitTests : public WEX::TestClass<DOCollectionUnitTests>
{
public:
    BEGIN_TEST_CLASS(DOCollectionUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(ValidateGetCollection)
        TEST_METHOD_PROPERTY(L"Description", L"Tests retrieving the collection from a DOCollection.")
    END_TEST_METHOD()

    TEST_METHOD(ValidateRemoveIfModifiesCollection)

    TEST_METHOD(ValidateRemoveModifiesCollection)

    TEST_METHOD(ValidateClearModifiesCollection)

    BEGIN_TEST_METHOD(ValidateCDOCollectionReserve)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that CDoCollection::Reserve works as expected.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValidateCDOCollectionShouldAssociateChildren)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that CDoCollection::ShouldAssociateChildren works as expected.")
    END_TEST_METHOD()
};

}}} } } }
