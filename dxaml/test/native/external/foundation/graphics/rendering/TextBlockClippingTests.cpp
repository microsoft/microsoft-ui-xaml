// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBlockClippingTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        Platform::String^ TextBlockClippingTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }

        bool TextBlockClippingTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextBlockClippingTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextBlockClippingTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Renders scaled TextBlock and RichTextBlock to ensure that text is not trimmed
        // when given enough horizontal space
        //------------------------------------------------------------------------
        void TextBlockClippingTests::BasicTrimmingTest()
        {
            // Clear out the current window content before injecting MockDComp, to
            // MockDComp doesn't capture an image for anything currently in the content,
            // since that will interfere with the expected surface counts.
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), 1.4f);

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TextBlockClippingTests.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
        }

        //------------------------------------------------------------------------
        // Test case: Redirects TextBlock content across a fractional column boundary at 150%
        // scale and verifies the glyph mask retains its full edge coverage.
        //------------------------------------------------------------------------
        void TextBlockClippingTests::RedirectVisualAtFractionalScale()
        {
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(401, 300));
            TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
            TestServices::Utilities->ResetMockDCompSurfaceId();

            Grid^ root = nullptr;
            ListView^ sourceElement = nullptr;
            Border^ targetElement = nullptr;
            RedirectVisual^ redirectVisual = nullptr;

            RunOnUIThread([&]()
            {
                root = safe_cast<Grid^>(XamlReader::Load(
                    L"<Grid Width='401' Height='300' "
                    L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    L"xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.ColumnDefinitions>"
                    L"    <ColumnDefinition Width='*'/>"
                    L"    <ColumnDefinition Width='*'/>"
                    L"  </Grid.ColumnDefinitions>"
                    L"  <ListView x:Name='SourceElement'>"
                    L"    <ListViewItem><TextBlock FontSize='14' Text='Hello world'/></ListViewItem>"
                    L"    <ListViewItem><TextBlock FontSize='14' Text='Hello world'/></ListViewItem>"
                    L"    <ListViewItem><TextBlock FontSize='14' Text='Hello world'/></ListViewItem>"
                    L"    <ListViewItem><TextBlock FontSize='14' Text='Hello world'/></ListViewItem>"
                    L"  </ListView>"
                    L"  <Border x:Name='TargetElement' Grid.Column='1'/>"
                    L"</Grid>"));

                sourceElement = safe_cast<ListView^>(root->FindName(L"SourceElement"));
                targetElement = safe_cast<Border^>(root->FindName(L"TargetElement"));
                VERIFY_IS_NOT_NULL(sourceElement);
                VERIFY_IS_NOT_NULL(targetElement);

                root->RasterizationScale = 1.5f;
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto sourceVisual = ElementCompositionPreview::GetElementVisual(sourceElement);
                VERIFY_IS_NOT_NULL(sourceVisual);

                redirectVisual = sourceVisual->Compositor->CreateRedirectVisual(sourceVisual);
                VERIFY_IS_NOT_NULL(redirectVisual);

                ElementCompositionPreview::SetElementChildVisual(targetElement, redirectVisual);
            });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
        }

    } }
} } } }
