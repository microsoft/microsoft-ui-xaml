// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {

class AKScopeUnitTests : public WEX::TestClass<AKScopeUnitTests>
{
public:
    BEGIN_TEST_CLASS(AKScopeUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(VerifyCanInvokeCorrectAKOwner)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that AKScope invokes the correct AKOwner given an AccessKey.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyCanInvokeCorrectAKOwnerByFiltering)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that AKScope invokes the correct AKOwner given a filtering sequence.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyCanSupressFiltering)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that AKScope can supress filtering for cases like HotKey invocation.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyCorrectCharactersFiltered)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that AKScope can correctly passes along a filtered string.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyInvokeProperlyHandlesSurrogatePairs)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that AKScope handles surrogate pairs properly when being invoked.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyEscapeKeyFilteringBehavior)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that AKScope can correctly pops the last character being filtered when handling an escape key.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyEscapeKeyFilteringBehaviorWithSurrogatePairs)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that AKScope handles surrogate pairs properly when handling an escape key.")
    END_TEST_METHOD()
};

} } } } }
