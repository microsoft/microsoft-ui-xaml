// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AccessKeysOwnerUnitTests.h"
#include "AccessKeysOwner.h"
#include "AccessKey.h"
#include "UIAEnums.h"
#include <CxxMockTaef.h>
#include <Indexes.g.h>
#include <Mocks.h>

using namespace AccessKeys;

typedef AKOwner<MockCDO, MockPeer, MockProvider, MockProvider, MockProvider, MockProvider> AccessKeyOwner;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {
    
    void AKOwnerUnitTests::VerifyAKOwnerEqualityOperator()
    {
        AKAccessKey aKey1, aKey2, aKey3;
        aKey1 = L'S';
        aKey2 = L'S';
        aKey3 = L'Q';

        MockCDO element1;
        AccessKeyOwner akOwner1(&element1, aKey1);
        AccessKeyOwner akOwner2(&element1, aKey2);
        AccessKeyOwner akOwner3(&element1, aKey3);

        // Verify akOwner1 2 and 3 constructed as expected
        VERIFY_ARE_EQUAL(akOwner1.GetAccessKey(), aKey1);
        VERIFY_ARE_EQUAL(akOwner2.GetAccessKey(), aKey2);
        VERIFY_ARE_EQUAL(akOwner3.GetAccessKey(), aKey3);

        // AKOwner should be equal to itself
        VERIFY_ARE_EQUAL(akOwner1, akOwner1);
        VERIFY_ARE_EQUAL(akOwner2, akOwner2);
        VERIFY_ARE_EQUAL(akOwner3, akOwner3);

        // AKOwners with identical AccessKeys and owning elements should be equal
        VERIFY_ARE_EQUAL(akOwner1, akOwner2);

        VERIFY_ARE_NOT_EQUAL(akOwner1, akOwner3);
    }

    void AKOwnerUnitTests::InvokeFailedSetWhenNoPattern()
    {
        MockPeer dummyPeer;
        Expect(dummyPeer, GetPattern)
            .ReturnValue(nullptr);

        AKAccessKey aKey1;
        aKey1 = L'S';

        MockCDO element1;
        Expect(element1, GetAutomationPeer)
            .Once()
            .ReturnValue(&dummyPeer);
        Expect(element1, Release)
            .Once();
        Expect(element1, RaiseAccessKeyInvoked)
            .Once()
            .ReturnValue(false);
        Expect(element1, IsActive)
            .ReturnValue(true);

        AccessKeyOwner akOwner1(&element1, aKey1);

        // Call to Invoke should cause string to toggle from false to true due to the detoured mock logging implementation
        VERIFY_IS_FALSE(akOwner1.Invoke());
        VERIFY_EXPECTATIONS(element1);
    }

    void AKOwnerUnitTests::AutomationIsInvokedIfEventIsNotHandled()
    {
        MockProvider mockProvider;
        Expect(mockProvider, Invoke)
            .Once()
            .ReturnValue(true);

        MockUIAProvider mockUIAProvider;
        Expect(mockUIAProvider, GetPatternInterface)
            .ReturnValue(&mockProvider);

        MockPeer dummyPeer;
        Expect(dummyPeer, GetPattern)
            .ReturnValue(&mockUIAProvider);

        AKAccessKey aKey1;
        aKey1 = L'S';

        MockCDO element1;
        Expect(element1, GetAutomationPeer)
            .Once()
            .ReturnValue(nullptr);
        Expect(element1, OnCreateAutomationPeer)
            .Once()
            .ReturnValue(&dummyPeer);
        Expect(element1, Release)
            .Once();
        Expect(element1, RaiseAccessKeyInvoked)
            .Once()
            .ReturnValue(false);
        Expect(element1, IsActive)
            .ReturnValue(true);

        AccessKeyOwner akOwner1(&element1, aKey1);
        VERIFY_IS_TRUE(akOwner1.Invoke());
        VERIFY_EXPECTATIONS(element1);
    }

} } } } }
