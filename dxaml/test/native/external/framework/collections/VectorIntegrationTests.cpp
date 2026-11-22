// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "VectorIntegrationTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Collections {

    bool VectorIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

        bool VectorIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

    bool VectorIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void VectorIntegrationTests::VerifyVectorOperationsBounds()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 300));
        xaml_controls::Grid^ grid = nullptr;

        RunOnUIThread([&]()
        {
            grid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            grid->Children->Append(ref new xaml_shapes::Rectangle());

            // Try to remove non-existent item
            VERIFY_THROWS_WINRT(grid->Children->RemoveAt(1), Platform::Exception^, L"An exception should be thrown when trying to remove a non-existent index.");
            VERIFY_ARE_EQUAL(grid->Children->Size, 1u);

            // Try to remove existent item
            grid->Children->RemoveAt(0);
            VERIFY_ARE_EQUAL(grid->Children->Size, 0u);

            // Try to insert at index equal to the vector size
            grid->Children->InsertAt(0, ref new xaml_shapes::Rectangle());
            VERIFY_ARE_EQUAL(grid->Children->Size, 1u);

            // Try to insert at index lower than the vector size
            grid->Children->InsertAt(0, ref new xaml_shapes::Rectangle());
            VERIFY_ARE_EQUAL(grid->Children->Size, 2u);

            // Try to insert at index greater than the vector size
            VERIFY_THROWS_WINRT(grid->Children->InsertAt(3, ref new xaml_shapes::Rectangle()), Platform::Exception^, L"An exception should be thrown when trying to insert elements in an index higher than the size.");
            VERIFY_ARE_EQUAL(grid->Children->Size, 2u);

            xaml_shapes::Rectangle^ rectangleWithName = ref new xaml_shapes::Rectangle();
            rectangleWithName->Name = "TestRect";

            // Try to set at non-existent index
            VERIFY_THROWS_WINRT(grid->Children->SetAt(2, rectangleWithName), Platform::Exception^, L"An exception should be thrown when trying to set elements in a non-existent index.");

            // Try to set at existent index
            grid->Children->SetAt(1, rectangleWithName);

            // Try to get at non-existent index, current implementation returns nullptr even if out of bounds, we should change this to return an exception
            // Since updating GetAt methods has a high risk of creating new regressions, this method should be updated for RS2
            auto nullResult = grid->Children->GetAt(2u);
            VERIFY_IS_NULL(nullResult);

            // Try to get at existent index
            xaml_shapes::Rectangle^ resultRect = (xaml_shapes::Rectangle^)grid->Children->GetAt(1);
            VERIFY_IS_TRUE(resultRect->Name == "TestRect");

            // Try to remove at end when vector size is greater than zero
            grid->Children->RemoveAtEnd();
            VERIFY_ARE_EQUAL(grid->Children->Size, 1u);

            // Try to remove at end when vector size is greater than zero
            grid->Children->RemoveAtEnd();
            VERIFY_ARE_EQUAL(grid->Children->Size, 0u);

            // Try to remove at end when vector size is zero
            VERIFY_THROWS_WINRT(grid->Children->RemoveAtEnd(), Platform::Exception^, L"An exception should be thrown when trying to remove elements in an empty vector.");
        });
    }

    void VectorIntegrationTests::ItemCollectionIndexOfComparesBoxedValuesByValue()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            // An ItemsControl without an ItemsSource keeps its items in its own ItemCollection, whose IndexOf
            // compares the requested value with each stored item by identity first and then by boxed value.
            auto itemsControl = ref new xaml_controls::ItemsControl();
            auto items = itemsControl->Items;

            const GUID guid1 = { 0x12345678, 0x1234, 0x5678, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 } };
            const GUID guid2 = { 0x12345678, 0x1234, 0x5678, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09 } };
            wf::TimeSpan timeSpan;
            timeSpan.Duration = 10000001;
            wf::TimeSpan otherTimeSpan;
            otherTimeSpan.Duration = 10000000;
            Platform::Object^ nanBox = wf::PropertyValue::CreateSingle(std::numeric_limits<float>::quiet_NaN());

            items->Append(wf::PropertyValue::CreateChar16(L'a'));                         // 0
            items->Append(wf::PropertyValue::CreateChar16(L'b'));                         // 1
            items->Append(wf::PropertyValue::CreateUInt8(200));                           // 2
            items->Append(wf::PropertyValue::CreateInt16(-7));                            // 3
            items->Append(wf::PropertyValue::CreateUInt16(65000));                        // 4
            items->Append(wf::PropertyValue::CreateUInt32(4000000000u));                  // 5
            items->Append(wf::PropertyValue::CreateInt64(-5000000000ll));                 // 6
            items->Append(wf::PropertyValue::CreateUInt64(18000000000000000000ull));      // 7
            items->Append(wf::PropertyValue::CreateSingle(1.5f));                         // 8
            items->Append(wf::PropertyValue::CreateSize(wf::Size(2.0f, 3.0f)));           // 9
            items->Append(wf::PropertyValue::CreateGuid(Platform::Guid(guid1)));          // 10
            items->Append(wf::PropertyValue::CreateTimeSpan(timeSpan));                   // 11
            items->Append(nanBox);                                                        // 12
            items->Append(wf::PropertyValue::CreateSingle(0.0f));                         // 13
            VERIFY_ARE_EQUAL(14u, items->Size);

            auto verifyFound = [items](Platform::Object^ value, unsigned int expectedIndex)
            {
                unsigned int index = 0xFFFFFFFF;
                VERIFY_IS_TRUE(items->IndexOf(value, &index));
                VERIFY_ARE_EQUAL(expectedIndex, index);
            };

            auto verifyNotFound = [items](Platform::Object^ value)
            {
                unsigned int index = 0xFFFFFFFF;
                VERIFY_IS_FALSE(items->IndexOf(value, &index));
            };

            LOG_OUTPUT(L"Char16: equal value found at its own index; other characters (including other case) are not found.");
            verifyFound(wf::PropertyValue::CreateChar16(L'b'), 1u);
            verifyNotFound(wf::PropertyValue::CreateChar16(L'B'));
            verifyNotFound(wf::PropertyValue::CreateChar16(L'c'));

            LOG_OUTPUT(L"UInt8");
            verifyFound(wf::PropertyValue::CreateUInt8(200), 2u);
            verifyNotFound(wf::PropertyValue::CreateUInt8(201));

            LOG_OUTPUT(L"Int16");
            verifyFound(wf::PropertyValue::CreateInt16(-7), 3u);
            verifyNotFound(wf::PropertyValue::CreateInt16(-8));

            LOG_OUTPUT(L"UInt16");
            verifyFound(wf::PropertyValue::CreateUInt16(65000), 4u);
            verifyNotFound(wf::PropertyValue::CreateUInt16(64999));

            LOG_OUTPUT(L"UInt32");
            verifyFound(wf::PropertyValue::CreateUInt32(4000000000u), 5u);
            verifyNotFound(wf::PropertyValue::CreateUInt32(3999999999u));

            LOG_OUTPUT(L"Int64");
            verifyFound(wf::PropertyValue::CreateInt64(-5000000000ll), 6u);
            verifyNotFound(wf::PropertyValue::CreateInt64(-4999999999ll));
            LOG_OUTPUT(L"Int64: values that share the low 32 bits but differ in the high 32 bits are different.");
            verifyNotFound(wf::PropertyValue::CreateInt64(-5000000000ll + 0x100000000ll));
            verifyNotFound(wf::PropertyValue::CreateInt64(-5000000000ll - 0x100000000ll));

            LOG_OUTPUT(L"UInt64 above INT64_MAX");
            verifyFound(wf::PropertyValue::CreateUInt64(18000000000000000000ull), 7u);
            verifyNotFound(wf::PropertyValue::CreateUInt64(17999999999999999999ull));
            LOG_OUTPUT(L"UInt64: values that share the low 32 bits but differ in the high 32 bits are different.");
            verifyNotFound(wf::PropertyValue::CreateUInt64(18000000000000000000ull - 0x100000000ull));

            LOG_OUTPUT(L"Single");
            verifyFound(wf::PropertyValue::CreateSingle(1.5f), 8u);
            verifyNotFound(wf::PropertyValue::CreateSingle(1.25f));

            LOG_OUTPUT(L"Size: both dimensions must match.");
            verifyFound(wf::PropertyValue::CreateSize(wf::Size(2.0f, 3.0f)), 9u);
            verifyNotFound(wf::PropertyValue::CreateSize(wf::Size(2.0f, 4.0f)));
            verifyNotFound(wf::PropertyValue::CreateSize(wf::Size(5.0f, 3.0f)));

            LOG_OUTPUT(L"Guid: a difference in the last byte is significant.");
            verifyFound(wf::PropertyValue::CreateGuid(Platform::Guid(guid1)), 10u);
            verifyNotFound(wf::PropertyValue::CreateGuid(Platform::Guid(guid2)));

            LOG_OUTPUT(L"TimeSpan: a difference of one tick is significant.");
            verifyFound(wf::PropertyValue::CreateTimeSpan(timeSpan), 11u);
            verifyNotFound(wf::PropertyValue::CreateTimeSpan(otherTimeSpan));

            LOG_OUTPUT(L"Single NaN: a new NaN box never compares equal, but the same box instance is found by identity.");
            verifyNotFound(wf::PropertyValue::CreateSingle(std::numeric_limits<float>::quiet_NaN()));
            verifyFound(nanBox, 12u);

            LOG_OUTPUT(L"Single -0 compares equal to +0.");
            verifyFound(wf::PropertyValue::CreateSingle(-0.0f), 13u);

            LOG_OUTPUT(L"Values with the same number but a different property type are not found.");
            verifyNotFound(wf::PropertyValue::CreateInt32(200));
            verifyNotFound(wf::PropertyValue::CreateUInt16(200));
            verifyNotFound(wf::PropertyValue::CreateDouble(1.5));
            verifyNotFound(wf::PropertyValue::CreateInt64(4000000000ll));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Collections
