// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>
#include <collection.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace StickyHeaders {

    class StickyHeadersIntegrationTests : public WEX::TestClass < StickyHeadersIntegrationTests >
    {
    public:
        BEGIN_TEST_CLASS(StickyHeadersIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(StickyGroupHeadersEnabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that sticky headers are functioning correctly with AreStickyGroupHeadersEnabled set to true.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(StickyGroupHeadersDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that sticky headers are functioning correctly with AreStickyGroupHeadersEnabled set to false.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateHeaderStretchInItemsStackPanel)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the header stretches when inline for ItemsStackPanel - converged behavior")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateHeaderStretchInItemsWrapGrid)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the header stretches when inline for ItemsWrapGrid - converged behavior")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
        //
        // Platform:Desktop
        //
        
        // Running only on desktop for now since input/DManip on the phone is causing 
        // test reliability issues.
        BEGIN_TEST_METHOD(OverpanBounceNoStickyHeader)
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // when doing touch, DComp dumps seem inconsistent.
            TEST_METHOD_PROPERTY(L"Description", L"Validates converged behavior for overpan - no more OverPanCompression in Threshold.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        // Running only on desktop for now since input/DManip on the phone is causing 
        // test reliability issues.
        BEGIN_TEST_METHOD(ValidateOverpanBounceWithStickyHeader)
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // when doing touch, DComp dumps seem inconsistent.
            TEST_METHOD_PROPERTY(L"Description", L"Validates converged behavior for overpan - no more OverPanCompression in Threshold. with sticky headers")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StickyGroupHeadersListHeaderResized)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that sticky header clip is updated when the List Header size changes")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        //
        // Platform:Phone
        //

    private:
        xaml_controls::ListView^ SetupGroupedListView(Platform::String^ areStickyGroupHeadersEnabledStringValue);
        xaml_controls::ListView^ SetupGroupedListViewWithoutAnimations(Platform::String^ areStickyGroupHeadersEnabledStringValue);
        xaml_controls::ListView^ SetupGroupedListViewWithInlineHeaders(Platform::String^ panelStringValue);
        xaml_controls::GridView^ SetupGroupedGridViewWithInlineHeaders(Platform::String^ panelStringValue);
        Platform::Collections::Vector<Platform::Object^>^ GetGroupedData();
    };

} } } } } }

