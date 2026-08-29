// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {

class AKOwnerUnitTests : public WEX::TestClass<AKOwnerUnitTests>
{
public:
    BEGIN_TEST_CLASS(AKOwnerUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(VerifyAKOwnerEqualityOperator)
        TEST_METHOD_PROPERTY(L"Description",L"Verify the equality (==) operator for AKOwner")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InvokeFailedSetWhenNoPattern)
        TEST_METHOD_PROPERTY(L"Description", L"Verify that we properly mark invokeFailed when we do not see a pattern")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AutomationIsInvokedIfEventIsNotHandled)
        TEST_METHOD_PROPERTY(L"Description", L"Automation Peer is invoked if the event is not handled")
    END_TEST_METHOD()
};

} } } } }
