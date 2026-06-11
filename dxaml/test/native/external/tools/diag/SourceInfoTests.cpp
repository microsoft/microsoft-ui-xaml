// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "SourceInfoTests.h"
#include "XamlDiagnosticsTestHelpers.h"
#include <CustomUserControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <TestEvent.h>
#include "MainPage.xaml.h"
#include <CustomMetadataRegistrar.h>
#include "RuleTesterHelper.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace ::Tests::Tools::XamlDiagnostics;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace shared_types = ::Tests::Tools::Shared;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        #pragma region Test Methods

        bool SourceInfoTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool SourceInfoTests::ClassCleanup()
        {
            return true;
        }

        bool SourceInfoTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());

            return EnsureTapLoaded();
        }

        bool SourceInfoTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        void SourceInfoTests::TestSourceInfo()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback = m_connectionHelper->Advise();

            TestCleanupWrapper cleanup([&] {
                m_connectionHelper->OnTestComplete(callback);
            });

            RunOnUIThread([&]()
            {
                Canvas^ rootPanel = ref new Canvas();
                Platform::String^ componentLocation = "ms-appx:///resources/sample/SampleWithChecksum.xaml";
                Application::LoadComponent(
                    rootPanel,
                    ref new ::Windows::Foundation::Uri(componentLocation),
                    Primitives::ComponentResourceLocation::Application);

                VERIFY_IS_NOT_NULL(rootPanel);

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto canvasHandle = callback->GetElementByName(L"rootPanel");

            VERIFY_ARE_EQUAL(canvasHandle.SrcInfo.LineNumber, 4u);
            VERIFY_ARE_EQUAL(canvasHandle.SrcInfo.ColumnNumber, 9u);
            VERIFY_ARE_EQUAL(wcscmp(canvasHandle.SrcInfo.Hash, L"000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F"), 0);
        }

        void SourceInfoTests::TestSourceInfoOnEmptyElements()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XbfEmptyGridWithBorder.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto borderHandle = callback->GetElementInPageByType(L"Microsoft.UI.Xaml.Controls.Border");
            auto gridHandle = callback->GetElementInPageByType(L"Microsoft.UI.Xaml.Controls.Grid");
            VERIFY_ARE_EQUAL(borderHandle.SrcInfo.LineNumber, 7u);
            VERIFY_ARE_EQUAL(borderHandle.SrcInfo.ColumnNumber, 6u);

            VERIFY_ARE_EQUAL(gridHandle.SrcInfo.LineNumber, 8u);
            VERIFY_ARE_EQUAL(gridHandle.SrcInfo.ColumnNumber, 10u);
        }

        void SourceInfoTests::TestSourceInfoOnGridWithBrush()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XbfGridWithBrush.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto gridHandle = callback->GetElementByName(L"grid");
            VERIFY_ARE_EQUAL(gridHandle.SrcInfo.LineNumber, 16u);
            // The column number, because of the background property, is reported at 11. VS only requires that
            // the column number be between the two tags, so this test case reflects that.
            VERIFY_IS_TRUE(gridHandle.SrcInfo.ColumnNumber >= 6u && gridHandle.SrcInfo.ColumnNumber <= 89u);

            auto canvasHandle = callback->GetElementByName(L"canvas");
            VERIFY_ARE_EQUAL(canvasHandle.SrcInfo.LineNumber, 19u);
            VERIFY_IS_TRUE(canvasHandle.SrcInfo.ColumnNumber >= 10u && canvasHandle.SrcInfo.ColumnNumber <= 32u);

            auto buttonHandle = callback->GetElementByName(L"button");
            VERIFY_ARE_EQUAL(buttonHandle.SrcInfo.LineNumber, 17u);
            VERIFY_IS_TRUE(buttonHandle.SrcInfo.ColumnNumber >= 9u && buttonHandle.SrcInfo.ColumnNumber <= 146u);
        }

        void SourceInfoTests::TestSourceInfoOnCustomWriters()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XbfCustomWriters.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto buttonHandle = callback->GetElementByName(L"theButton");

            PropertyChainSource buttonStyle = m_tap->GetPropertyChainSource(buttonHandle.Handle, BaseValueSourceStyle, 4u);
            VERIFY_ARE_EQUAL(buttonStyle.SrcInfo.LineNumber, 9u);
            VERIFY_IS_TRUE(buttonStyle.SrcInfo.ColumnNumber >= 10u && buttonStyle.SrcInfo.ColumnNumber <= 56);
       }

        void SourceInfoTests::TestSourceInfoOnInlineStyle()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XbfInlineStyle.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto buttonHandle = callback->GetElementByName(L"theButton");

            // Get properties on the grid so we can check the line info on style
            PropertyChainSource style = m_tap->GetSourceForProperty(buttonHandle.Handle, L"Width", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(style.SrcInfo.LineNumber, 9u);
            VERIFY_IS_TRUE(style.SrcInfo.ColumnNumber >= 12 && style.SrcInfo.ColumnNumber <= 37);

        }

        void SourceInfoTests::TestSourceInfoOnNestedStyles()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XbfNestedStyles.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto listViewItemHandle = callback->GetElementInPageByType(L"Microsoft.UI.Xaml.Controls.ListViewItem");

            // Get properties on the grid so we can check the line info on style and resource dictionary
            PropertyChainSource style = m_tap->GetSourceForProperty(listViewItemHandle.Handle, L"Background", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(style.SrcInfo.LineNumber, 11u);
            VERIFY_IS_TRUE(style.SrcInfo.ColumnNumber >= 14u && style.SrcInfo.ColumnNumber <= 45u);
        }

        void SourceInfoTests::TestSourceInfoOnBasedOnStyle()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XbfBasedOnStyle.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto buttonHandle = callback->GetElementByName(L"button");

            // The effective BorderThickness property should be reported on the applied style
            wil::unique_propertychainsource borderThicknessStyle = m_tap->GetSourceForProperty(buttonHandle.Handle, L"BorderThickness", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(borderThicknessStyle.SrcInfo.LineNumber, 12u);
            VERIFY_IS_TRUE(borderThicknessStyle.SrcInfo.ColumnNumber >= 8u && borderThicknessStyle.SrcInfo.ColumnNumber <= 118u);

            // The overwritten BorderThickness property should be reported on the BasedOnStyle
            wil::unique_propertychainsource overridenBorderThicknessStyle = m_tap->GetSourceForProperty(buttonHandle.Handle, L"BorderThickness", BaseValueSourceStyle, true /*overridden*/);
            VERIFY_ARE_EQUAL(overridenBorderThicknessStyle.SrcInfo.LineNumber, 8u);
            VERIFY_IS_TRUE(overridenBorderThicknessStyle.SrcInfo.ColumnNumber >= 8u && overridenBorderThicknessStyle.SrcInfo.ColumnNumber <= 61u);

            // The Background property should be reported on the BasedOnStyle since it isn't overwritten
            wil::unique_propertychainsource effectiveBackgroundStyle = m_tap->GetSourceForProperty(buttonHandle.Handle, L"Background", BaseValueSourceStyle);
            VERIFY_ARE_EQUAL(effectiveBackgroundStyle.SrcInfo.LineNumber, 8u);
            VERIFY_IS_TRUE(effectiveBackgroundStyle.SrcInfo.ColumnNumber >= 8u && effectiveBackgroundStyle.SrcInfo.ColumnNumber <= 118u);
        }

        void SourceInfoTests::TestSourceInfoOnRootOfXamlDoc()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PageWithCustomUserControl.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto pageElement = callback->GetElementInPageByType(L"Microsoft.UI.Xaml.Controls.Page");

            VERIFY_IS_GREATER_THAN_OR_EQUAL(pageElement.SrcInfo.LineNumber, 3u);
            VERIFY_IS_LESS_THAN_OR_EQUAL(pageElement.SrcInfo.LineNumber, 6u);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(pageElement.SrcInfo.ColumnNumber, 0u);
            VERIFY_IS_LESS_THAN_OR_EQUAL(pageElement.SrcInfo.ColumnNumber, 52u);

            auto customUserControl = callback->GetElementByName(L"userControl1");
            VERIFY_ARE_EQUAL(customUserControl.SrcInfo.LineNumber, 13u);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(customUserControl.SrcInfo.ColumnNumber, 10u);
            VERIFY_IS_LESS_THAN_OR_EQUAL(customUserControl.SrcInfo.ColumnNumber, 145u);
            VERIFY_IS_TRUE(wcscmp(componentLocation->ToString()->Data(), customUserControl.SrcInfo.FileName) == 0);
        }

        void SourceInfoTests::VerifyImplicitResourceDictionaryHasSourceInfo()
        {
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/MainPage.xaml");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement grid = callback->GetElementByName(L"LayoutRoot");
            // The local scope's base value sources is the dictionary itself
            unsigned int resourcesIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(grid.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &resourcesIndex));
            InstanceHandle dictionaryHandle = 0;
            VERIFY_SUCCEEDED(m_tap->GetProperty(grid.Handle, resourcesIndex, &dictionaryHandle));
            auto dictionarySource = m_tap->GetPropertyChainSource(dictionaryHandle, BaseValueSource::BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(dictionaryHandle, dictionarySource.Handle);

            VERIFY_ARE_EQUAL(dictionarySource.SrcInfo.LineNumber, 22u);
            VERIFY_ARE_EQUAL(dictionarySource.SrcInfo.ColumnNumber, 26u);
            VERIFY_ARE_EQUAL(wcscmp(componentLocation->ToString()->Data(), dictionarySource.SrcInfo.FileName), 0);
        }

        void SourceInfoTests::VerifySourceInfoDesignModeV2()
        {
            // Shut off Xbf v2, make sure that being in the new design mode causes correct source info.
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            TestSourceInfoOnEmptyElements();
        }

        #pragma endregion

    }
} } } } }
