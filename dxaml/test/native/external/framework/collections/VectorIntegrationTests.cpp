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

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Collections
