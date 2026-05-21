// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScopeUnitTests.h"
#include "Scope.h"
#include "AccessKey.h"
#include "CxxMockTaef.h"
#include <Mocks.h>
#include <TypeTableStructs.h>

using namespace AccessKeys;

typedef AKScope<MockCDO, MockAKOwner> AccessKeyScope;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {


    void AKScopeUnitTests::VerifyCanInvokeCorrectAKOwner()
    {
        MockCDO parent, element1, element2, element3;

        AKAccessKey aKey1, aKey2, aKey3;
        aKey1 = L'W';
        aKey2 = L'I';
        aKey3 = L'N';

        std::vector<std::pair<MockCDO*, AKAccessKey>> initList;
        initList.push_back(std::make_pair(&element1, aKey1));
        initList.push_back(std::make_pair(&element2, aKey2));
        initList.push_back(std::make_pair(&element3, aKey3));

        AccessKeyScope myScope(&parent, initList);

        Expect(*element1.m_pOwner, Invoke)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element3.m_pOwner, Invoke)
            .Once()
            .ReturnValue(S_OK);

        // Try to invoke a key that is not in the scope e.g. 'T'.  Should expect invoked to be false for all elements
        AKInvokeReturnParams<MockCDO> invokeResult;
        VERIFY_SUCCEEDED(myScope.Invoke(L'T', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Invoke a key that is in Scope e.g. 'W'.
        VERIFY_SUCCEEDED(myScope.Invoke(L'W', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        // Invoke another key that is in Scope e.g. 'N'
        VERIFY_SUCCEEDED(myScope.Invoke(L'N', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        invokeResult.invokedElement.reset(); // Reset the invoked Element back to null.

        // Invoke another key that is not in Scope e.g. 'P'.  Invoked state should not change.
        VERIFY_SUCCEEDED(myScope.Invoke(L'P', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        VERIFY_EXPECTATIONS(element1);
        VERIFY_EXPECTATIONS(element2);
        VERIFY_EXPECTATIONS(element3);

        VERIFY_EXPECTATIONS(*element1.m_pOwner);
        VERIFY_EXPECTATIONS(*element2.m_pOwner);
        VERIFY_EXPECTATIONS(*element3.m_pOwner);
    }

    void AKScopeUnitTests::VerifyCanInvokeCorrectAKOwnerByFiltering()
    {
        MockCDO parent, element1, element2, element3;

        AKAccessKey aKey1(std::wstring(L"Q")),
                    aKey2(std::wstring(L"A1")),
                    aKey3(std::wstring(L"A2"));

        std::vector<std::pair<MockCDO*, AKAccessKey>> initList;
        initList.push_back(std::make_pair(&element1, aKey1));
        initList.push_back(std::make_pair(&element2, aKey2));
        initList.push_back(std::make_pair(&element3, aKey3));

        AccessKeyScope myScope(&parent, initList);

        Expect(*element1.m_pOwner, HideAccessKey)
            .Twice()
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, ShowAccessKey)
            .With(CxxMock::StringEquals(L""))
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element2.m_pOwner, ShowAccessKey)
            .CallCount(CxxMock::eq(2))
            .ReturnValue(S_OK);
        Expect(*element2.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element3.m_pOwner, ShowAccessKey)
            .CallCount(CxxMock::eq(2))
            .ReturnValue(S_OK);
        Expect(*element3.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        AKInvokeReturnParams<MockCDO> invokeResult;
        // Try to invoke a key that is not in the scope e.g. 'T'.  Should expect invoked to be false
        VERIFY_SUCCEEDED(myScope.Invoke(L'T', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);


        // Invoke a key that is in Scope e.g. 'A'.  Because there are multiple that start with A, should start filtering
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);  // ShowAccessKey called on A1,A2.
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Invoke another key that completes filtering
        VERIFY_SUCCEEDED(myScope.Invoke(L'1', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);  // Because this is a filtering match, will not call ShowAccessKey on Q
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        invokeResult.invokedElement.reset();
        // Because filtering is complete, a Q should directly invoke
        VERIFY_SUCCEEDED(myScope.Invoke(L'Q', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        invokeResult.invokedElement.reset();
        // Now I'll test Filtering behavior when I incorrectly input a key sequence during filtering.
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted); // ShowAccessKey called on A1,A2.
        VERIFY_IS_NULL(invokeResult.invokedElement);
        invokeResult.invokedElement.reset();

        VERIFY_SUCCEEDED(myScope.Invoke(L'3', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);
        // 3 doesn't reset filtering, so entering 2 should invoke.
        VERIFY_SUCCEEDED(myScope.Invoke(L'2', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        VERIFY_EXPECTATIONS(element1);
        VERIFY_EXPECTATIONS(element2);
        VERIFY_EXPECTATIONS(element3);

        VERIFY_EXPECTATIONS(*element1.m_pOwner);
        VERIFY_EXPECTATIONS(*element2.m_pOwner);
        VERIFY_EXPECTATIONS(*element3.m_pOwner);
    }

    void AKScopeUnitTests::VerifyCanSupressFiltering()
    {
        MockCDO parent, element1, element2, element3;

        AKAccessKey aKey1(std::wstring(L"Q")),
            aKey2(std::wstring(L"A1")),
            aKey3(std::wstring(L"A2"));

        std::vector<std::pair<MockCDO*, AKAccessKey>> initList;
        initList.push_back(std::make_pair(&element1, aKey1));
        initList.push_back(std::make_pair(&element2, aKey2));
        initList.push_back(std::make_pair(&element3, aKey3));

        AccessKeyScope myScope(&parent, initList);

        Expect(*element1.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element2.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element3.m_pOwner, Invoke)
            .ReturnValue(S_OK);


        AKInvokeReturnParams<MockCDO> invokeResult;
        // Try to invoke a key that is not in the scope e.g. 'T'.  Should expect invoked to be false
        VERIFY_SUCCEEDED(myScope.Invoke(L'T', false, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Invoke a key that is in Scope e.g. 'A'.  Because there are multiple that start with A, should not start filtering, nor invoke
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', false, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Invoke another key that would complete filtering.  The filtering feature is supressed, so no invocation should occur.
        VERIFY_SUCCEEDED(myScope.Invoke(L'1', false, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // A Q should directly invoke, no filtering.
        VERIFY_SUCCEEDED(myScope.Invoke(L'Q', false, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        invokeResult.invokedElement.reset();
        // If I supress filtering, A should not cause any filtering, Q should directly invoke after
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', false, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        VERIFY_SUCCEEDED(myScope.Invoke(L'Q', false, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        VERIFY_EXPECTATIONS(element1);
        VERIFY_EXPECTATIONS(element2);
        VERIFY_EXPECTATIONS(element3);

        VERIFY_EXPECTATIONS(*element1.m_pOwner);
        VERIFY_EXPECTATIONS(*element2.m_pOwner);
        VERIFY_EXPECTATIONS(*element3.m_pOwner);
    }

    void AKScopeUnitTests::VerifyCorrectCharactersFiltered()
    {
        MockCDO parent, element1, element2;

        AKAccessKey aKey1(std::wstring(L"Q")),
            aKey2(std::wstring(L"A1"));

        std::vector<std::pair<MockCDO*, AKAccessKey>> initList;
        initList.push_back(std::make_pair(&element1, aKey1));
        initList.push_back(std::make_pair(&element2, aKey2));

        AccessKeyScope myScope(&parent, initList);

        Expect(*element1.m_pOwner, HideAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, ShowAccessKey)
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element2.m_pOwner, ShowAccessKey)
            .Once()
            .With(CxxMock::StringEquals(L"A"))
            .ReturnValue(S_OK);
        Expect(*element2.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        AKInvokeReturnParams<MockCDO> invokeResult;
        // Try to invoke a key that is not in the scope e.g. 'T'.  Should expect invoked to be false
        VERIFY_SUCCEEDED(myScope.Invoke(L'T', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Invoke a key that is in Scope e.g. 'A'.  Because there are multiple that start with A, should start filtering
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);  // ShowAccessKey called on A1,A2.
        VERIFY_IS_NULL(invokeResult.invokedElement);

        invokeResult.invokedElement.reset();
        // Invoke another key that completes filtering
        VERIFY_SUCCEEDED(myScope.Invoke(L'1', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);  // Because this is a filtering match, will not call ShowAccessKey on Q
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        VERIFY_EXPECTATIONS(element1);
        VERIFY_EXPECTATIONS(element2);

        VERIFY_EXPECTATIONS(*element1.m_pOwner);
        VERIFY_EXPECTATIONS(*element2.m_pOwner);
    }

    void AKScopeUnitTests::VerifyInvokeProperlyHandlesSurrogatePairs()
    {
        MockCDO parent, element1, element2, element3;

        AKAccessKey aKey1(std::wstring(L"\U0001E932-")), // ADLaM small letter NUN (U+D83A U+DD32 surrogate pair) followed by minus sign
            aKey2(std::wstring(L"\U0001E926+")), // ADLaM small letter BA (U+D83A U+DD26 surrogate pair) followed by plus sign
            aKey3(std::wstring(L"A1"));

        std::vector<std::pair<MockCDO*, AKAccessKey>> initList;
        initList.push_back(std::make_pair(&element1, aKey1));
        initList.push_back(std::make_pair(&element2, aKey2));
        initList.push_back(std::make_pair(&element3, aKey3));

        AccessKeyScope myScope(&parent, initList);

        Expect(*element1.m_pOwner, HideAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element2.m_pOwner, HideAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element2.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element3.m_pOwner, ShowAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element3.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        AKInvokeReturnParams<MockCDO> invokeResult;

        // Invoke ADLaM small letter BHE (U+D83A U+DD29), should *not* start filtering.
        VERIFY_SUCCEEDED(myScope.Invoke(L'\xd83a', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);
        VERIFY_SUCCEEDED(myScope.Invoke(L'\xdd29', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Verify that we can now start filerting at this stage by invoking A.
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        VERIFY_EXPECTATIONS(element1);
        VERIFY_EXPECTATIONS(element2);
        VERIFY_EXPECTATIONS(element3);

        VERIFY_EXPECTATIONS(*element1.m_pOwner);
        VERIFY_EXPECTATIONS(*element2.m_pOwner);
        VERIFY_EXPECTATIONS(*element3.m_pOwner);
    }

    void AKScopeUnitTests::VerifyEscapeKeyFilteringBehavior()
    {
        MockCDO parent, element1, element2, element3;

        AKAccessKey aKey1(std::wstring(L"A1")),
            aKey2(std::wstring(L"A21")),
            aKey3(std::wstring(L"A22"));

        std::vector<std::pair<MockCDO*, AKAccessKey>> initList;
        initList.push_back(std::make_pair(&element1, aKey1));
        initList.push_back(std::make_pair(&element2, aKey2));
        initList.push_back(std::make_pair(&element3, aKey3));

        AccessKeyScope myScope(&parent, initList);

        Expect(*element1.m_pOwner, HideAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, ShowAccessKey)
            .Twice()
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element2.m_pOwner, ShowAccessKey)
            .CallCount(CxxMock::eq(3))
            .ReturnValue(S_OK);
        Expect(*element2.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element3.m_pOwner, ShowAccessKey)
            .CallCount(CxxMock::eq(3))
            .ReturnValue(S_OK);
        Expect(*element3.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        AKInvokeReturnParams<MockCDO> invokeResult;

        // Invoke a key that is in Scope e.g. 'A'.  Because there are multiple that start with A, should start filtering
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);  // ShowAccessKey called on A1,A2.
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Invoke another key that continues filtering
        VERIFY_SUCCEEDED(myScope.Invoke(L'2', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        invokeResult.invokedElement.reset();

        // Back up 1 character in the filtering, and then try to invoke A1.  This will cause A1 to have had show called two times.
        VERIFY_SUCCEEDED(myScope.ProcessEscapeKey());
        VERIFY_SUCCEEDED(myScope.Invoke(L'1', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        VERIFY_EXPECTATIONS(element1);
        VERIFY_EXPECTATIONS(element2);
        VERIFY_EXPECTATIONS(element3);

        VERIFY_EXPECTATIONS(*element1.m_pOwner);
        VERIFY_EXPECTATIONS(*element2.m_pOwner);
        VERIFY_EXPECTATIONS(*element3.m_pOwner);
    }

    void AKScopeUnitTests::VerifyEscapeKeyFilteringBehaviorWithSurrogatePairs()
    {
        MockCDO parent, element1, element2, element3;

        AKAccessKey aKey1(std::wstring(L"\U0001E932-")), // ADLaM small letter NUN (U+D83A U+DD32 surrogate pair) followed by minus sign
            aKey2(std::wstring(L"\U0001E926+")), // ADLaM small letter BA (U+D83A U+DD26 surrogate pair) followed by plus sign
            aKey3(std::wstring(L"A"));

        std::vector<std::pair<MockCDO*, AKAccessKey>> initList;
        initList.push_back(std::make_pair(&element1, aKey1));
        initList.push_back(std::make_pair(&element2, aKey2));
        initList.push_back(std::make_pair(&element3, aKey3));

        AccessKeyScope myScope(&parent, initList);

        Expect(*element1.m_pOwner, ShowAccessKey)
            .Twice()
            .ReturnValue(S_OK);
        Expect(*element1.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element2.m_pOwner, ShowAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element2.m_pOwner, HideAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element2.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        Expect(*element3.m_pOwner, HideAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element3.m_pOwner, ShowAccessKey)
            .Once()
            .ReturnValue(S_OK);
        Expect(*element3.m_pOwner, Invoke)
            .ReturnValue(S_OK);

        AKInvokeReturnParams<MockCDO> invokeResult;

        // Invoke ADLaM small letter NUN, should start filtering.
        VERIFY_SUCCEEDED(myScope.Invoke(L'\xd83a', true, &invokeResult));
        VERIFY_IS_FALSE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);
        VERIFY_SUCCEEDED(myScope.Invoke(L'\xdd32', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NULL(invokeResult.invokedElement);

        // Back up the whole surrogate pair in the filtering. This will cause A to be shown for the first time.
        VERIFY_SUCCEEDED(myScope.ProcessEscapeKey());

        // Verify that we can now invoke A at this stage.
        VERIFY_SUCCEEDED(myScope.Invoke(L'A', true, &invokeResult));
        VERIFY_IS_TRUE(invokeResult.invokeAttempted);
        VERIFY_IS_NOT_NULL(invokeResult.invokedElement);

        VERIFY_EXPECTATIONS(element1);
        VERIFY_EXPECTATIONS(element2);
        VERIFY_EXPECTATIONS(element3);

        VERIFY_EXPECTATIONS(*element1.m_pOwner);
        VERIFY_EXPECTATIONS(*element2.m_pOwner);
        VERIFY_EXPECTATIONS(*element3.m_pOwner);
    }

} } } } }
