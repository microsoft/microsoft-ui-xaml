// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextFontFallback.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeatureOverride.h>
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
using namespace Microsoft::UI::Xaml::Media;
using namespace ::Windows::Globalization;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation {
        namespace Text {

            bool TextFontFallbackTests::ClassSetup()
            {
                CommonTestSetupHelper::CommonTestClassSetup();
                return true;
            }

            bool TextFontFallbackTests::ClassCleanup()
            {
                return true;
            }

            bool TextFontFallbackTests::TestSetup()
            {
                savedPrimaryLanguageOverride =  ::Windows::Globalization::ApplicationLanguages::PrimaryLanguageOverride;
                return true;
            }

            bool TextFontFallbackTests::TestCleanup()
            {
                ::Windows::Globalization::ApplicationLanguages::PrimaryLanguageOverride = savedPrimaryLanguageOverride;
                TestServices::WindowHelper->VerifyTestCleanup();
                return true;
            }

            Platform::String^ TextFontFallbackTests::GetResourcesPath() const
            {
                return GetPackageFolder() + L"resources\\native\\foundation\\Text\\";
            }

            void TextFontFallbackTests::TextFontFallbackControlsDesktop()
            {
                // on Desktop, the default EA font fallback priority for Japanese is higher, so targeting test to check if Chinese text fallback correctly
                TextFontFallbackControlsTestHelper(L"TextFontFallbackDesktop.xaml", L"zh-CN");
            }

            void TextFontFallbackTests::TextFontFallbackLanguageUpdateDesktop()
            {
                // test updating language property on each individual control for font fallback
                TextFontFallbackLanguageUpdateTestHelper(L"TextFontFallbackDesktop.xaml", L"zh-CN");
            }

            void TextFontFallbackTests::AutoFontJa()
            {
                FontNameValidationHelper("ja", "Yu Gothic UI");
            }

            void TextFontFallbackTests::AutoFontKo()
            {
                FontNameValidationHelper("ko", "Malgun Gothic");
            }

            void TextFontFallbackTests::AutoFontEN()
            {
                FontNameValidationHelper("en", "Segoe UI");
            }

            void TextFontFallbackTests::FontNameValidationHelper(Platform::String^ appLanguage, Platform::String^ defaultFontName)
            {
                ::Windows::Globalization::ApplicationLanguages::PrimaryLanguageOverride = appLanguage;
                TestServices::Utilities->ClearDefaultLanguageString();
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = nullptr;
                });

                // All TextBlocks rendered should be in green color, that means they are using DWriteTextLayout for measure/arrange.
                RuntimeEnabledFeatureOverride featureDrawDWriteTextLayoutInGreen(RuntimeFeatureBehavior::RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, true);

                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
                TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800,600));

                Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TextControlsWithBackSlash.xaml"));
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = root;
                });

                TestServices::WindowHelper->WaitForIdle();
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);

                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

                TextBlock^ tb = nullptr;
                RunOnUIThread([&]()
                {
                    tb = ref new TextBlock();
                    tb->FontFamily = ref new FontFamily("Segoe UI");
                    tb->FontFamily = FontFamily::XamlAutoFontFamily;
                    VERIFY_IS_TRUE(defaultFontName == tb->FontFamily->Source);
                    VERIFY_IS_TRUE(tb->FontFamily->Source == FontFamily::XamlAutoFontFamily->Source);
                });
            }

            void TextFontFallbackTests::TextFontFallbackControlsTestHelper(Platform::String^ filename, Platform::String^ languageOverride)
            {
                // set primary application language override which will be used by BCP47 for resolving font fallback language list
                ::Windows::Globalization::ApplicationLanguages::PrimaryLanguageOverride = languageOverride;

                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = nullptr;
                });

                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
                TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

                Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + filename));

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(Panel, Loaded);

                RunOnUIThread([&]()
                {
                    // TestServicesStatics::EnsureStylesAreLoaded populates language property during test initialization
                    // So we need set the language property here to force generation of fallback language list again (using PrimaryLanguageOverride set earlier)
                    root->Language = L"en-US";

                    rootLoadedRegistration.Attach(
                        root,
                        ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root FE Loaded handler.");
                        rootLoadedEvent->Set();
                    }));
                    TestServices::WindowHelper->WindowContent = root;
                });

                LOG_OUTPUT(L"Waiting for root Loaded event.");
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();
                TestServices::WindowHelper->SynchronouslyTickUIThread(2);
                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"Resetting surface id.");
                TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
                TestServices::Utilities->ResetMockDCompSurfaceId();
                TestServices::WindowHelper->WaitForIdle();
                LOG_OUTPUT(L"Dump Dcomp to check EA font output.");
                // dump all surfaces to verify expected EA ext displayed.
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
            }

            void TextFontFallbackTests::TextFontFallbackLanguageUpdateTestHelper(Platform::String^ filename, Platform::String^ languageOverride)
            {
                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = nullptr;
                });

                WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
                TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

                Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + filename));

                auto rootLoadedEvent = std::make_shared<Event>();
                auto rootLoadedRegistration = CreateSafeEventRegistration(Panel, Loaded);

                RunOnUIThread([&]()
                {
                    TestServices::WindowHelper->WindowContent = root;
                    rootLoadedRegistration.Attach(
                        root,
                        ref new xaml::RoutedEventHandler(
                        [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                    {
                        LOG_OUTPUT(L"Root FE Loaded handler.");
                        rootLoadedEvent->Set();
                    }));
                });

                LOG_OUTPUT(L"Waiting for root Loaded event.");
                rootLoadedEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    // set language property on each individual control
                    auto fastPathTextBlock = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"fastPathTextBlock"));
                    VERIFY_IS_NOT_NULL(fastPathTextBlock);
                    fastPathTextBlock->Language = languageOverride;

                    auto slowPathTextBlock = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"slowPathTextBlock"));
                    VERIFY_IS_NOT_NULL(slowPathTextBlock);
                    slowPathTextBlock->Language = languageOverride;
                });

                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Dump Dcomp to check EA font output.");
                // dump all surfaces to verify expected EA ext displayed.
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
            }
        }
    }
} } } }
