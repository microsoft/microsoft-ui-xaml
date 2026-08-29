// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class HighContrastAdjustmentTests : public WEX::TestClass<HighContrastAdjustmentTests>
        {
        public:
            BEGIN_TEST_CLASS(HighContrastAdjustmentTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VerifyTextBlockDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the TextBlock DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"UAP:WaitForXamlWindowActivation", L"false")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyRichTextBlockDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the RichTextBlock DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the TextBox DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyRichEditBoxDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the RichEditBox DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyPasswordBoxDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the PasswordBox DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyHyperlinkDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the Hyperlink DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyHyperlinkButtonDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the HyperlinkButton DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifySelectedTextBlockDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the TextBlock DComp tree with selected text and high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifySelectedTextBoxDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the TextBox DComp tree with selected text and high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifySelectedRichTextBlockDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the RichTextBlock DComp tree with selected text and high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCheckBoxDCompTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the CheckBox DComp tree with and without high contrast adjustments.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyBackPlateUpdatesOnTextAlignment)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the BackPlate updates its position when text moves due to text alignment.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyHighContrastAdjustmentOpacityOverride)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the HighContrastAdjustment Opacity override behavior.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            // TODO RichTextBlock doesn't update font when Enabling/Disabling control because it lacks an OnPropertyChanged method and currently uses SetValue for similar operations.
            BEGIN_TEST_METHOD(VerifyHCAFontOverrideUpdatesWhenControlEnabledChanges)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the HighContrastAdjustment Font override updates after ControlEnabled property changes.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // MockDComp brush differences
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            END_TEST_METHOD()

        private:
            void ValidateControlDCompTree(Platform::String^ xamlString);
            void ValidateControlDCompTree(
                Platform::String^ xamlString, ElementHighContrastAdjustment elementHCAdj,
                ApplicationHighContrastAdjustment appHCAdj, HighContrastTheme hcTheme, Platform::String^ variation);
        };

    } }
} } } }

