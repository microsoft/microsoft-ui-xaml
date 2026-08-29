// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "JumpListItemBackgroundConverterIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

#include <generic\DependencyObjectTests.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace JumpListItemBackgroundConverter {

    bool JumpListItemBackgroundConverterIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool JumpListItemBackgroundConverterIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool JumpListItemBackgroundConverterIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void JumpListItemBackgroundConverterIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_primitives::JumpListItemBackgroundConverter>::CanInstantiate();
    }

    void JumpListItemBackgroundConverterIntegrationTests::CanConvert()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&] ()
        {
            auto converter = ref new xaml_primitives::JumpListItemBackgroundConverter();

            auto expectedBrushWithItems = ref new xaml_media::SolidColorBrush(mu::Colors::Red);
            auto expectedBrushWithoutItems = ref new xaml_media::SolidColorBrush(mu::Colors::Blue);

            converter->Enabled = expectedBrushWithItems;
            converter->Disabled = expectedBrushWithoutItems;

            auto outerVector = ref new Platform::Collections::Vector<Platform::Object^>();
            auto innerVector = ref new Platform::Collections::Vector<Platform::String^>();

            innerVector->Append(L"Test String");

            outerVector->Append(innerVector);
            outerVector->Append(ref new Platform::Collections::Vector<Platform::String^>());

            auto source = ref new xaml_data::CollectionViewSource();
            source->IsSourceGrouped = true;
            source->Source = outerVector;

            auto groupWithItems = safe_cast<xaml_data::ICollectionViewGroup^>(source->View->CollectionGroups->GetAt(0));
            auto groupWithoutItems = safe_cast<xaml_data::ICollectionViewGroup^>(source->View->CollectionGroups->GetAt(1));

            auto actualBrushWithItems = safe_cast<xaml_media::SolidColorBrush^>(converter->Convert(groupWithItems, xaml_data::ICollectionViewGroup::typeid, nullptr, nullptr));
            auto actualBrushWithoutItems = safe_cast<xaml_media::SolidColorBrush^>(converter->Convert(groupWithoutItems, xaml_data::ICollectionViewGroup::typeid, nullptr, nullptr));

            VERIFY_IS_NOT_NULL(actualBrushWithItems);
            VERIFY_ARE_EQUAL(expectedBrushWithItems->Color.A, actualBrushWithItems->Color.A);
            VERIFY_ARE_EQUAL(expectedBrushWithItems->Color.R, actualBrushWithItems->Color.R);
            VERIFY_ARE_EQUAL(expectedBrushWithItems->Color.G, actualBrushWithItems->Color.G);
            VERIFY_ARE_EQUAL(expectedBrushWithItems->Color.B, actualBrushWithItems->Color.B);

            VERIFY_IS_NOT_NULL(actualBrushWithoutItems);
            VERIFY_ARE_EQUAL(expectedBrushWithoutItems->Color.A, actualBrushWithoutItems->Color.A);
            VERIFY_ARE_EQUAL(expectedBrushWithoutItems->Color.R, actualBrushWithoutItems->Color.R);
            VERIFY_ARE_EQUAL(expectedBrushWithoutItems->Color.G, actualBrushWithoutItems->Color.G);
            VERIFY_ARE_EQUAL(expectedBrushWithoutItems->Color.B, actualBrushWithoutItems->Color.B);
        });
    }

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::ToggleButton
