// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "UIElementWeakCollectionTests.h"
#include <XamlTailored.h>
#include "TestCleanupWrapper.h"
#include <collection.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Platform::Collections;
using namespace ::Windows::Foundation::Collections;

using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics;

bool UIElementWeakCollectionTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

void UIElementWeakCollectionTests::ItemAccess()
{
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        Canvas^ e1 = ref new Canvas();
        Canvas^ e2 = ref new Canvas();
        Canvas^ e3 = ref new Canvas();
        Canvas^ e4 = ref new Canvas();

        LOG_OUTPUT(L"> Creating UIElementWeakCollection.");
        UIElementWeakCollection^ weakCollection = ref new UIElementWeakCollection();
        VERIFY_IS_NOT_NULL(weakCollection);
        VERIFY_ARE_EQUAL(0u, weakCollection->Size);

        LOG_OUTPUT(L"> Appending to UIElementWeakCollection.");
        weakCollection->Append(e1);
        weakCollection->Append(e4);

        LOG_OUTPUT(L"> Retrieving from UIElementWeakCollection.");
        VERIFY_ARE_EQUAL(2u, weakCollection->Size);
        VERIFY_ARE_EQUAL(e1, weakCollection->GetAt(0));
        VERIFY_ARE_EQUAL(e4, weakCollection->GetAt(1));

        LOG_OUTPUT(L"> Inserting into UIElementWeakCollection.");
        weakCollection->InsertAt(2, e3);
        weakCollection->InsertAt(1, e2);

        LOG_OUTPUT(L"> Retrieving from UIElementWeakCollection.");
        VERIFY_ARE_EQUAL(4u, weakCollection->Size);
        VERIFY_ARE_EQUAL(e1, weakCollection->GetAt(0));
        VERIFY_ARE_EQUAL(e2, weakCollection->GetAt(1));
        VERIFY_ARE_EQUAL(e4, weakCollection->GetAt(2));
        VERIFY_ARE_EQUAL(e3, weakCollection->GetAt(3));

        LOG_OUTPUT(L"> Removing from UIElementWeakCollection.");
        weakCollection->RemoveAt(2);

        LOG_OUTPUT(L"> Iterating over UIElementWeakCollection.");
        VERIFY_ARE_EQUAL(3u, weakCollection->Size);
        Vector<UIElement^> expected({ e1, e2, e3 });
        IIterator<UIElement^>^ expectedIterator = expected.First();
        for (UIElement^ item : weakCollection)
        {
            VERIFY_ARE_EQUAL(expectedIterator->Current, item);
            expectedIterator->MoveNext();
        }
        VERIFY_IS_FALSE(expectedIterator->HasCurrent);

        LOG_OUTPUT(L"> Removing from UIElementWeakCollection.");
        weakCollection->RemoveAtEnd();

        LOG_OUTPUT(L"> Retrieving from UIElementWeakCollection.");
        VERIFY_ARE_EQUAL(2u, weakCollection->Size);
        VERIFY_ARE_EQUAL(e1, weakCollection->GetAt(0));
        VERIFY_ARE_EQUAL(e2, weakCollection->GetAt(1));

        LOG_OUTPUT(L"> Setting element of UIElementWeakCollection.");
        weakCollection->SetAt(0, e4);

        LOG_OUTPUT(L"> Retrieving from UIElementWeakCollection.");
        VERIFY_ARE_EQUAL(2u, weakCollection->Size);
        VERIFY_ARE_EQUAL(e4, weakCollection->GetAt(0));
        VERIFY_ARE_EQUAL(e2, weakCollection->GetAt(1));

        LOG_OUTPUT(L"> Clearing from UIElementWeakCollection.");
        weakCollection->Clear();
        VERIFY_ARE_EQUAL(0u, weakCollection->Size);

        LOG_OUTPUT(L"> Try to insert past the end of UIElementWeakCollection. Expect bounds error.");
        VERIFY_THROWS_SPECIFIC_WINRT(weakCollection->InsertAt(5, e3), Platform::Exception^, [](Platform::Exception^ ex) { return ex->HResult == E_BOUNDS; });
    });
    wh->WaitForIdle();
}

