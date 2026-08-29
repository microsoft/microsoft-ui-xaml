//  Copyright (c) Microsoft Corporation.  All rights reserved.

#include "pch.h"
#include "FontFamilyModelTests.h"
#include <XamlTailored.h>
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <CustomSystemFontCollectionOverride.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace Private::Infrastructure;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        bool FontFamilyModelTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool FontFamilyModelTests::TestSetup()
        {
            Private::Infrastructure::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool FontFamilyModelTests::TestCleanup()
        {
            Private::Infrastructure::TestServices::WindowHelper->ShutdownXaml();
            Private::Infrastructure::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        void FontFamilyModelTests::ValidateVariableFontUsage()
        {
            RuntimeEnabledFeatureOverride featureForceTypographicModel(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceDWriteTypographicModel, true);
            TestServices::Utilities->ClearDefaultLanguageString();

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            static wchar_t const* fontNames[] = { L"Segoe UI", L"Sitka" };
            static wchar_t const* fontFileNames[] = { L"segoeui.ttf", L"SitkaVF.ttf" };

            CustomSystemFontCollectionOverride systemFontCollectionOverride(fontFileNames, ARRAYSIZE(fontFileNames), fontNames, ARRAYSIZE(fontNames));

            wf::Size size(800, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TextBlock^ textBlock = nullptr;
            RunOnUIThread([&]()
            {
                // This is a a fairly simple test.  If the the Textblock is not rendered using the Typographic font model, the variable
                // Sitka font will be rendered as Segoe.  Then we dump multiple variations to ensure that we properly select the font.
                auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <TextBlock FontSize='20' FontFamily='XamlAutoFontFamily' Text='XamlAutoFontFamily'/>"
                    L"  <TextBlock FontSize='20' FontFamily='Sitka' Text='Sitka Family'/>"
                    L"  <TextBlock FontSize='20' FontFamily='Sitka' Text='Sitka Family Bold' FontWeight='Bold'/>"
                    L"  <TextBlock FontSize='20' FontFamily='Sitka' Text='Sitka Family Italic' FontStyle='Italic'/>"
                    L"  <TextBlock FontSize='20' FontFamily='Sitka' Text='Sitka Family Condensed' FontStretch='Condensed'/>"
                    L"  <Grid>"
                    L"    <Grid.RenderTransform>"
                    L"       <ScaleTransform ScaleX='12' ScaleY='12'/>"
                    L"    </Grid.RenderTransform>"
                    L"    <TextBlock FontSize='5' FontFamily='Sitka' Text='Sitka Family Large Scale' Padding='0,0,0,50'/>"
                    L"  </Grid>"
                    L"  <TextBlock FontSize='60' FontFamily='Sitka' Text='Sitka Family Large Font'/>"
                    L"  <RichTextBlock FontFamily='Sitka' FontSize='20'>"
                    L"    <Paragraph>"
                    L"      <Run>Sitka Family</Run>"
                    L"      <Run FontWeight='Bold'>Bold</Run>"
                    L"      <Run FontStyle='Italic'>Italic</Run>"
                    L"      <Run FontStretch='Condensed'>Condensed</Run>"
                    L"      <Run FontSize='60'>Large</Run>"
                    L"    </Paragraph>"
                    L"  </RichTextBlock>"
                    L"  <TextBox Text='Sitka Text Box' FontFamily='Sitka' FontSize='20'/>" // This should render as Segoe UI since RichEdit doesn't support typographic model.
                    L"</StackPanel>"));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            // dump all surfaces to verify text displayed correctly
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);

        }

    } }
} } } }

