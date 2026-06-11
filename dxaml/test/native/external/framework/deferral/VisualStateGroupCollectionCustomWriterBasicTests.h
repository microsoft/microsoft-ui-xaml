// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

struct IVisualStateGroupCollectionTestHooks;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class VisualStateGroupCollectionCustomWriterBasicTests : public WEX::TestClass<VisualStateGroupCollectionCustomWriterBasicTests>
        {
        public:
            BEGIN_TEST_CLASS(VisualStateGroupCollectionCustomWriterBasicTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateVisualStateGroupStringExtraction)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the CustomWriter extracts the VisualStateGroup strings correctly.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateVisualStateGroupStringNonDirectiveExtraction)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the CustomWriter extracts the VisualStateGroup strings correctly when using a directive.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateVisualStateGroupStringAsElementExtraction)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the CustomWriter extracts the VisualStateGroup strings correctly when using an element.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateVisualStateStringExtraction)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the CustomWriter extracts the VisualState strings correctly.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateVisualStateGroupTransitionDetection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the CustomWriter can detect and flag VisualStateGroups that require dynamically generated timelines.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateEmptyVisualStateGroupCollectionExtraction)
                TEST_METHOD_PROPERTY(L"Description", L"Validates an empty VisualStateGroupCollection parses correctly.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateBasicObjectCreation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates a storyboard can be delay-created.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateMarkupExtensionInTransitionPropertyDoesNotForceBailout)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that using a markup extension to provide the value for a VisualTransition property does not result in deferral bailing out.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()


        private:
            static void GetVisualStateGroupTestHooksFromRootElement(Microsoft::UI::Xaml::FrameworkElement^ element, IVisualStateGroupCollectionTestHooks** ppTestHooks);

            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureParserCustomWriter;
        };
    }
} } } }