void UIElementWeakCollectionTests::WeakRef()
{
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    UIElementWeakCollection^ weakCollection;
    Canvas^ e1;
    Canvas^ e2;
    Canvas^ e3;
    Canvas^ e4;
    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        e1 = ref new Canvas();
        e2 = ref new Canvas();
        e3 = ref new Canvas();
        e4 = ref new Canvas();

        LOG_OUTPUT(L"> Creating UIElementWeakCollection.");
        weakCollection = ref new UIElementWeakCollection();
        VERIFY_IS_NOT_NULL(weakCollection);
        VERIFY_ARE_EQUAL(0u, weakCollection->Size);

        LOG_OUTPUT(L"> Appending to UIElementWeakCollection.");
        weakCollection->Append(e1);
        weakCollection->Append(e2);
        weakCollection->Append(e3);
        weakCollection->Append(e4);

        LOG_OUTPUT(L"> Retrieving from UIElementWeakCollection.");
        VERIFY_ARE_EQUAL(4u, weakCollection->Size);
        VERIFY_ARE_EQUAL(e1, weakCollection->GetAt(0));
        VERIFY_ARE_EQUAL(e2, weakCollection->GetAt(1));
        VERIFY_ARE_EQUAL(e3, weakCollection->GetAt(2));
        VERIFY_ARE_EQUAL(e4, weakCollection->GetAt(3));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Releasing some items.");
        e2 = nullptr;
        e4 = nullptr;

        LOG_OUTPUT(L"> Retrieving from UIElementWeakCollection. The released items should return null.");
        VERIFY_ARE_EQUAL(4u, weakCollection->Size);
        VERIFY_ARE_EQUAL(e1, weakCollection->GetAt(0));
        VERIFY_IS_NULL(weakCollection->GetAt(1));
        VERIFY_ARE_EQUAL(e3, weakCollection->GetAt(2));
        VERIFY_IS_NULL(weakCollection->GetAt(3));

        LOG_OUTPUT(L"> Iterating over UIElementWeakCollection. The released items should return null.");
        VERIFY_ARE_EQUAL(4u, weakCollection->Size);
        Vector<UIElement^> expected({ e1, nullptr, e3, nullptr });
        IIterator<UIElement^>^ expectedIterator = expected.First();
        for (UIElement^ item : weakCollection)
        {
            VERIFY_ARE_EQUAL(expectedIterator->Current, item);
            expectedIterator->MoveNext();
        }
        VERIFY_IS_FALSE(expectedIterator->HasCurrent);
    });
}

void UIElementWeakCollectionTests::NonDependencyObject()
{
    auto wh = TestServices::WindowHelper;
    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        Canvas^ root = ref new Canvas();
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Creating UIElementWeakCollection.");
        UIElementWeakCollection^ weakCollection = ref new UIElementWeakCollection();
        VERIFY_IS_NOT_NULL(weakCollection);

        LOG_OUTPUT(L"> Casting to IInspectable.");
        wrl::ComPtr<IInspectable> weakCollectionII = reinterpret_cast<IInspectable*>(weakCollection);
        VERIFY_IS_NOT_NULL(weakCollectionII);

        {
            LOG_OUTPUT(L"> QI to DependencyObject. This should fail.");
            void* dependencyObject = nullptr;
            HRESULT castHR = weakCollectionII->QueryInterface(__uuidof(DependencyObject^), &dependencyObject);
            VERIFY_ARE_EQUAL(E_NOINTERFACE, castHR);
            VERIFY_IS_NULL(dependencyObject);
        }

        {
            LOG_OUTPUT(L"> QI to IDependencyObject. This should fail.");
            void* dependencyObject = nullptr;
            HRESULT castHR = weakCollectionII->QueryInterface(__uuidof(IDependencyObject^), &dependencyObject);
            VERIFY_ARE_EQUAL(E_NOINTERFACE, castHR);
            VERIFY_IS_NULL(dependencyObject);
        }
    });
}
