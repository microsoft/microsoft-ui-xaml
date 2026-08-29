// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SemanticZoom {

    class SemanticZoomIntegrationTests : public WEX::TestClass<SemanticZoomIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(SemanticZoomIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP") // DCPP NoCoreWindow mode - SemanticZoom tests fail get stuck "Waiting for BuildTreeService to finish..."
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a SemanticZoom.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a SemanticZoom from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanZoomOutToKeysList)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully zoom out to the view showing list of group keys.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBackButtonExitsZoomedOutView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully zoom out to the view showing list of group keys.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanZoomInToKey)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully zoom in to a list of items grouped by a key.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set and get SemanticZom specific properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeAlignment)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the VerticalAlignment of the SemanticZoom can be changed without causing a layout cycle.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        xaml_controls::SemanticZoom^ SetupSemanticZoomTest();
        void DoToggleActiveView(
            xaml_controls::SemanticZoom^ semanticZoom,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> viewChangeStartedEvent,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> viewChangeCompletedEvent);
    };

} } } } } }
