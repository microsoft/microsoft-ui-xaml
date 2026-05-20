// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ColorTextTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        Platform::String^ ColorTextTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        }

        bool ColorTextTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ColorTextTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ColorTextTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Renders TextBlocks and Glyphs with color text enabled and disabled.
        //------------------------------------------------------------------------
        void ColorTextTests::BasicColorTest()
        {
            // Clear out the current window content before injecting MockDComp, to
            // MockDComp doesn't capture an image for anything currently in the content,
            // since that will interfere with the expected surface counts.
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorTextTests-BasicColorTest.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
        }

        //------------------------------------------------------------------------
        // Test case: Changes the ColorFont properties and verifies correct updated rendering.
        //------------------------------------------------------------------------
        void ColorTextTests::PropertyChanges()
        {
            // Clear out the current window content before injecting MockDComp, to
            // MockDComp doesn't capture an image for anything currently in the content,
            // since that will interfere with the expected surface counts.
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"ColorTextTests-PropertyChanges.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            Glyphs^ g4;

            RunOnUIThread([&]()
            {
                TextBlock^ tb1 = safe_cast<TextBlock^>(root->FindName("tb1"));
                TextBlock^ tb2 = safe_cast<TextBlock^>(root->FindName("tb2"));
                Glyphs^ g1 = safe_cast<Glyphs^>(root->FindName("g1"));
                Glyphs^ g2 = safe_cast<Glyphs^>(root->FindName("g2"));
                Glyphs^ g3 = safe_cast<Glyphs^>(root->FindName("g3"));
                g4 = safe_cast<Glyphs^>(root->FindName("g4"));

                VERIFY_IS_NOT_NULL(tb1);
                VERIFY_IS_NOT_NULL(tb2);
                VERIFY_IS_NOT_NULL(g1);
                VERIFY_IS_NOT_NULL(g2);
                VERIFY_IS_NOT_NULL(g3);
                VERIFY_IS_NOT_NULL(g4);

                // Swap the IsColorFontEnabled property on the TextBlocks and first two Glyphs
                tb1->IsColorFontEnabled = !tb1->IsColorFontEnabled;
                tb2->IsColorFontEnabled = !tb2->IsColorFontEnabled;
                g1->IsColorFontEnabled = !g1->IsColorFontEnabled;
                g2->IsColorFontEnabled = !g2->IsColorFontEnabled;

                // Change the ColorFontPaletteIndex on the third Glyphs
                VERIFY_IS_TRUE(g3->IsColorFontEnabled);
                g3->ColorFontPaletteIndex = 1;

            });

            TraceConsumer::Start();

            RunOnUIThread([&]()
            {
                // Change the ColorFontPaletteIndex on the fourth Glyphs to an invalid palette index and verify telemetry invocation
                VERIFY_IS_TRUE(g4->IsColorFontEnabled);
                g4->ColorFontPaletteIndex = 5;
            });

            TestServices::WindowHelper->WaitForIdle();
            TraceConsumer::Stop();
            TraceConsumer::VerifyEventTraced("NoColorGlyphRendering", 1);

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
        }

    } }
} } } }
